# Obohos-HS6-control

Control of Obohos HS-6 crane remote by ESP32C3 Super Mini controller and NRF905 Wireless Transceiver Modul (Aliexpress)<br>
Uses nRF905 Radio Library for Arduino (by Zak Kemble)<br>
Copyright: (C) 2026 by Uli Huber<br>
License: GNU GPL v3 

<h2> Features:</h2> 

- Arduino based, should work on any ESP32 with WiFi

- Tested only for remotes in the 433MHz band

- Connects to best SSID from provided list

- Interfaces with HomeAssistant by MQTT discovery

- Allows concurrent switching of power from OBOHOS HS-6 remote and HomeAssistant

- Provides automatic power down after timeout (to avoid leaving it switched on...)

- Provides network time for debug and other purposes
  
- Bonus: Added Python script to calculate frequency/channel and address from OBOHOS serial number

<h2>Serial-Number </h2>
The serial number of the HS-6 remotes has coded information about the used channel and address. <BR>
The information below is the result of analysing the internal protocol of three HS-6 remotes and therefore empiric. It might not fit to all remotes. There are older serial numbers with a 2-digit frequency code that do not match. 

<h5>Example:  710C334</h5>

710:&nbsp;&nbsp;&nbsp;&nbsp;lower part of 4 dig hex address (assume 0710)<br>
C:  &nbsp;&nbsp;&nbsp;&nbsp;unknown, Separator?<br>
3:  &nbsp;&nbsp;&nbsp;&nbsp;unknown<br>
34: &nbsp;&nbsp;&nbsp;&nbsp;channel indicator.  channel=int(((430.3 + 34/10)-422.4)\*10) = 0x71<br>

The 32 bit hex address is built from first 3 or 4 hex digits and a suffix "0006".
Because of LSB reversal it is 0x10070006<br>

<h5>Address and frequency calculator</h5>
Obohos.py is a simple calculator for address and channel.
If the calculation fails, the only way to get the information is tapping the PIC16F716, where the SPI-Protocol to the NRF905 is handled.<br>

- Pin 1: &nbsp;&nbsp;&nbsp;&nbsp;MISO<br>
- Pin2: &nbsp;&nbsp;&nbsp;&nbsp;MOSI<br>
- Pin19:&nbsp;&nbsp;&nbsp;&nbsp;SCK<br>
- Pin20:&nbsp;&nbsp;&nbsp;&nbsp; SS<br>

I sniffered the data with PulseView, which has a nice decoder for the NRF905.<br>

<h2>Codes</h2>
The HS-06 remote uses following 2 byte codes:<br>

- 0x0001  Down<br>
- 0x0002  Up<br>
- 0x0004  Off<br>
- 0x0008  On<br>
- 0x0010  Right<br>
- 0x0080  Left<br>

<h2>Basic communication setting</h2>
The remotes all have a 2 byte payload size, 32-bit addresses and 8-bit CRC.<br>
The NRF905 chip ignores all communications with not matching payload size and/or wrong address and/or wrong CRC.<br>
Of course the frequency/channel must match too.<br>

<h2>Necessary configurations</h2>
nRF905\_config.h in library needs not to be edited, as all specifics are dealt with in the main program<br>
<h4>Defines for the actual remote and local settings are in Obohos.h: </h4>
#define DEVICE\_NAME "Obohos"<br>
and<br>
#define CHANNEL 0x71<br>
#define RXADDR  0x10070006 &nbsp;&nbsp;&nbsp;&nbsp;// Address of this device<br>
#define TXADDR  0x10070006 &nbsp;&nbsp;&nbsp;&nbsp;// Address of device to send to<br>
and:<br>
const char\* timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";<br>

<h4>Private credentials need to be completed in includes/secrets.h.example and saved as includes/secrets.h :</h4>
const char \*mqtt\_server   = "homeassistant"; &nbsp;&nbsp;&nbsp;&nbsp;//"homeassistant.local" network address of MQTT server<br>
const char \*mqtt\_user     = "user";          &nbsp;&nbsp;&nbsp;&nbsp; &nbsp;&nbsp;&nbsp;&nbsp;  // MQTT Username here<br>
const char \*mqtt\_password = "password";     &nbsp;&nbsp;&nbsp;&nbsp;   // MQTT Password here<br>
Networks nets\[] = { { "SSID1", "password1" },{ "SSID2", "password2" } }; &nbsp;&nbsp;&nbsp;&nbsp; // table of available SSIDs, can be more than two<br>

<h2>Schematic</h2>
This is just an example for the ESP32C3 Super mini. Any other ESP32 will do.<br>
![ESP32C3 Super Mini - NRF905](./images/Obohos_schematic.JPG)










