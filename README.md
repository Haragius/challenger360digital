# Challenger 360 digital
A collection of tools for running the ASRock Challenger 360 Digital AIO liquid cooling system (the display) on Linux.
In theory, it's possible to map any readable sensor to the display. In practice, however, that doesn't make much sense.
Because of limitations in the drivers, it may be the case, for example, that no value can be calculated for the load.
Although the display of values works, the load monitoring feature has not yet been implemented.
I currently recommend using the CPU's sensors.

**Temperature from lm_sensor:**  
AMD - k10temp  
Intel - coretemp  

**RPM from lm_sensor:**  
SuperIO - nct6799 - fan4_input, for example  

**Clock Speed from sysfs:**  
CPU  

This is the first project I'm making public. It's also the first project in which I'm using CMake.  
I'd appreciate any suggestions for improvement.  

## Features
**challenger360digitalctl**

- Lists sensors that can be used
  - Displays a warning if the driver for the SuperIO chip is not loaded (it checks against:)
     - Nuvoton / Winbond "nct*"
     - ITE Tech "it87*"
     - Fintek "f718*"
     - Winbond "w836*"
     - Winbond "w837*"
     - SMSC "smsc*"
     - SMSC "sch5627*"
- Configuring Sensor Assignments (config)
- Sends values such as temperature, RPM, layout, etc., to the display
- Sends raw hex messages (for testing)
- Send the signal to the challenger360digitald daemon to re-read the config


**challenger360digitald**

- Reads the configured sensors from the configuration file, retrieves the current values, and sends them to the display.
- Rotates between the 3 possible layouts
- A static layout can be configured (if rotation = 0)


**libchallenger360digital**

A shared library.

## Upcoming Changes
- The procedure for handling the PID file needs to be changed. 
- A strategy for calculating the load. (Not every sensor provides the maximum value needed for the calculation)
- Implementation for displaying the load value.

## Build
```
git clone https://github.com/Haragius/challenger360digital.git
cd challenger360digital
cmake -S . -B build
cmake --build build
```

## Install
**Installation with CMake**
> [!NOTE]
> Installation into the root filesystem requires root privileges.
> You can install it anywhere to simply view the file structure if you use the --prefix parameter,
> but the udev rule and systemd.service are in the wrong place. The shared library and header file may not be found if you try to include them in your own project.
>
> To activate the udev rule, the rules must be reloaded or the PC must be restarted.
> To communicate with the display as a regular user, the user must be a member of the **plugdev** group or the udev rule needs to be modified.

**custom location**
```
cmake --install build --prefix ./dist
```
**root filesystem**
```
sudo cmake --install build
```

**Manual Installation**
> [!NOTE]
> To install manually, simply copy the following files to the appropriate location.
> Root privileges required.

> [!CAUTION]
> **Check the permissions and ownership!**
```
sudo cp ./build/challenger360digitalctl/challenger360digitalctl /usr/local/bin/
sudo cp ./build/challenger360digitald/challenger360digitald /usr/local/bin/
sudo cp ./build/libchallenger360digital/liblibchallenger360digital.so /usr/local/lib/
sudo cp ./udev/99-challenger360digital.rules /usr/local/lib/udev/rules.d/
sudo cp ./systemd/challenger360digital.service /usr/local/lib/systemd/system/
```

## Usage
> [!NOTE]
> You can use the daemon without systemd. However, if you use the daemon with systemd,
> make sure the path to the configuration file /etc/challenger360digital/challenger360digital.conf is correct.
> Save the configuration in the correct location or edit the challenger360digital.service file.

**First, let's get a general overview of the sensors**
```
challenger360digitalctl --sensors
```
**Second, generate the configuration file**
```
sudo challenger360digitalctl --config /etc/challenger360digital/challenger360digital.conf
```
**Start the daemon with Systemd**
```
sudo systemctl enable challenger360digital.service
sudo systemctl start challenger360digital.service
```
**Start the daemon in the shell**
> [!NOTE]
> Root privileges required because the PID file is hard-coded.

```
sudo challenger360digitald --config /path/to/challenger360digital.conf
```
