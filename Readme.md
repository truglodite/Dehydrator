# Dehydrator
<img src="https://cdn.thingiverse.com/renders/fa/e7/e0/af/d5/63c64e5bdb93b1421403e6982178a298_preview_featured.jpg" width="900">
A bang bang relay controller for food dehydrators, with 3 button LCD interface, separate fan relay control, time/temp tables, and drying/holding modes.

## 3D Printed Controller & Relay Enclosures
- *Presto 6302*: https://www.thingiverse.com/thing:2853628

## Hardware Used:
- Arduino Uno (or similar m328p)
- DHT22 sensor
- Dallas temp sensor (optional)
- 1602 I2C LCD
- 2@ 5VDC/250VAC-20A Relay (for heater & fan, fan may use a lower amp relay)
- Food Dehydrator, 120VAC (Presto 6302 or similar)
- 3@ 6x6x6mm N.O. toggle buttons
- 5VDC supply (USB, wallwart, or similar)

## Operation
Normal bootup goes in to off mode. Hitting select switches to filament select mode.

In filament select mode, the heater will remain off, while the LCD displays the filament name, drying temperature, & holding humidity. During filament select mode, up/down changes the selected filament, and select will go to drying mode.

While in drying mode the heater is hysteresis controlled to maintain the desired temperature for the configured elapsed time. The LCD displays hours until dry (eta), relative humidity %, actual/desired temp, and heater %. The fan remains on always in drying mode. During dry mode, up/down will adjust the desired temperature, and select will switch to holding mode. If the timer counts to zero the code goes to holding mode.

In to holding mode, which toggles the heater on/off using +/- hysteresis to maintain the set humidity level. In holding mode, the LCD displays "Hold", heating %,
actual temp, and actual/desired humidity. The fan turns on when the heater is turned on, and it turns off a few seconds after heater turns off. During holding mode, the up/down buttons adjust the desired humidity %, and the select button will go to off mode. A safety check ensures the selected filament holding temperature is not exceeded in holding mode regardless of humidity values.

In all modes, the LED toggles on/off to indicate power going to the heater. On displays that show heater %, heater % is calculated as (on time)/(on time + off time). This percentage is updated each time the heater is turned either on or off.

### Adjusting Hysteresis (runtime)
During bootup, if select is held until the splash screen, the code will enter temperature hysteresis adjust mode. In this mode you can use up/down to modify the hysteresis used in dry mode. When done, push select to go to off mode. The modified hysteresis will persist until reboot.

## Notes
Since DHT sensors are prone to errors, this code implements an "NAN" value check on the dht sensor readings. By default if more than 5 consecutive NAN's are read, the code will soft reset.

### Warning: Line Voltage Work Required
Hacking a dehydrator requires you to be familiar with best practices while working around and designing high voltage devices. As such, this is not a suitable build for a non-skilled electrician. It is recommended that you wire your relays like this:

                            ----[heater relay]----[heater]----
                           |                                  |
  [LINE]----[fan relay]----|-------------[fan]----------------|----[NEUTRAL]

This way the fan relay must be activated for the heater to work, regardless of the combination signals coming from the Arduino board. Regardless how it is wired, you assume all liabilities by with building and operating this device... ie if you burn your house down or electrocute yourself by building and/or operating a dehydrator, you're on your own. ;)
