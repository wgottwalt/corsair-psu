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
    float temp_hcrit[2];
    float in_hcrit[3];
    float in_lcrit[3];
    float curr_hcrit[3];
    uint8_t temp_hcrit_support;
    uint8_t in_hcrit_support;
    uint8_t in_lcrit_support;
    uint8_t curr_hcrit_support;
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

Query::Result Query::value(const Values val) noexcept
{
    if (valid())
    {
        int32_t result = 0;
        int32_t err = 0;

        switch (val)
        {
            case Values::HighCritTemp0:
                return {_data->temp_hcrit[0], _data->temp_hcrit_support & 1};
            case Values::HighCritTemp1:
                return {_data->temp_hcrit[1], _data->temp_hcrit_support & 2};
            case Values::HighCritCurr12v:
                return {_data->curr_hcrit[0], _data->in_hcrit_support & 1};
            case Values::HighCritCurr5v:
                return {_data->curr_hcrit[1], _data->in_hcrit_support & 2};
            case Values::HighCritCurr3v3:
                return {_data->curr_hcrit[2], _data->in_hcrit_support & 4};
            case Values::HighCritVolt12v:
                return {_data->in_hcrit[0], _data->in_lcrit_support & 1};
            case Values::HighCritVolt5v:
                return {_data->in_hcrit[1], _data->in_lcrit_support & 2};
            case Values::HighCritVolt3v3:
                return {_data->in_hcrit[2], _data->in_lcrit_support & 4};
            case Values::LowCritVolt12v:
                return {_data->in_lcrit[0], _data->curr_hcrit_support & 1};
            case Values::LowCritVolt5v:
                return {_data->in_lcrit[1], _data->curr_hcrit_support & 2};
            case Values::LowCritVolt3v3:
                return {_data->in_lcrit[2], _data->curr_hcrit_support & 4};
            case Values::Temp0:
                err = getValue(CMD_TEMP0, 0, &result);
                break;
            case Values::Temp1:
                err = getValue(CMD_TEMP1, 0, &result);
                break;
            case Values::Curr12v:
                err = getValue(CMD_RAIL_AMPS, 0, &result);
                break;
            case Values::Curr5v:
                err = getValue(CMD_RAIL_AMPS, 1, &result);
                break;
            case Values::Curr3v3:
                err = getValue(CMD_RAIL_AMPS, 2, &result);
                break;
            case Values::Volt12v:
                err = getValue(CMD_RAIL_VOLTS, 0, &result);
                break;
            case Values::Volt5v:
                err = getValue(CMD_RAIL_VOLTS, 1, &result);
                break;
            case Values::Volt3v3:
                err = getValue(CMD_RAIL_VOLTS, 2, &result);
                break;
            case Values::Watt12v:
                err = getValue(CMD_RAIL_WATTS, 0, &result);
                break;
            case Values::Watt5v:
                err = getValue(CMD_RAIL_WATTS, 1, &result);
                break;
            case Values::Watt3v3:
                err = getValue(CMD_RAIL_WATTS, 2, &result);
                break;
            case Values::VoltIn:
                err = getValue(CMD_IN_VOLTS, 2, &result);
                break;
            case Values::Fan:
                err = getValue(CMD_FAN, 2, &result);
                break;
        }

        if (err >= 0)
            return {result / 1000.0, true};
    }

    return {0, false};
}

//--- protected methods ---

int32_t Query::linearToInt(const uint16_t val, const int32_t scale) const noexcept
{
	const int32_t exp = (static_cast<int16_t>(val)) >> 11;
	const int32_t mant = ((static_cast<int16_t>(val & 0x7ff)) << 5) >> 5;
	const int32_t result = mant * scale;

	return (exp >= 0) ? (result << exp) : (result >> -exp);
}

int32_t Query::hidCmd(const uint8_t p0, const uint8_t p1, const uint8_t p2, void *data) noexcept
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

int32_t Query::request(const uint8_t cmd, const uint8_t rail, void *data) noexcept
{
std::lock_guard<std::mutex> guard(__mutex);

    switch (cmd)
    {
        case CMD_RAIL_VOLTS_HCRIT:
        case CMD_RAIL_VOLTS_LCRIT:
        case CMD_RAIL_AMPS_HCRIT:
        case CMD_RAIL_VOLTS:
        case CMD_RAIL_AMPS:
        case CMD_RAIL_WATTS:
            if (int32_t err = hidCmd(2, CMD_SELECT_RAIL, rail, NULL); err < 0)
                return err;
            break;
        default:
            break;
    }

    return hidCmd(3, cmd, 0, data);
}

int32_t Query::getValue(const uint8_t cmd, const uint8_t rail, int32_t *val) noexcept
{
    uint8_t data[ReplySize];
    uint32_t tmp;

    if (int32_t err = request(cmd, rail, data); err < 0)
        return err;

    tmp = (data[3] << 24) + (data[2] << 16) + (data[1] << 8) + data[0];
    switch (cmd)
    {
        case CMD_RAIL_VOLTS_HCRIT:
        case CMD_RAIL_VOLTS_LCRIT:
        case CMD_RAIL_AMPS_HCRIT:
        case CMD_TEMP_HCRIT:
        case CMD_IN_VOLTS:
        case CMD_RAIL_VOLTS:
        case CMD_RAIL_AMPS:
        case CMD_TEMP0:
        case CMD_TEMP1:
            *val = linearToInt(tmp & 0xFFFF, 1000);
            break;
        case CMD_FAN:
            *val = linearToInt(tmp & 0xFFFF, 1);
            break;
        case CMD_RAIL_WATTS:
        case CMD_TOTAL_WATTS:
            *val = linearToInt(tmp & 0xFFFF, 1000);
            break;
        case CMD_TOTAL_UPTIME:
        case CMD_UPTIME:
            *val = tmp;
            break;
        default:
            return -EOPNOTSUPP;
    }

    return 0;
}

int32_t Query::fwinfo() noexcept
{
    int32_t err = 0;

    if (err = hidCmd(3, CMD_VEND_STR, 0, _data->vendor.data()); err < 0)
        return err;
    if (err = hidCmd(3, CMD_PROD_STR, 0, _data->product.data()); err < 0)
        return err;

    return err;
}

void Query::criticals() noexcept
{
    int32_t tmp;

    for (int32_t rail = 0; rail < 2; ++rail)
    {
        if (!getValue(CMD_TEMP_HCRIT, rail, &tmp))
        {
            _data->temp_hcrit_support |= (1 << rail);
            _data->temp_hcrit[rail] = tmp / 1000.0;
        }
    }

    for (int32_t rail = 0; rail < 3; ++rail)
    {
        if (!getValue(CMD_RAIL_VOLTS_HCRIT, rail, &tmp))
        {
            _data->in_hcrit_support |= (1 << rail);
            _data->in_hcrit[rail] = tmp / 1000.0;
        }

        if (!getValue(CMD_RAIL_VOLTS_LCRIT, rail, &tmp))
        {
            _data->in_lcrit_support |= (1 << rail);
            _data->in_lcrit[rail] = tmp / 1000.0;
        }

        if (!getValue(CMD_RAIL_AMPS_HCRIT, rail, &tmp))
        {
            _data->curr_hcrit_support |= (1 << rail);
            _data->curr_hcrit[rail] = tmp / 1000.0;
        }
    }
}

bool Query::init() noexcept
{
    if (int32_t err = hidCmd(CMD_INIT, 0x03, 0x00); err >= 0)
    {
        if (err = fwinfo(); err < 0)
            return false;
        criticals();

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
