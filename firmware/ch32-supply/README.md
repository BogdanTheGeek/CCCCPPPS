
# About
> We have `programable power supply` at home.

# Usage
## Debug Interface:
This interface is enabled by default when building the firmware
## USB Interface:
Build with `-DCONFIG_USE_USB`
> [!NOTE] To be implemented.

## Commands
 - `0` to turn the supply off
 - `c` and `v` to switch between constant current and constant voltage
 - `+` or `=` to increse the target voltage/current in 50mV/25mA increments
 - `1-9` to quickly set the voltage/current target in multiples of 1000mV/100mA
 - ~~Konami code makes the unit self distruct.~~ Feature removed due to misuse.

# Build
```sh
export CH32V003FUN="path/to/ch32v003fun"
make
```
