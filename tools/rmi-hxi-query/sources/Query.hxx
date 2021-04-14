#pragma once

#include <string>
#include <hidapi/hidapi.h>

class Query {
public:
    //--- public types and constants ---
    struct USBDevice {
        const uint16_t vid;
        const uint16_t pid;
        const std::string vname;
        const std::string pname;
    };
    static const size_t BufSize = 64;

    //--- public constructors ---
    Query();
    Query(const Query &rhs) = delete;
    Query(Query &&rhs) = delete;
    ~Query();

    //--- public operators ---
    Query &operator=(const Query &rhs) = delete;
    Query &operator=(Query &&rhs) = delete;

    //--- public methods ---
    bool valid() const;
    uint16_t vid() const;
    uint16_t pid() const;
    std::string vendorName() const;
    std::string productName() const;

private:
    //--- private properties ---
    hid_device *_hid_dev;
    std::string _vname;
    std::string _pname;
    uint16_t _vid;
    uint16_t _pid;
    bool _valid;
};
