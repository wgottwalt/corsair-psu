This is the main development repository for the corsair-psu hwmon driver which
is also part of mainline.

The `corsair-psu` driver supports Corsair RMi and HXi series power supplies.
The separate `corsair-psu-axi` USB driver supports the Corsair AX1600i. The AXi
driver is read-only and exposes input and rail voltage, current and power,
temperature, fan speed and mode, and labels through hwmon. It also exposes the
AX1600i's twelve 12 V pages with per-page current, power, and OCP limits.
Session uptime and USB bridge metadata are available through debugfs.

The AX1600i transport and register protocol is based on the work in
[corsair-top](https://github.com/thad0ctor/corsair-top). The 12 V page
telemetry, fan mode, uptime, and USB bridge metadata are based on protocol
research by [Jon0](https://github.com/Jon0) in
[Jon0/ax1600i](https://github.com/Jon0/ax1600i).

Build both modules with `make`. The AX1600i module can then be loaded with
`modprobe corsair-psu-axi` after installation, or `insmod corsair-psu-axi.ko`
for development testing.
