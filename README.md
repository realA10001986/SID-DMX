
# **&#9888; Not for public release**

Requires "esp_dmx" library (someweisguy) v4.0.1
(Ignore compiler warnings)

Pin mapping:
<table>
    <tr>
     <td align="center">SID</td><td align="center">LED-to-DMX shield</td>
    </tr>
    <tr>
     <td align="center">GPIO35</a></td>
     <td align="center">J1 P14</td>
    </tr>
    <tr>
     <td align="center">GPIO14</td>
     <td align="center">J1 P15</td>
    </tr>
    <tr>
     <td align="center">GPIO32</td>
     <td align="center">J1 P16</td>
    </tr>
  <tr>
     <td align="center">3V3 (screw terminal)</td>
     <td align="center">J1 P2</td>
    </tr>
 </table>
 
  J12: 
  - 5V from ESP32 dev board pin 
  - GND from any of the screw terminals


Some some mysterious reason, the SID sometimes experiences transmission errors which go away when an SD card is in the SID's card slot.
