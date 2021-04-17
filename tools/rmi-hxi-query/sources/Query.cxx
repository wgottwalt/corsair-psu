#include <algorithm>
#include <cerrno>
#include <mutex>
#include "Query.hxx"

#define CMD_SELECT_RAIL 0x00
#define CMD_RAIL_VOLTS_HCRIT 0x40
#define CMD_RAIL_VOLTS_LCRIT 0x44
#define CMD_RAIL_AMPS_HCRIT 0x46
#define CMD_TEMP_HCRIT 0x4F
#define CMD_IN_VOLTS 0x88
#define CMD_IN_AMPS 0x89
#define CMD_RAIL_VOLTS 0x8B
#define CMD_RAIL_AMPS 0x8C
#define CMD_TEMP0 0x8D
#define CMD_TEMP1 0x8E
#define CMD_FAN 0x90
#define CMD_RAIL_WATTS 0x96
#define CMD_VEND_STR 0x99
#define CMD_PROD_STR 0x9A
#define CMD_TOTAL_WATTS 0xEE
#define CMD_TOTAL_UPTIME 0xD1
#define CMD_UPTIME 0xD2
#define CMD_INIT 0xFE

//--- internal stuff ---

static const Query::USBDevice __devices[] = {
    { 0x1b1c, 0x1c03 },
    { 0x1b1c, 0x1c04 },
    { 0x1b1c, 0x1c05 },
    { 0x1b1c, 0x1c06 },
    { 0x1b1c, 0x1c07 },
    { 0x1b1c, 0x1c08 },
    { 0x1b1c, 0x1c09 },
    { 0x1b1c, 0x1c0a },
    { 0x1b1c, 0x1c0b },
    { 0x1b1c, 0x1c0c },
    { 0x1b1c, 0x1c0d },
};

struct Data {
    std::string vendor;
    std::string product;
    float temp_crut[2];
    float in_crit[3];
    float in_lcrit[3];
    float curr_crit[3];
};

static std::mutex __mutex;

//--- public constructors ---

Query::Query() noexcept
: _hid_dev(nullptr), _data(new Data), _buffer(), _vid(0), _pid(0), _valid(false),
  _init_failed(false)
{
    if (hid_init() != 0)
        return;

    for (const auto &[vid, pid] : __devices)
    {
        if (_hid_dev = hid_open(vid, pid, nullptr); _hid_dev)
        {
            _vid = vid;
            _pid = pid;
            _valid = true;

            break;
        }
    }

    if (_hid_dev)
    {
        _data->vendor.resize(ReplySize, '\0');
        _data->product.resize(ReplySize, '\0');

        if (!init())
        {
            _init_failed = true;
            cleanup();
        }
    }
}

Query::~Query() noexcept
{
    if (!_init_failed)
        cleanup();
    delete _data;
}

//--- public methods ---

bool Query::valid() const noexcept
{
    return _valid && _hid_dev && !_init_failed;
}

uint16_t Query::vid() const noexcept
{
    if (valid())
        return _vid;

    return 0;
}

uint16_t Query::pid() const noexcept
{
    if (valid())
        return _pid;

    return 0;
}

std::string Query::vendorName() const noexcept
{
    if (valid())
        return _data->vendor;

    return "undef";
}

std::string Query::productName() const noexcept
{
    if (valid())
        return _data->product;

    return "undef";
}

//--- protected methods ---

bool Query::init() noexcept
{
    if (int32_t err = cmd(CMD_INIT, 0x03, 0x00); err >= 0)
    {
        if (err = fwinfo(); err < 0)
            return false;

        return true;
    }

    return false;
}

void Query::cleanup() noexcept
{
    if (_hid_dev)
        hid_close(_hid_dev);
    hid_exit();
}

int32_t Query::linearToInt(const uint16_t val, const int32_t scale) const noexcept
{
	const int32_t exp = (static_cast<int16_t>(val)) >> 11;
	const int32_t mant = ((static_cast<int16_t>(val & 0x7ff)) << 5) >> 5;
	const int32_t result = mant * scale;

	return (exp >= 0) ? (result << exp) : (result >> -exp);
}

int32_t Query::cmd(const uint8_t p0, const uint8_t p1, const uint8_t p2, void *data) noexcept
{
    int32_t err = 0;

    std::fill(_buffer.begin(), _buffer.end(), 0);
    _buffer[0] = p0;
    _buffer[1] = p1;
    _buffer[2] = p2;

    if (hid_write(_hid_dev, reinterpret_cast<const uint8_t *>(_buffer.data()), _buffer.size()) < 0)
        return -EIO;

    err = hid_read_timeout(_hid_dev, reinterpret_cast<uint8_t *>(_buffer.data()), _buffer.size(),
                           TimeoutMS);
    switch (err)
    {
        case -1:
            return -EIO;
        case 0:
            return -EAGAIN;
        default:
            break;
    }

    if ((p0 != _buffer[0]) || (p1 != _buffer[1]))
        return -EFAULT;

    if (data)
        std::copy_n(_buffer.data() + 2, ReplySize, reinterpret_cast<char *>(data));

    return 0;
}

int32_t Query::fwinfo() noexcept
{
    int32_t err = 0;

    if (err = cmd(3, CMD_VEND_STR, 0, _data->vendor.data()); err < 0)
        return err;
    if (err = cmd(3, CMD_PROD_STR, 0, _data->product.data()); err < 0)
        return err;

    return err;
}

int32_t Query::criticals() noexcept
{
}
