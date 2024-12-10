# Cheap Crappy Constant Current Portable Programmable Power Supply 

`CCCCPPPS` is a DIY Programmable Buck/Boost Power Supply, designed with the goal
of being cheap, crappy, but very useful!  

![PCB_Front](/PCB/MkII/Front.png)  
![PCB_Back](/PCB/MkII/Back.png)  
![PCB_Schem](/PCB/MkII/Schematic.png)  

# Usage
## Web UI
https://bogdanthegeek.github.io/CCCCPPPS/software/webui/index.html
## Debug Interface:
This interface is enabled by default when building the firmware. Use `minichlink`
to issue commands listed below


## Commands
> [!NOTE]
> Need to improve the command list and handler

- `0` to turn the supply off
- `1-9` to set the target Voltage/Current in multiples of 1000mV/100mA
- `c` to switch to Constant Current Mode 
- `v` to switch to Constant Voltage Mode
- `+` to increase the target Voltage/Current depending on mode, in 50mV/25mA increments
- `-` to decrease the target Voltage/Current depending on mode, in 50mV/25mA increments
- ~~Konami code makes the unit self distruct.~~ Removed due to misuse.

# Build
```sh
export CH32V003FUN="path/to/ch32v003fun"
make
```

----
(c) 2024  
[Bogdan Ionescu](https://github.com/BogdanTheGeek)  
[ADBeta](https://github.com/ADBeta)  
