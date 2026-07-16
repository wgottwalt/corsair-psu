// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * corsair-psu-axi.c - Linux hwmon driver for Corsair AXi power supplies
 *
 * The AX1600i exposes a vendor-specific bulk USB interface instead of the
 * HID interface used by Corsair RMi and HXi power supplies.
 *
 * AX1600i transport and register protocol based on the corsair-top project:
 * https://github.com/thad0ctor/corsair-top
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/hwmon.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/unaligned.h>
#include <linux/usb.h>

#define DRIVER_NAME		"corsair-psu-axi"

#define CORSAIR_VENDOR_ID	0x1b1c
#define CORSAIR_AX1600I_ID	0x1c11

#define AXI_USB_TIMEOUT_MS	1000
#define AXI_MAX_ENCODED_SIZE	64
#define AXI_MAX_DECODED_SIZE	31

#define AXI_CMD_SETUP		0x11
#define AXI_CMD_EXECUTE		0x12
#define AXI_CMD_TRANSFER	0x13
#define AXI_CMD_READ		0x08

#define AXI_REG_SELECT_RAIL	0x00
#define AXI_REG_IN_VOLTS	0x88
#define AXI_REG_IN_AMPS		0x89
#define AXI_REG_RAIL_VOLTS	0x8b
#define AXI_REG_RAIL_AMPS	0x8c
#define AXI_REG_TEMP0		0x8d
#define AXI_REG_TEMP1		0x8e
#define AXI_REG_FAN		0x90
#define AXI_REG_RAIL_WATTS	0x96
#define AXI_REG_PRODUCT		0x9a
#define AXI_REG_UPTIME		0xd2
#define AXI_REG_TOTAL_WATTS	0xee

#define AXI_RAIL_COUNT		3
#define AXI_TEMP_COUNT		2

#define L_IN_VOLTS		"v_in"
#define L_OUT_VOLTS_12V		"v_out +12v"
#define L_OUT_VOLTS_5V		"v_out +5v"
#define L_OUT_VOLTS_3_3V	"v_out +3.3v"
#define L_IN_AMPS		"curr in"
#define L_AMPS_12V		"curr +12v"
#define L_AMPS_5V		"curr +5v"
#define L_AMPS_3_3V		"curr +3.3v"
#define L_FAN			"psu fan"
#define L_TEMP0			"vrm temp"
#define L_TEMP1			"case temp"
#define L_WATTS			"power total"
#define L_WATTS_12V		"power +12v"
#define L_WATTS_5V		"power +5v"
#define L_WATTS_3_3V		"power +3.3v"

static const u8 axi_encode_table[] = {
	0x55, 0x56, 0x59, 0x5a, 0x65, 0x66, 0x69, 0x6a,
	0x95, 0x96, 0x99, 0x9a, 0xa5, 0xa6, 0xa9, 0xaa
};

static const char *const label_watts[] = {
	L_WATTS, L_WATTS_12V, L_WATTS_5V, L_WATTS_3_3V
};

static const char *const label_volts[] = {
	L_IN_VOLTS, L_OUT_VOLTS_12V, L_OUT_VOLTS_5V, L_OUT_VOLTS_3_3V
};

static const char *const label_amps[] = {
	L_IN_AMPS, L_AMPS_12V, L_AMPS_5V, L_AMPS_3_3V
};

struct corsairpsu_axi_data {
	struct usb_device *udev;
	struct usb_interface *interface;
	struct device *hwmon_dev;
	struct mutex lock;
	u8 bulk_in;
	u8 bulk_out;
	u8 *tx_buffer;
	u8 *rx_buffer;
	char product[8];
	bool disconnected;
};

static int axi_decode_nibble(u8 encoded)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(axi_encode_table); i++)
		if (axi_encode_table[i] == encoded)
			return i;

	return -EPROTO;
}

