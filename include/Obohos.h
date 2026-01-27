#pragma once

/* GPIOs
		7, 			// SPI SS
		8, 			// CE (standby)
		9, 			// TRX (RX/TX mode)
		10, 		// PWR (power down)
		3, 			// CD (collision avoid)
		2, 			// DR (data ready)
		1, 			// AM (address match)
*/

#include "secrets.h"
#include <arduino.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson 
#include "time.h"

#include <nRF905.h>
#include <SPI.h>

#define CHANNEL 0x62
#define RXADDR  0x40060006 // Address of this device
#define TXADDR  0x40060006 // Address of device to send to
#define PAYLOAD_SIZE 2
#define COMMAND_ON 8
#define COMMAND_OFF 4

nRF905 transceiver = nRF905(SPI);

#define LED_PIN 8
#define LED_ON 0         // LED polarity. 1 is active-high. can be changed if LED is inverted
#define BEEPER 20
#define BEEP_TIME_MS 900   // ms
#define BEEP_SEQUENCE_MS	15000
#define SWITCH_OFF_TIME_MINUTES	60
#define SWITCH_OFF_TIME_MS 20000 //SWITCH_OFF_TIME_MINUTES * 60 * 1000 //milliseconds

#define ON true
#define OFF false

#define DEVICE_NAME "Obohos"  
#define DNS_NAME   DEVICE_NAME
#define MQTT_SWITCH_SET   DEVICE_NAME "/switch/set"
#define MQTT_SWITCH_STATE DEVICE_NAME "/switch/state"
#define MQTT_STATE DEVICE_NAME "/state"

#define LOOP_INTERVAL   100
#define PACKET_NONE		0
#define PACKET_OK		1
#define PACKET_INVALID 2


// WiFi connection
typedef struct  // for table of available  wireless networks
{
  const char *ssid;
  const char *password;
} Networks;

Networks nets[] = { { SSID1, SSID1_PW },  // table of available SSIDs
                    { SSID2, SSID2_PW } };
int bestNet = 0;

// Zeitzone-String für Deutschland (automatisch Sommer/Winterzeit)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0; // UTC (wird durch Zeitzonen-String überschrieben)
const int daylightOffset_sec = 0; // Wird ignoriert wenn Zeitzonen-String verwendet wird
// Posix Zeitzonen-String für Deutschland (automatische DST-Umstellung)
const char* timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";

// MQTT
const char *deviceName    = DEVICE_NAME;
const char *mqtt_server   = "homeassistant";   //"homeassistant.local";        // network address of MQTT server
const char *mqtt_user     = MQTT_USER;         // MQTT Username here
const char *mqtt_password = MQTT_PW;           // MQTT Password here
WiFiClient espClient;
PubSubClient MQTTclient(espClient);

/* Globals */
String version;
bool lightState = OFF;
unsigned long startTime;

void beep(int duration);
void getBestWifi();
void getNTPtime();
void MQTTreconnect();
void MQTTsendDiscover();
void MQTTsendBlock();
void MQTTcallback(char* topic, byte* payload, unsigned int length) ;
void checkTimeout(void);
void obohosLight(bool lightCommandOn);
void nRF905_onRxComplete(nRF905* device);
void nRF905_onRxInvalid(nRF905* device);
void nRF905_int_dr();
void nRF905_int_am();