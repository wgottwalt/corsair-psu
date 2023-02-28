#include <iomanip>
#include <iostream>
#include <sstream>
#include "Query.hxx"

using V = Query::Values;

int32_t main()
{
    Query *psu = nullptr;

    try
    {
        psu = new Query;
    }
    catch (std::exception &e)
    {
        std::cout << "error: " << e.what() << std::endl;
        return 1;
    }

    const auto asString = [&psu](const V sensor)
    {
        const auto [val, valid] = psu->value(sensor);
        std::stringstream ss;

        if (valid)
        {
            ss << std::setprecision(3) << std::fixed << val;

            return ss.str();
        }

        return ss.str();
    };

    const auto uptime = [&psu](const bool total)
    {
        const int32_t seconds_per_hour = 60 * 60;
        const int32_t seconds_per_day = seconds_per_hour * 24;
        const auto [val, valid] = psu->value(total ? V::TotalUptime : V::Uptime);
        const int32_t seconds = val;
        std::stringstream ss;

        if (valid)
        {
            if (val > seconds_per_day)
            {
                ss << std::setw(2) << std::setfill('0') << (seconds / seconds_per_day) << ":"
                   << std::setw(2) << std::setfill('0')
                       << (seconds % seconds_per_day / seconds_per_hour) << ":"
                   << std::setw(2) << std::setfill('0') << (seconds % seconds_per_hour / 60) << ":"
                   << std::setw(2) << std::setfill('0') << (seconds % 60);
            }
            else
            {
                ss << std::setw(2) << std::setfill('0')
                       << (seconds % seconds_per_day / seconds_per_hour) << ":"
                   << std::setw(2) << std::setfill('0') << (seconds % seconds_per_hour / 60) << ":"
                   << std::setw(2) << std::setfill('0') << (seconds % 60);
            }
        }

        return ss.str();
    };

    if (psu->valid())
    {
        std::cout << "--- " << psu->vendorName() << " " << psu->productName() << " (" << std::hex
                  << psu->vid() << ":" << psu->pid() << ") ---\n"
                  << "uptime:        " << uptime(false) << "\n"
                  << "total uptime:  " << uptime(true) << "\n"
                  << "vrm temp:      " << asString(V::Temp0) << "°C (max "
                      << asString(V::HighCritTemp0) << "°C)\n"
                  << "case temp:     " << asString(V::Temp1) << "°C (max "
                      << asString(V::HighCritTemp1) << "°C)\n"
                  << "fan:           " << asString(V::Fan) << "rpm\n"
                  << "v_in:          " << asString(V::VoltIn) << "V\n"
                  << "v_out 12v:     " << asString(V::Volt12v) << "V (min "
                      << asString(V::LowCritVolt12v) << "V max "
                      << asString(V::HighCritVolt12v) << "V)\n"
                  << "v_out 5v:      " << asString(V::Volt5v) << "V (min "
                      << asString(V::LowCritVolt5v) << "V max "
                      << asString(V::HighCritVolt5v) << "V)\n"
                  << "v_out 3.3v:    " << asString(V::Volt3v3) << "V (min "
                      << asString(V::LowCritVolt3v3) << "V max "
                      << asString(V::HighCritVolt3v3) << "V)\n"
                  << "curr_out 12v:  " << asString(V::Curr12v) << "A (max "
                      << asString(V::HighCritCurr12v) << "A)\n"
                  << "curr_out 5v:   " << asString(V::Curr5v) << "A (max "
                      << asString(V::HighCritCurr5v) << "A)\n"
                  << "curr_out 3.3v: " << asString(V::Curr3v3) << "A (max "
                      << asString(V::HighCritCurr3v3) << "A)\n"
                  << "power total:   " << asString(V::WattTotal) << "W\n"
                  << "power 12v:     " << asString(V::Watt12v) << "W\n"
                  << "power 5v:      " << asString(V::Watt5v) << "W\n"
                  << "power 3v3:     " << asString(V::Watt3v3) << "W\n"
                  << std::endl;
    }
    else
    {
        std::cout << "No matching USB device found... Does your user have the rights to access USB "
                  << "devices?\n"
                  << "You need a user with USB access rights or being root." << std::endl;
    }

    delete psu;

    return 0;
}