static int axi_encode(const u8 *src, size_t src_len, u8 *dst, size_t dst_size)
{
	size_t i;

	if (src_len > (dst_size - 2) / 2)
		return -EMSGSIZE;

	dst[0] = axi_encode_table[0] & 0xfc;
	for (i = 0; i < src_len; i++) {
		dst[1 + i * 2] = axi_encode_table[src[i] & 0x0f];
		dst[2 + i * 2] = axi_encode_table[src[i] >> 4];
	}
	dst[1 + src_len * 2] = 0;

	return 2 + src_len * 2;
}

static int axi_decode(const u8 *src, size_t src_len, u8 *dst, size_t dst_size)
{
	size_t encoded_payload_len;
	size_t i;
	int low;
	int high;

	if (src_len < 2 || src[src_len - 1] != 0)
		return -EPROTO;

	encoded_payload_len = src_len - 2;
	if (encoded_payload_len & 1)
		return -EPROTO;
	if (encoded_payload_len / 2 > dst_size)
		return -EMSGSIZE;

	for (i = 0; i < encoded_payload_len / 2; i++) {
		low = axi_decode_nibble(src[1 + i * 2]);
		high = axi_decode_nibble(src[2 + i * 2]);
		if (low < 0 || high < 0)
			return -EPROTO;
		dst[i] = low | (high << 4);
	}

	return encoded_payload_len / 2;
}

static int axi_exchange(struct corsairpsu_axi_data *priv, const u8 *request,
			size_t request_len, u8 *response, size_t response_size)
{
	int encoded_len;
	int actual_len;
	int ret;

	encoded_len = axi_encode(request, request_len, priv->tx_buffer,
				 AXI_MAX_ENCODED_SIZE);
	if (encoded_len < 0)
		return encoded_len;

	usleep_range(1000, 2000);
	ret = usb_bulk_msg(priv->udev, usb_sndbulkpipe(priv->udev, priv->bulk_out),
			   priv->tx_buffer, encoded_len, &actual_len,
			   AXI_USB_TIMEOUT_MS);
	if (ret)
		return ret;
	if (actual_len != encoded_len)
		return -EIO;

	ret = usb_bulk_msg(priv->udev, usb_rcvbulkpipe(priv->udev, priv->bulk_in),
			   priv->rx_buffer, AXI_MAX_ENCODED_SIZE, &actual_len,
			   AXI_USB_TIMEOUT_MS);
	if (ret)
		return ret;
	if (!actual_len)
		return -ENODATA;

	return axi_decode(priv->rx_buffer, actual_len, response, response_size);
}

static int axi_expect_status(struct corsairpsu_axi_data *priv, const u8 *request,
			     size_t request_len)
{
	u8 response[2];
	int ret;

	ret = axi_exchange(priv, request, request_len, response, sizeof(response));
	if (ret < 0)
		return ret;
	if (ret && response[0])
		return -EIO;

	return 0;
}

static int axi_setup(struct corsairpsu_axi_data *priv)
{
	static const u8 setup[] = {
		AXI_CMD_SETUP, 0x02, 0x64, 0x00, 0x00, 0x00, 0x00
	};

	return axi_expect_status(priv, setup, sizeof(setup));
}

static int axi_read_register_locked(struct corsairpsu_axi_data *priv, u8 reg,
				    u8 *data, u8 data_len)
{
	u8 response[AXI_MAX_DECODED_SIZE];
	u8 request[] = {
		AXI_CMD_TRANSFER, 0x03, 0x06, 0x01, 0x07, data_len, reg
	};
	static const u8 execute[] = { AXI_CMD_EXECUTE };
	u8 read[] = { AXI_CMD_READ, 0x07, data_len };
	int ret;

	ret = axi_expect_status(priv, request, sizeof(request));
	if (ret)
		return ret;

	ret = axi_exchange(priv, execute, sizeof(execute), response, sizeof(response));
	if (ret < 0)
		return ret;
	if (ret && response[0])
		return -EIO;

	ret = axi_exchange(priv, read, sizeof(read), response, sizeof(response));
	if (ret < data_len)
		return ret < 0 ? ret : -EPROTO;

	memcpy(data, response, data_len);
	return 0;
}

