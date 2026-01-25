/*
 * Project: Crane remote interface for nRF905 based OBOHOS HS-6 Systems
 * Uses nRF905 Radio Library for Arduino (by Zak Kemble)
 * Copyright: (C) 2026 by Uli Huber
 * License: GNU GPL v3 (see License.txt)
 */

 #include "ObohosOff.h"

static volatile uint8_t packetStatus;

// Don't modify these 2 functions. They just pass the DR/AM interrupt to the correct nRF905 instance.
void nRF905_int_dr(){transceiver.interrupt_dr();}
void nRF905_int_am(){transceiver.interrupt_am();}

// Event function for RX complete
void nRF905_onRxComplete(nRF905* device)
  {
	packetStatus = PACKET_OK;
	transceiver.standby();
  }

// Event function for RX invalid
void nRF905_onRxInvalid(nRF905* device)
  {
	packetStatus = PACKET_INVALID;
	transceiver.standby();
  }

void setup()
  {
  Serial.begin(115200);
	//while(!Serial.available());
  delay(5000);
  Serial.printf("\n\nObohos - U.Huber 2026\n");
	Serial.println("Build: " __TIME__ "  " __DATE__);
	Serial.println(__FILE__);
	char *p1;
	//for (char *p = __FILE__; *p != 0; p++)
    //	if (*p == '\\') p1 = p + 1;
	version = String(DEVICE_NAME) + " " + String(__DATE__) + String("  ") + String(__TIME__);
	pinMode(LED_PIN, GPIO_MODE_INPUT_OUTPUT);  // for easy inverting
	digitalWrite(LED_PIN, LED_ON);

  getBestWifi();
  getNTPtime();

	Serial.println("Init MQTT");   
	MQTTclient.setServer(mqtt_server, 1883);
	MQTTclient.setBufferSize(512);  // discovery packet is bigger than 256
	MQTTsendDiscover();
	MQTTclient.setCallback(callback);
	MQTTclient.subscribe(MQTT_SWITCH_SET);
	MQTTclient.publish(MQTT_SWITCH_STATE, "OFF", true);

	Serial.println("Init OTA");
	ArduinoOTA.setPassword("uhu");
	ArduinoOTA.setHostname(DEVICE_NAME);
	ArduinoOTA.begin();


	transceiver.begin(
		10000000, // SPI Clock speed (10MHz)
		7, // SPI SS
		0, // CE (standby)
		9, // TRX (RX/TX mode)
		10, // PWR (power down)
		3, // CD (collision avoid)
		2, // DR (data ready)
		1, // AM (address match)
		nRF905_int_dr, // Interrupt function for DR
		nRF905_int_am // Interrupt function for AM
	  );
	
	// Register event functions
	transceiver.events(
		nRF905_onRxComplete,
		nRF905_onRxInvalid,
		NULL,
		NULL
	  );
	
	// Set address of this device
	transceiver.setListenAddress(RXADDR);
  transceiver.setPayloadSize(PAYLOAD_SIZE,PAYLOAD_SIZE);
  transceiver.setChannel(CHANNEL);


	// Put into receive mode
	transceiver.RX();

	Serial.println(F("Server started"));
	byte regs[NRF905_REGISTER_COUNT];
	transceiver.getConfigRegisters(regs);
	Serial.print(F("Raw: "));
	byte dataInvalid = 0;
	for(byte i=0;i<NRF905_REGISTER_COUNT;i++)
	  Serial.printf("%02x ",regs[i]);
	  
  Serial.printf("\nPayloadsize: %d\n",PAYLOAD_SIZE);

  MQTTsendBlock();
  Serial.println("Startup complete!\n");
  Serial.println(F("Waiting for remote or HomeAssistant..."));
  }

/******************************************************************/

void loop()
  {
	// Wait for data
	while(packetStatus == PACKET_NONE)
  	{
		MQTTclient.loop();
  	ArduinoOTA.handle();
    if ( ((millis() - startTime)/100) % 10 == 0 && lightState == ON) Serial.printf(" %2d s\r", (SWITCH_OFF_TIME - millis() + startTime)/1000);

    if (((millis() - startTime) > SWITCH_OFF_TIME) && lightState == ON) 
      {
      digitalWrite(LED_PIN, !LED_ON); 
	    Serial.println("Licht aus!");
	    MQTTclient.publish(MQTT_SWITCH_STATE, "OFF", true);
	    obohosLight(OFF);
	    lightState = OFF;
      }
		delay(10); //LOOP_INTERVAL);
		}

	if(packetStatus != PACKET_OK)
	  {
		packetStatus = PACKET_NONE;
		Serial.println(F("Invalid packet!"));
		transceiver.RX();
	  }
	else
	  {
		packetStatus = PACKET_NONE;
    startTime = millis();
		// Make buffer for data and read payload
		uint8_t buffer[PAYLOAD_SIZE];
		transceiver.read(buffer, sizeof(buffer));

    if (buffer[0] == 4)
      {
      digitalWrite(LED_PIN, !LED_ON); 
	    if (lightState == ON) 
        {
        Serial.println("Licht aus!");
        MQTTclient.publish(MQTT_SWITCH_STATE, "OFF", true);
        }
      lightState = OFF;
      } 
    else if (buffer[0] == 8)
      {
      digitalWrite(LED_PIN, LED_ON); 
	    if (lightState == OFF) 
        {
        Serial.println("Licht an!");
        MQTTclient.publish(MQTT_SWITCH_STATE, "ON", true);
        }
      lightState = ON;
      } 

		transceiver.RX();

	  }
  }


