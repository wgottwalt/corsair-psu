This is the main development repository for the corsair-psu hwmon driver which
is also part of mainline.

This driver supports all Corsair RMi and HXi series power supplies. This repo
is used to add support for the AXi series power supplies, which are quite
similar, but do not work with the USB HID code of the mainlined driver.

There are also tools to access/test the power supplies using libusb which will
be added to the kernel driver if it works and is tested.
