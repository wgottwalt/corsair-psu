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
    { 0x1b1c, 0x1c03, "Corsair", "HX550i" },
    { 0x1b1c, 0x1c04, "Corsair", "HX650i" },
    { 0x1b1c, 0x1c05, "Corsair", "HX750i" },
    { 0x1b1c, 0x1c06, "Corsair", "HX850i" },
    { 0x1b1c, 0x1c07, "Corsair", "HX1000i" },
    { 0x1b1c, 0x1c08, "Corsair", "HX1200i" },
    { 0x1b1c, 0x1c09, "Corsair", "RM550i" },
    { 0x1b1c, 0x1c0a, "Corsair", "RM650i" },
    { 0x1b1c, 0x1c0b, "Corsair", "RM750i" },
    { 0x1b1c, 0x1c0c, "Corsair", "RM850i" },
    { 0x1b1c, 0x1c0d, "Corsair", "RM1000i" },
};

static std::mutex __mutex;

//--- public constructors ---

Query::Query() noexcept
: _hid_dev(nullptr), _vname(""), _pname(""), _buffer(), _vid(0), _pid(0), _valid(false),
  _init_failed(false)
{
    if (hid_init() != 0)
        return;

    for (const auto &[vid, pid, vname, pname] : __devices)
    {
        if (_hid_dev = hid_open(vid, pid, nullptr); _hid_dev)
        {
            _vid = vid;
            _pid = pid;
            _vname = vname;
            _pname = pname;
            _valid = true;

            break;
        }
    }

    if (_hid_dev)
    {
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
}

//--- public methods ---

bool Query::valid() const noexcept
{
    return _valid && _hid_dev && !_init_failed;
}

uint16_t Query::vid() const noexcept
{
    return _vid;
}

uint16_t Query::pid() const noexcept
{
    return _pid;
}

std::string Query::vendorName() const noexcept
{
    return _vname;
}

std::string Query::productName() const noexcept
{
    return _pname;
}

//--- protected methods ---

bool Query::init()
{
    int32_t err = cmd(CMD_INIT, 0x03, 0x00);

    if (err >= 0)
        return true;

    return false;
}

void Query::cleanup()
{
    if (_hid_dev)
        hid_close(_hid_dev);
    hid_exit();
}

int32_t Query::cmd(const uint8_t p0, const uint8_t p1, const uint8_t p2, uint32_t *data)
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
        std::copy_n(reinterpret_cast<char *>(data), sizeof (*data), _buffer.begin());

    return 0;
}
