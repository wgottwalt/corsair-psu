This is the main development repository for the corsair-psu hwmon driver which
is also part of mainline.

The `corsair-psu` driver supports Corsair RMi and HXi series power supplies.
The separate `corsair-psu-axi` USB driver supports the Corsair AX1600i. The AXi
driver is read-only and exposes input and rail voltage, current and power,
temperature, fan speed, and labels through hwmon.

The AX1600i transport and register protocol is based on the work in
[corsair-top](https://github.com/thad0ctor/corsair-top).

Build both modules with `make`. The AX1600i module can then be loaded with
`modprobe corsair-psu-axi` after installation, or `insmod corsair-psu-axi.ko`
for development testing.
