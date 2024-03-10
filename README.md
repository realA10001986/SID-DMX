
**&#9888; Not for public release**

# Firmware for SID - DMX controlled

This repository holds a firmware for CircuitSetup's Status Indicator Display (SID) which allows control through DMX. It is designed to work using the [Sparkfun LED-to-DMX](https://www.sparkfun.com/products/15110) shield.

### DMX channels

<table>
    <tr><td>DMX channel</td><td>Function</td></tr>
    <tr><td>34</td><td>Brightness (0=off; 1-255=darkest-brightest)</td></tr>
    <tr><td>35</td><td>Auto-animate (1-255=lowest-highest=tt; 0=off, use ch36-45)</td></tr>
    <tr><td>36</td><td>Column 1 height</td></tr>
    <tr><td>37</td><td>Column 2 height</td></tr>
    <tr><td>38</td><td>Column 3 height</td></tr>
    <tr><td>39</td><td>Column 4 height</td></tr>
    <tr><td>40</td><td>Column 5 height</td></tr>
    <tr><td>41</td><td>Column 6 height</td></tr>
    <tr><td>42</td><td>Column 7 height</td></tr>
    <tr><td>43</td><td>Column 8 height</td></tr>
    <tr><td>44</td><td>Column 9 height</td></tr>
    <tr><td>45</td><td>Column 10 height</td></tr>
</table>

If DMX_USE_VERIFY is defined in sid_global.h (which it is by default), a DMX packet verifier is implemented. Channel 46 must be at value 100, otherwise the DMX packet is ignored.

### Build information

Requires [esp_dmx](https://github.com/someweisguy/esp_dmx) library v4.0.1 or later.

### Firmware update

To update the firmware without Arduino IDE/PlatformIO, copy a pre-compiled binary (filename must be "sidfw.bin") to a FAT32 formatted SD card, insert this card into the SID, and power up. The SID will show an egg timer while it updates its firmware. Afterwards it will reboot.

### Hardware: Pin mapping

The SID control board has a row of solder pads next to the ESP32 dev board. All below pins are accessible on this row of solder pads:

![SID](https://github.com/realA10001986/SID-DMX/assets/76924199/2a595c14-b8a1-4972-9907-6ba399776696)

The pin numbers listed below in SID column refer to above picture:

<table>
    <tr>
     <td align="center">SID</td><td align="center">LED-to-DMX shield</td>
    </tr>
    <tr>
     <td align="center">GPIO35 (pin 14)</a></td>
     <td align="center">J1 P14</td>
    </tr>
    <tr>
     <td align="center">GPIO14 (pin 8)</td>
     <td align="center">J1 P15</td>
    </tr>
    <tr>
     <td align="center">GPIO32 (pin 13)</td>
     <td align="center">J1 P16</td>
    </tr>
    <tr>
     <td align="center">3V3 (pin 19)</td>
     <td align="center">J1 P2</td>
    </tr>
    <tr>
     <td align="center">5V (pin 1)</td>
     <td align="center">J12 +</td>
    </tr>
    <tr>
     <td align="center">GND (pin 6)</td>
     <td align="center">J12 -</td>
    </tr>
 </table>

![DMXshield](https://github.com/realA10001986/SID-DMX/assets/76924199/1783dbd9-4378-41cf-90a9-2809d8f59a17)

Some some mysterious reason, the SID sometimes experiences transmission errors which go away when an SD card is in the SID's card slot.
