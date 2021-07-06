This is a simple USB HID query tool for the Corsair RMi/HXi series power
supplies and tries to replicate exactly the information the driver provides.
It does so by using libhidapi (based on libusb) to access the device
directly by known USB IDs. There is no need to load the driver at all.

The tool is written using C++20 and the meta buildsystem cmake. It also
provides a cmake find script for libhidapi, so this one needs to be
installed in your Linux distribution.
