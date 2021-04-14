#include "Query.hxx"

//--- internal stuff ---

static const Query::USBDevice CorsairDevices[] = {
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

//--- public constructors ---

Query::Query()
: _hid_dev(nullptr), _vname(""), _pname(""), _vid(0), _pid(0), _valid(false)
{
    int32_t err = hid_init();

    if (err != 0)
        return;

    for (const auto &[vid, pid, vname, pname] : CorsairDevices)
    {
        if (_hid_dev = hid_open(vid, pid, nullptr); _hid_dev)
        {
            _vid = vid;
            _pid = pid;
            _vname = vname;
            _pname = pname;
            _valid = true;

            return;
        }
    }

    _hid_dev = nullptr;
}

Query::~Query()
{
    hid_close(_hid_dev);
    hid_exit();
}

//--- public methods ---

bool Query::valid() const
{
    return _valid && _hid_dev;
}

uint16_t Query::vid() const
{
    return _vid;
}

uint16_t Query::pid() const
{
    return _pid;
}

std::string Query::vendorName() const
{
    return _vname;
}

std::string Query::productName() const
{
    return _pname;
}
