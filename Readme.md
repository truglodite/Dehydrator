# Dehydrator
An Arduino based adjustable precision bang bang relay controller for food dehydrators.

<img src="https://cdn.thingiverse.com/renders/fa/e7/e0/af/d5/63c64e5bdb93b1421403e6982178a298_preview_featured.jpg" width="600">

## 3D Printed Controller & Relay Enclosures
- *Presto 6302*: https://www.thingiverse.com/thing:2853628

## Features
- LCD interface w/ intuitive up/select/down button menus
- Filament heating time and temperature tables (editable in configuration.h)
- 'OFF', 'Select Filament', and 'Dry -> Holding' modes
- Separate relays for fan & heater
- Dallas & DHTXX sensors for decent precision with low cost
- Adjustable temperature hysteresis for heating mode
- Heater "ON" LED indicator
- Displays temp, set temp, humidity, and heater duty cycle % in dry and hold modes
- Obeys minimum switching times

## Hardware Used:
- Arduino pro mini (or similar m328p)
- DHT22 sensor
- Dallas temp sensor (optional)
- 1602 I2C LCD
- 2@ 5VDC/250VAC-20A Relay (for heater & fan, fan may use a lower amp relay)
- Food Dehydrator, 120VAC (Presto 6302 or similar)
- 3@ 6x6x6mm N.O. toggle buttons
- 3@ 4k7ohm resistors (button pullups)
- 5VDC supply (USB, wallwart, or similar)
- 0.22uF+220ohm RC snubber (Kemet P409CE224M275AH221 or similar)
- LED + limiting resistor (heater on indicator)

## Operation
Normal bootup goes in to off mode. Hitting select switches to filament select mode.

In filament select mode, the heater will remain off, while the LCD displays the filament name, drying temperature, & holding humidity. During filament select mode, up/down changes the selected filament, and select will go to drying mode.

While in drying mode the heater is hysteresis controlled to maintain the desired temperature for the configured elapsed time. The LCD displays hours until dry (eta), relative humidity %, actual/desired temp, and heater duty cycle %. The fan remains on always in drying mode. During dry mode, up/down will adjust the desired temperature, and select will switch to holding mode. If the timer counts to zero the code goes to holding mode.

Holding mode toggles the heater on/off using +/- hysteresis to maintain the set humidity level. The LCD displays "Hold", heating %, actual temp, and actual/desired humidity. The fan turns on when the heater is turned on, and it turns off a few seconds after heater turns off. The up/down buttons adjust the desired humidity %, and the select button will go to off mode. A safety check ensures the selected filament holding temperature is not exceeded in holding mode regardless of humidity values.

In all modes, an LED toggles on/off to indicate that the heater relay is ON. Some display modes show heater %. It is calculated as (on time)/(on time + off time). This percentage is updated each time the heater is turned either on or off.

### Adjusting Hysteresis (runtime)
During bootup, if select is held until the splash screen, the code will enter temperature hysteresis adjust mode. In this mode you can use up/down to modify the hysteresis used in dry mode. When done, push select to go to off mode. The modified hysteresis will persist until reboot.

## Notes
Since DHT sensors are prone to errors, this code implements an "NAN" value check on the dht sensor readings. By default if more than 5 consecutive NAN's are read, the code will soft reset.

* Warning: Mains Voltage Work Required!!!

Hacking a dehydrator requires you to be familiar with best practices while working around and designing high voltage devices. As such, this is not a suitable build for a non-skilled electrician. If you burn your house down or electrocute yourself using any ideas from this project, you're on your own. ;)

* HV wiring

It is recommended that you wire your high voltage lines like this:
```
                                       --[heater relay]--[heater]--
                                      |                            |
[LINE]-[thermal fuse]---[fan relay]---|------------[fan]-----------|---[NEUTRAL]
                                      |                            |
                                       ---------[RC snubber]-------
```
Wired this way, the fan relay must be activated for the heater to work. This is safer than wiring the fan relay in parallel to the heater relay.

Most appliances with mains powered appliances with heaters have a thermal fuse installed near the heating element. It is typically located and wired in a way that minimizes live wire length inside the device after it opens (short wire run indicated by the single "-"). For obvious safety reasons, do not bypass the fuse, and avoid increasing the length of 'hot wire' leading to it. Also, do not use solder on the thermal fuse, or anywhere near the heated areas of the appliance.

* RC snubber

For the Presto brand dehydrator used by the author of this code, and likely other brands as well, motor off switching transients result in occasional 'phantom button press' when the fan shut off. The behavior was verified with an o-scope, and an appropriate solution was tested; a 0.47uF+150ohm RC snubber in parallel to the motor wires. It is preferable to use a purpose built snubber device like the [Kemet P409CE474M275AH151](https://www.mouser.com/datasheet/2/212/KEM_F3089_P409_X2_275-1103726.pdf) for safety. Install it as close to the motor as reasonably possible. Accessing the motor on the author's Presto brand dehydrator was impossible without first removing the impeller and destroying it in the process. Instead, connecting the snubber in the relay/junction box was good enough to fix the problem.

## Installation

The files and directory structure intended for use with PlatformIO. The code is also compatible with Arduino IDE and others.
