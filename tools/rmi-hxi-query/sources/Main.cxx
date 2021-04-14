#include <iostream>
#include "Query.hxx"

int32_t main()
{
    Query psu;

    if (psu.valid())
    {
        std::cout << "found: " << psu.vendorName() << " " << psu.productName() << " (" << std::hex
                  << psu.vid() << ":" << psu.pid() << ")" << std::endl;
    }
    else
    {
        std::cout << "No device found... Does your user have the rights to access usb device?\n"
                  << "If not, give the proper rights to the user or use root." << std::endl;
    }

    return 0;
}