static int axi_write_register_locked(struct corsairpsu_axi_data *priv, u8 reg,
				     const u8 *data, u8 data_len)
{
	u8 request[5 + 1];
	u8 response[2];
	static const u8 execute[] = { AXI_CMD_EXECUTE };
	int ret;

	if (data_len != 1)
		return -EINVAL;

	request[0] = AXI_CMD_TRANSFER;
	request[1] = 0x01;
	request[2] = 0x04;
	request[3] = data_len;
	request[4] = reg;
	request[5] = data[0];

	ret = axi_expect_status(priv, request, sizeof(request));
	if (ret)
		return ret;

	ret = axi_exchange(priv, execute, sizeof(execute), response, sizeof(response));
	if (ret < 0)
		return ret;
	if (ret && response[0])
		return -EIO;

	return 0;
}

static int axi_select_rail_locked(struct corsairpsu_axi_data *priv, u8 rail)
{
	if (rail >= AXI_RAIL_COUNT)
		return -EINVAL;

	return axi_write_register_locked(priv, AXI_REG_SELECT_RAIL, &rail, sizeof(rail));
}

static int axi_linear11_to_int(u16 raw, int scale, long *value)
{
	int exponent = sign_extend32(raw >> 11, 4);
	int mantissa = sign_extend32(raw & 0x7ff, 10);
	s64 result = (s64)mantissa * scale;

	if (exponent >= 0)
		result <<= exponent;
	else
		result >>= -exponent;

	if (result > LONG_MAX || result < LONG_MIN)
		return -ERANGE;

	*value = result;
	return 0;
}

static int axi_get_value(struct corsairpsu_axi_data *priv, u8 reg, int rail,
			 int scale, long *value)
{
	u8 data[2];
	int ret;

	mutex_lock(&priv->lock);
	if (priv->disconnected) {
		ret = -ENODEV;
		goto out;
	}
	if (rail >= 0) {
		ret = axi_select_rail_locked(priv, rail);
		if (ret)
			goto out;
	}

	ret = axi_read_register_locked(priv, reg, data, sizeof(data));
	if (!ret)
		ret = axi_linear11_to_int(get_unaligned_le16(data), scale, value);
out:
	mutex_unlock(&priv->lock);
	return ret;
}

static umode_t axi_hwmon_is_visible(const void *data, enum hwmon_sensor_types type,
				    u32 attr, int channel)
{
	switch (type) {
	case hwmon_temp:
		return (attr == hwmon_temp_input || attr == hwmon_temp_label) ? 0444 : 0;
	case hwmon_fan:
		return (attr == hwmon_fan_input || attr == hwmon_fan_label) ? 0444 : 0;
	case hwmon_power:
		return (attr == hwmon_power_input || attr == hwmon_power_label) ? 0444 : 0;
	case hwmon_in:
		return (attr == hwmon_in_input || attr == hwmon_in_label) ? 0444 : 0;
	case hwmon_curr:
		return (attr == hwmon_curr_input || attr == hwmon_curr_label) ? 0444 : 0;
	default:
		return 0;
	}
}

