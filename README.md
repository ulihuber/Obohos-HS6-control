# Obohos-HS6-control

Control of Obohos HS-6 crane remote <br>
Project: Crane remote interface for nRF905 based OBOHOS HS-6 Systems<br>
Uses nRF905 Radio Library for Arduino (by Zak Kemble)<br>
Copyright: (C) 2026 by Uli Huber<br>
License: GNU GPL v3 

<h2> Features:</h2>

- Added Python script to calculate frequency/channel and address from OBOHOS serial number

- Tested only for remotes in the 433MHz band

- Connects to best SSID from provided list

- Interfaces with HomeAssistant by MQTT discovery

- Allows concurrent switching of power from OBOHOS HS-6 remote and HomeAssistant

- Provides automatic power down after timeout (to avoid leaving it switched on...)

- Provides network time for debug and other purposes

<h2>Serial-Number </h2>
The serial number of the HS-6 remotes has coded information about the used channel 

and address. The information below is the result of analysing the internal protocol of three HS-6 remotes and therefore empiric. It might not fit to all remotes. There are older serial numbers with a 2-digit frequency code that do not match. 

Example:  710C334

710: 	lower part of 4 dig hex address (assume 0710)<br>
C:		unknown, Separator?<br>
3:    unknown<br>
34: 	channel indicator.  channel=int(((430.3 + 34/10)-422.4)\*10) = 0x71<br>

The 32 bit hex address is built from first 3 or 4 hex digits and a suffix "0006".
Because of LSB reversal it is 0x10070006



Obohos.py is a simple calculator for address and channel.
If the calculation fails, the only way to get the information is tapping the PIC16F716, where the SPI-Protocol to the NRF905 is handled.<br>

Pin 1 : MISO<br>
Pin2  : MOSI<br>
Pin19 : SCK<br>
Pin20 : SS<br>
I sniffered the data with PulseView, which has a nice decoder for the NRF905.<br>

<h2>Basic configuration</h2>
The remotes all have a 2 byte payload size, 32-bit addresses and 8-bit CRC.<br>
The NRF905 chip ignores all communications with not matching payload size and/or wrong address and/or wrong CRC.<br>
Of course the frequency/channel must match too.<br>
The other configurations in the "nRF905 Radio Library" can remain unchanged<br>

<h2>Necessary configurations</h2>
nRF905\_config.h in library needs not to be edited
Defines for the actual remote and local settings are in Obohos.h:

\#define CHANNEL 0x71<br>
\#define RXADDR  0x40060006 // Address of this device<br>
\#define TXADDR  0x40060006 // Address of device to send to<br>
and<br>
\#define DEVICE\_NAME "Obohos"<br>

const char \*mqtt\_server   = "homeassistant";   //"homeassistant.local" network address of MQTT server<br>
const char \*mqtt\_user     = "user";            // MQTT Username here<br>
const char \*mqtt\_password = "password";        // MQTT Password here<br>

const char\* timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";<br>

Networks nets\[] = { { "SSID1", "password1" },  // table of available SSIDs<br>
                     { "SSID2", "password2" } };<br>







