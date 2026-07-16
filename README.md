This is the main development repository for the corsair-psu hwmon driver which
is also part of mainline.

The `corsair-psu` driver supports Corsair RMi and HXi series power supplies.
The separate `corsair-psu-axi` USB driver supports the Corsair AX1600i. It
exposes input and rail voltage, current and power, temperature, fan speed and
mode, and labels through hwmon. The telemetry interfaces are read-only; fan
control is available through writable `pwm1` and `pwm1_enable` attributes.
It also exposes the AX1600i's twelve 12 V pages with per-page current, power,
and OCP limits. Session uptime and USB bridge metadata are available through
debugfs.

`pwm1_enable` follows the standard hwmon values: write `2` for the PSU's
automatic fan controller or `1` for manual control. In manual mode, `pwm1`
accepts values from 0 to 255 and converts them to the AX1600i's 0-100 percent
duty-cycle register. Switching to manual mode first sets the fan to 100 percent
so a stale low duty cycle cannot stop the fan unexpectedly; write the desired
`pwm1` value after enabling manual mode. A manual `pwm1` value of `0` stops the
fan, so automatic mode is recommended unless fixed-speed operation is needed.

The AX1600i transport and register protocol is based on the work in
[corsair-top](https://github.com/thad0ctor/corsair-top). The 12 V page
telemetry, fan control, uptime, and USB bridge metadata are based on protocol
research by [Jon0](https://github.com/Jon0) in
[Jon0/ax1600i](https://github.com/Jon0/ax1600i).

Build both modules with `make`. The AX1600i module can then be loaded with
`modprobe corsair-psu-axi` after installation, or `insmod corsair-psu-axi.ko`
for development testing.
