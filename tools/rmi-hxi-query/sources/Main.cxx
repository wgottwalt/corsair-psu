#include <iomanip>
#include <iostream>
#include <sstream>
#include "Query.hxx"

using V = Query::Values;

int32_t main()
{
    Query psu;

    const auto asString = [&psu](const V sensor)
    {
        const auto [val, valid] = psu.value(sensor);
        std::stringstream ss;

        if (valid)
        {
            ss << std::setprecision(3) << std::fixed << val;

            return ss.str();
        }

        return ss.str();
    };

    if (psu.valid())
    {
        std::cout << "--- " << psu.vendorName() << " " << psu.productName() << " (" << std::hex
                  << psu.vid() << ":" << psu.pid() << ") ---" << std::endl;
        std::cout << "vrm temp:      " << asString(V::Temp0) << "°C (max "
                      << asString(V::HighCritTemp0) << "°C)\n"
                  << "case temp:     " << asString(V::Temp1) << "°C (max "
                      << asString(V::HighCritTemp1) << "°C)\n"
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
                  << "power 12v:     " << asString(V::Watt12v) << "W\n"
                  << "power 5v:      " << asString(V::Watt5v) << "W\n"
                  << "power 3v3:     " << asString(V::Watt3v3) << "W\n"
                  << std::endl;
    }
    else
    {
        std::cout << "No device found... Does your user have the rights to access usb device?\n"
                  << "If not, give the proper rights to the user or use root." << std::endl;
    }

    return 0;
}
