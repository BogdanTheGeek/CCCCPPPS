# Cheap Crappy Constant Current Portable Programmable Power Supply 

`CCCCPPPS` is a DIY Programmable Buck/Boost Power Supply capable of  
* Max Output Voltage  16V
* TBD

# Usage
## Debug Interface:
This interface is enabled by default when building the firmware. Use `minichlink`
to issue commands listed below

## USB Interface:
Build with `-DCONFIG_USE_USB`
> [!NOTE]
> To be implemented.

## Commands
> [!NOTE]
> Need to improve the command list and handler

- `0` to turn the supply off
- `1-9` to set the target Voltage/Current in multiples of 1000mV/100mA
- `c` to switch to Constant Current Mode 
- `v` to switch to Constant Voltage Mode
- `+` to increase the target Voltage/Current depending on mode, in 50mV/25mA increments
- ~~Konami code makes the unit self distruct.~~ Removed due to misuse.

# Build
```sh
export CH32V003FUN="path/to/ch32v003fun"
make
```

----
(c) 2024  
Bogdan Ionescu  
ADBeta  
