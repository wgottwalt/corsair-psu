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
        std::cout << "vrm temp:      (max " << asString(V::HighCritTemp0) << "°C)\n"
                  << "case temp:     (max " << asString(V::HighCritTemp1) << "°C)\n"
                  << "v_out 12v:     (min " << asString(V::LowCritVolt12v) << "V max "
                      << asString(V::HighCritVolt12v) << "V)\n"
                  << "v_out 5v:      (min " << asString(V::LowCritVolt5v) << "V max "
                      << asString(V::HighCritVolt5v) << "V)\n"
                  << "v_out 3.3v:    (min " << asString(V::LowCritVolt3v3) << "V max "
                      << asString(V::HighCritVolt3v3) << "V)\n"
                  << "curr_out 12v:  (max " << asString(V::HighCritCurr12v) << "A)\n"
                  << "curr_out 5v:   (max " << asString(V::HighCritCurr5v) << "A)\n"
                  << "curr_out 3.3v: (max " << asString(V::HighCritCurr3v3) << "A)\n";
    }
    else
    {
        std::cout << "No device found... Does your user have the rights to access usb device?\n"
                  << "If not, give the proper rights to the user or use root." << std::endl;
    }

    return 0;
}