void obohosLight(bool lightCommandOn)
	{
  uint8_t sendBuffer[PAYLOAD_SIZE];
	//memset(sendBuffer, 0, sizeof(sendBuffer)); 
	if (lightCommandOn) sendBuffer[0] = 8;
	else 			          sendBuffer[0] = 4;
	sendBuffer[1] = 0;
	transceiver.write(TXADDR, sendBuffer, sizeof(sendBuffer)); 	// Write reply data and destination address to radio
	while(!transceiver.TX(NRF905_NEXTMODE_TX, true)); 				// Send the reply data, once the transmission has completed go into receive mode
	delay(10);
	sendBuffer[0] = 0;
	sendBuffer[1] = 0;
	transceiver.write(TXADDR, sendBuffer, sizeof(sendBuffer));
	while(!transceiver.TX(NRF905_NEXTMODE_RX, true));
	}


//****************************************************************
// MQTT routines

void MQTTreconnect() 
{
  // Init MQTT
  int attempts = 0;
  while (!MQTTclient.connected() && (attempts++ < 3))  // Loop if we're not reconnected
  {
    Serial.println("Attempting MQTT connection...");
    if (MQTTclient.connect(deviceName, mqtt_user, mqtt_password)) 
    {
      Serial.println("connected");
    } 
    else 
    {
      //Serial.printf("MQTT connect failed, rc= %s\n", MQTTclient.state());
      Serial.println("MQTT try again in 5 seconds\n");
      if (WiFi.status() != WL_CONNECTED) getBestWifi() ;
      delay(2000);  // Wait 5 seconds before retrying
    }
  }
  if (!MQTTclient.connected())
  {
    Serial.printf("No MQTT server\n");
  }
}

void sendMQTTDiscoveryMsg(String topic, String topic_type,  String unit, String entityClass, String iconString, String yamlTemplate) 
{
  String discoveryTopic = String("homeassistant/") 
                          + topic_type + "/"
                          + DEVICE_NAME + "/" 
                          + topic 
                          + "/config";
  JsonDocument doc;
  doc["name"] = topic;  //String(initData.sensorName) + "_" + topic;
  doc["unique_id"] = String(DEVICE_NAME) + "_" + topic;
  doc["force_update"] = true;
  doc["enabled_by_default"] = true;
  if (iconString.length() > 0)   doc["icon"] = iconString;
  if (topic_type == "sensor")
    {
    doc["state_topic"] = String(DEVICE_NAME) +"/state";
    if (unit != "") doc["unit_of_measurement"] = unit;
    if (entityClass != "") doc["device_class"] = entityClass;
    doc["value_template"] = yamlTemplate;
    }
  else if (topic_type == "switch")
    { 
    doc["state_topic"] = String(MQTT_SWITCH_STATE);
    doc["command_topic"] = String(MQTT_SWITCH_SET);
    doc["payload_on"] = String("ON");
    doc["payload_off"] = String("OFF");
    }
  JsonObject device = doc["device"].to<JsonObject>();
  doc["enabled_by_default"] = true;
  device["manufacturer"] = "UHU";
  device["model"] = version;
  device["name"] = String(DEVICE_NAME);
  device["identifiers"][0] = String(DEVICE_NAME);  //WiFi.macAddress();

  char output[512];
  doc.shrinkToFit();  // optional
  int n = serializeJson(doc, output);

  MQTTreconnect();
  if (!MQTTclient.publish(discoveryTopic.c_str(), (const uint8_t*)output, n, true)) 
     Serial.printf("MQTT publish discovery - Fehler!\n");
  delay(200);
}