static int axi_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
			  u32 attr, int channel, long *value)
{
	struct corsairpsu_axi_data *priv = dev_get_drvdata(dev);

	switch (type) {
	case hwmon_temp:
		if (attr != hwmon_temp_input || channel >= AXI_TEMP_COUNT)
			return -EOPNOTSUPP;
		return axi_get_value(priv, channel ? AXI_REG_TEMP1 : AXI_REG_TEMP0,
				     -1, 1000, value);
	case hwmon_fan:
		if (attr != hwmon_fan_input || channel)
			return -EOPNOTSUPP;
		return axi_get_value(priv, AXI_REG_FAN, -1, 1, value);
	case hwmon_power:
		if (attr != hwmon_power_input || channel > AXI_RAIL_COUNT)
			return -EOPNOTSUPP;
		if (!channel)
			return axi_get_value(priv, AXI_REG_TOTAL_WATTS, -1, 1000000, value);
		return axi_get_value(priv, AXI_REG_RAIL_WATTS, channel - 1,
				     1000000, value);
	case hwmon_in:
		if (attr != hwmon_in_input || channel > AXI_RAIL_COUNT)
			return -EOPNOTSUPP;
		if (!channel)
			return axi_get_value(priv, AXI_REG_IN_VOLTS, -1, 1000, value);
		return axi_get_value(priv, AXI_REG_RAIL_VOLTS, channel - 1, 1000, value);
	case hwmon_curr:
		if (attr != hwmon_curr_input || channel > AXI_RAIL_COUNT)
			return -EOPNOTSUPP;
		if (!channel)
			return axi_get_value(priv, AXI_REG_IN_AMPS, -1, 1000, value);
		return axi_get_value(priv, AXI_REG_RAIL_AMPS, channel - 1, 1000, value);
	default:
		return -EOPNOTSUPP;
	}
}

static int axi_hwmon_read_string(struct device *dev, enum hwmon_sensor_types type,
				 u32 attr, int channel, const char **str)
{
	if (type == hwmon_temp && attr == hwmon_temp_label && channel < AXI_TEMP_COUNT) {
		*str = channel ? L_TEMP1 : L_TEMP0;
		return 0;
	}
	if (type == hwmon_fan && attr == hwmon_fan_label && !channel) {
		*str = L_FAN;
		return 0;
	}
	if (type == hwmon_power && attr == hwmon_power_label &&
	    channel < ARRAY_SIZE(label_watts)) {
		*str = label_watts[channel];
		return 0;
	}
	if (type == hwmon_in && attr == hwmon_in_label &&
	    channel < ARRAY_SIZE(label_volts)) {
		*str = label_volts[channel];
		return 0;
	}
	if (type == hwmon_curr && attr == hwmon_curr_label &&
	    channel < ARRAY_SIZE(label_amps)) {
		*str = label_amps[channel];
		return 0;
	}

	return -EOPNOTSUPP;
}

static const struct hwmon_ops axi_hwmon_ops = {
	.is_visible = axi_hwmon_is_visible,
	.read = axi_hwmon_read,
	.read_string = axi_hwmon_read_string,
};

static const struct hwmon_channel_info *const axi_hwmon_info[] = {
	HWMON_CHANNEL_INFO(temp,
			   HWMON_T_INPUT | HWMON_T_LABEL,
			   HWMON_T_INPUT | HWMON_T_LABEL),
	HWMON_CHANNEL_INFO(fan,
			   HWMON_F_INPUT | HWMON_F_LABEL),
	HWMON_CHANNEL_INFO(power,
			   HWMON_P_INPUT | HWMON_P_LABEL,
			   HWMON_P_INPUT | HWMON_P_LABEL,
			   HWMON_P_INPUT | HWMON_P_LABEL,
			   HWMON_P_INPUT | HWMON_P_LABEL),
	HWMON_CHANNEL_INFO(in,
			   HWMON_I_INPUT | HWMON_I_LABEL,
			   HWMON_I_INPUT | HWMON_I_LABEL,
			   HWMON_I_INPUT | HWMON_I_LABEL,
			   HWMON_I_INPUT | HWMON_I_LABEL),
	HWMON_CHANNEL_INFO(curr,
			   HWMON_C_INPUT | HWMON_C_LABEL,
			   HWMON_C_INPUT | HWMON_C_LABEL,
			   HWMON_C_INPUT | HWMON_C_LABEL,
			   HWMON_C_INPUT | HWMON_C_LABEL),
	NULL
};

static const struct hwmon_chip_info axi_chip_info = {
	.ops = &axi_hwmon_ops,
	.info = axi_hwmon_info,
};

