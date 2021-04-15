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
    static const int32_t TimeoutMS = 250;

    //--- public constructors ---
    Query() noexcept;
    Query(const Query &rhs) = delete;
    Query(Query &&rhs) = delete;
    ~Query() noexcept;

    //--- public operators ---
    Query &operator=(const Query &rhs) = delete;
    Query &operator=(Query &&rhs) = delete;

    //--- public methods ---
    bool valid() const noexcept;
    uint16_t vid() const noexcept;
    uint16_t pid() const noexcept;
    std::string vendorName() const noexcept;
    std::string productName() const noexcept;

protected:
    //--- protected methods ---
    bool init();
    void cleanup();
    int32_t cmd(const uint8_t p0, const uint8_t p1, const uint8_t p2, uint32_t *data = nullptr);

private:
    //--- private properties ---
    hid_device *_hid_dev;
    std::string _vname;
    std::string _pname;
    std::string _buffer;
    uint16_t _vid;
    uint16_t _pid;
    bool _valid;
    bool _init_failed;
};
