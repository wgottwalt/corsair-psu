#pragma once

#include <array>
#include <string>
#include <tuple>
#include <hidapi/hidapi.h>

struct Data;

class Query {
public:
    //--- public types and constants ---
    using Result = std::tuple<float,bool>;
    enum class Values {
        HighCritTemp0, HighCritTemp1,
        HighCritCurr12v, HighCritCurr5v, HighCritCurr3v3,
        HighCritVolt12v, HighCritVolt5v, HighCritVolt3v3,
        LowCritVolt12v, LowCritVolt5v, LowCritVolt3v3,
        Temp0, Temp1,
        Curr12v, Curr5v, Curr3v3,
        Volt12v, Volt5v, Volt3v3,
        Watt12v, Watt5v, Watt3v3,
        VoltIn,
        Fan,
        Uptime, TotalUptime
    };
    struct USBDevice {
        const uint16_t vid;
        const uint16_t pid;
    };
    static const size_t BufSize = 64;
    static const size_t ReplySize = 16;
    static const int32_t TimeoutMS = 250;

    //--- public constructors ---
    Query() noexcept;
    Query(const Query &rhs) noexcept(false) = delete;
    Query(Query &&rhs) noexcept = delete;
    ~Query() noexcept;

    //--- public operators ---
    Query &operator=(const Query &rhs) noexcept(false) = delete;
    Query &operator=(Query &&rhs) noexcept = delete;

    //--- public methods ---
    bool valid() const noexcept;
    uint16_t vid() const noexcept;
    uint16_t pid() const noexcept;
    std::string vendorName() const noexcept;
    std::string productName() const noexcept;
    Result value(const Values val) noexcept;

protected:
    //--- protected methods ---
    int32_t linearToInt(const uint16_t val, const int32_t scale) const noexcept;
    int32_t hidCmd(const uint8_t p0, const uint8_t p1, const uint8_t p2, void *data = nullptr)
        noexcept;
    int32_t request(const uint8_t cmd, const uint8_t rail, void *data) noexcept;
    int32_t getValue(const uint8_t cmd, const uint8_t rail, int32_t *val) noexcept;
    int32_t fwinfo() noexcept;
    void criticals() noexcept;
    bool init() noexcept;
    void cleanup() noexcept;

private:
    //--- private properties ---
    hid_device *_hid_dev;
    Data *_data;
    std::array<uint8_t,BufSize> _buffer;
    uint16_t _vid;
    uint16_t _pid;
    bool _valid;
    bool _init_failed;
};