static int axi_find_endpoints(struct usb_interface *interface,
			      struct corsairpsu_axi_data *priv)
{
	struct usb_host_interface *alt = interface->cur_altsetting;
	struct usb_endpoint_descriptor *endpoint;
	int i;

	for (i = 0; i < alt->desc.bNumEndpoints; i++) {
		endpoint = &alt->endpoint[i].desc;
		if (usb_endpoint_is_bulk_in(endpoint))
			priv->bulk_in = usb_endpoint_num(endpoint);
		else if (usb_endpoint_is_bulk_out(endpoint))
			priv->bulk_out = usb_endpoint_num(endpoint);
	}

	return priv->bulk_in && priv->bulk_out ? 0 : -ENODEV;
}

static int axi_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct corsairpsu_axi_data *priv;
	u8 product[sizeof(priv->product) - 1];
	int ret;

	priv = devm_kzalloc(&interface->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->udev = usb_get_dev(interface_to_usbdev(interface));
	priv->interface = interface;
	priv->tx_buffer = devm_kmalloc(&interface->dev, AXI_MAX_ENCODED_SIZE,
				      GFP_KERNEL);
	priv->rx_buffer = devm_kmalloc(&interface->dev, AXI_MAX_ENCODED_SIZE,
				      GFP_KERNEL);
	if (!priv->tx_buffer || !priv->rx_buffer) {
		ret = -ENOMEM;
		goto err_put_dev;
	}
	mutex_init(&priv->lock);
	usb_set_intfdata(interface, priv);

	ret = axi_find_endpoints(interface, priv);
	if (ret)
		goto err_put_dev;

	ret = usb_control_msg(priv->udev, usb_sndctrlpipe(priv->udev, 0),
			      0x02, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
			      0x0002, 0, NULL, 0, AXI_USB_TIMEOUT_MS);
	if (ret < 0)
		goto err_put_dev;

	mutex_lock(&priv->lock);
	ret = axi_setup(priv);
	if (!ret)
		ret = axi_read_register_locked(priv, AXI_REG_PRODUCT, product, sizeof(product));
	mutex_unlock(&priv->lock);
	if (ret)
		goto err_put_dev;

	memcpy(priv->product, product, sizeof(product));
	priv->product[sizeof(product)] = '\0';
	if (strncmp(priv->product, "AX1600i", sizeof(product))) {
		ret = -ENODEV;
		goto err_put_dev;
	}

	priv->hwmon_dev = hwmon_device_register_with_info(&interface->dev,
							  "corsairpsu_axi", priv,
							  &axi_chip_info, NULL);
	if (IS_ERR(priv->hwmon_dev)) {
		ret = PTR_ERR(priv->hwmon_dev);
		goto err_put_dev;
	}

	dev_info(&interface->dev, "Corsair %s power supply connected\n", priv->product);
	return 0;

err_put_dev:
	usb_set_intfdata(interface, NULL);
	usb_put_dev(priv->udev);
	return ret;
}

static void axi_disconnect(struct usb_interface *interface)
{
	struct corsairpsu_axi_data *priv = usb_get_intfdata(interface);

	if (!priv)
		return;

	usb_set_intfdata(interface, NULL);
	mutex_lock(&priv->lock);
	priv->disconnected = true;
	mutex_unlock(&priv->lock);
	hwmon_device_unregister(priv->hwmon_dev);
	usb_put_dev(priv->udev);
}

static const struct usb_device_id axi_id_table[] = {
	{ USB_DEVICE(CORSAIR_VENDOR_ID, CORSAIR_AX1600I_ID) },
	{ }
};
MODULE_DEVICE_TABLE(usb, axi_id_table);

static struct usb_driver axi_driver = {
	.name = DRIVER_NAME,
	.probe = axi_probe,
	.disconnect = axi_disconnect,
	.id_table = axi_id_table,
};
module_usb_driver(axi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("corsair-psu contributors");
MODULE_DESCRIPTION("Linux hwmon driver for Corsair AXi power supplies");