void MQTTsendDiscover() 
  {
  sendMQTTDiscoveryMsg("Time",         "sensor", "",   "",            "mdi:clock-outline", "{{ value_json.time }}");
  sendMQTTDiscoveryMsg("IP",           "sensor", "",   "",            "mdi:wifi",          "{{ value_json.IP }}");
  sendMQTTDiscoveryMsg("SSID",         "sensor", "",   "",            "mdi:wifi",          "{{ value_json.SSID }}");
  sendMQTTDiscoveryMsg("RSSI_WIFI",    "sensor", "dB", "",            "mdi:wifi",          "{{ value_json.RSSI_WIFI}}");
  sendMQTTDiscoveryMsg("Light",        "switch",  "",  "",            "mdi:lightbulb",     "{{ value_json.light }}");
  }

void MQTTsendBlock() {
  char timeString[64];
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) 
    {
    strftime(timeString, sizeof(timeString), "%Y-%m-%dT%H:%M:%S", &timeinfo); //   "%d.%m.%Y %H:%M"
    }

  MQTTreconnect();
 
  JsonDocument doc;
  doc["time"] = timeString; //"01.01.2000"; //epoch_to_string(time(nullptr));
  doc["IP"] = WiFi.localIP().toString();
  doc["SSID"] = String(nets[bestNet].ssid);
  doc["RSSI_WIFI"] = WiFi.RSSI();
  doc["version"] =    version;

  char buffer[500];
  int n = serializeJson(doc, buffer);
  //Serial.printf("JSON buffersize data: %d\n", n);
  if (!MQTTclient.publish(MQTT_STATE, (const uint8_t*)buffer, n, true))
    Serial.printf("MQTT publish data - Fehler!\n");
  delay(200);
}


//**************************************************************************
// callback - Wird aufgerufen, wenn HA den Schalter betätigt

void callback(char* topic, byte* payload, unsigned int length) 
  {
  String msg = "";
  //Serial.println("Callback");

  for (int i = 0; i < length; i++) 
    {
    msg += (char)payload[i];
    }
	if (msg == "OFF") 
    {
    digitalWrite(LED_PIN, !LED_ON); 
	  Serial.println("Licht aus!");
	  MQTTclient.publish(MQTT_SWITCH_STATE, "OFF", true);
	  obohosLight(OFF);
	  lightState = OFF;
    }
	else if (msg == "ON") 
    {
    digitalWrite(LED_PIN, LED_ON); 
	  Serial.println("Licht an!");
	  MQTTclient.publish(MQTT_SWITCH_STATE, "ON", true);
	  obohosLight(ON);
	  lightState = ON;
    startTime = millis();
    }	
  MQTTsendBlock();

  }


  //**************************************************************************
// find best wireless network from list nets[]
void getBestWifi() 
{
  int bestRSSI = -999;

  WiFi.disconnect();
  //WiFi.persistent(true);
  WiFi.mode(WIFI_STA);

  Serial.printf("Network Scan\n");
  int i;
  for (i = 0; i < sizeof(nets) / sizeof(Networks); i++) 
    {
    int loops = 10;
    Serial.printf("SSID: %s --> ", nets[i].ssid);
    WiFi.begin(nets[i].ssid, nets[i].password);
    while ((WiFi.status() != WL_CONNECTED) && (loops-- > 0)) delay(500);
    if (WiFi.status() != WL_CONNECTED) 
    {
      Serial.printf("Not available!\n");
    } 
    else 
    {
      Serial.printf("RSSI: %d\n", WiFi.RSSI());
      if (WiFi.RSSI() > bestRSSI)  // keep index for SSID with better RSSI
      {
        bestRSSI = WiFi.RSSI();
        bestNet = i;
      }
    }
    WiFi.setAutoReconnect(false); 
    WiFi.disconnect();
    digitalWrite(LED_PIN, !LED_ON);  // blink as indication for network search
    delay(200);                      //
    digitalWrite(LED_PIN, LED_ON);
  }
  Serial.printf("Connecting to %s, ", nets[bestNet].ssid);
  //WiFi.setHostname("sensor");
  WiFi.begin(nets[bestNet].ssid, nets[bestNet].password);
  int loops = 10;
  while ((WiFi.status() != WL_CONNECTED) && (loops-- > 0)) delay(500);

  Serial.printf("Connected SSID: %s\n", nets[bestNet].ssid);
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
  Serial.println(WiFi.localIP());
}

//*****************************************************************
// Get local time
void getNTPtime()
  {
  // NTP mit Zeitzonen-String konfigurieren
  configTzTime(timeZone, ntpServer);
  
  // Auf Zeitabruf warten
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Fehler beim Abruf der Zeit!");
    return;
  }
  
  Serial.println(&timeinfo, "Aktuelle Zeit: %A, %B %d %Y %H:%M:%S");
  Serial.print("Zeitzone: ");
  Serial.println(timeZone);
  }