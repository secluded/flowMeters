//Shield for the Itead IBoard Pro - Arduino Mega clone

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"

#include <SPI.h>
#include <SD.h>

#include <Ethernet.h>
#include "PubSubClient.h"

#define DEBOUNCE_TIME 100
#define NO_METERS 32
typedef struct {
  byte pinNumber;
  unsigned long changeTime;
  boolean lastState;
  boolean btnState;
} meterData;


meterData meter[] = { 
  {34,0,HIGH,HIGH},{65,0,HIGH,HIGH},{64,0,HIGH,HIGH},{63,0,HIGH,HIGH},{62,0,HIGH,HIGH},{61,0,HIGH,HIGH},{60,0,HIGH,HIGH},{59,0,HIGH,HIGH},
  {58,0,HIGH,HIGH},{54,0,HIGH,HIGH},{55,0,HIGH,HIGH},{56,0,HIGH,HIGH},{57,0,HIGH,HIGH},{66,0,HIGH,HIGH},{67,0,HIGH,HIGH},{68,0,HIGH,HIGH},
  {69,0,HIGH,HIGH},{41,0,HIGH,HIGH},{40,0,HIGH,HIGH},{29,0,HIGH,HIGH},{28,0,HIGH,HIGH},{27,0,HIGH,HIGH},{26,0,HIGH,HIGH},{25,0,HIGH,HIGH},
  {24,0,HIGH,HIGH},{23,0,HIGH,HIGH},{22,0,HIGH,HIGH},{39,0,HIGH,HIGH},{38,0,HIGH,HIGH},{37,0,HIGH,HIGH},{36,0,HIGH,HIGH},{35,0,HIGH,HIGH}
};
 


RTC_DS1307 rtc;

EthernetClient ethClient;
PubSubClient mqttClient;
#define CLIENT_ID "flowTest5435"


#define SD_SELECT_PIN 4
uint8_t mac[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x06};



void init_RTC()
{
  Serial.print("Initializing RTC...");
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//  rtc.adjust(DateTime(2017, 1, 21, 3, 0, 0));
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
  Serial.println("RTC initialized.");

  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);
}

void init_SD()
{
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(SD_SELECT_PIN)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
}

void init_MQTT()
{
  // setup ethernet communication using DHCP
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Unable to configure Ethernet using DHCP"));
    for (;;);
  }

  Serial.println(F("Ethernet configured via DHCP"));
  Serial.print("IP address: ");
  Serial.println(Ethernet.localIP());
  Serial.println();
  /*
    Serial.println(Ethernet.localIP()[0]);
    Serial.println(Ethernet.localIP()[1]);
    Serial.println(Ethernet.localIP()[2]);
    Serial.println(Ethernet.localIP()[3]);
  */
  String ip = String (Ethernet.localIP()[0]);
  ip = ip + ".";
  ip = ip + String (Ethernet.localIP()[1]);
  ip = ip + ".";
  ip = ip + String (Ethernet.localIP()[2]);
  ip = ip + ".";
  ip = ip + String (Ethernet.localIP()[3]);
  //Serial.println(ip);

  // setup mqtt client
  mqttClient.setClient(ethClient);
   mqttClient.setServer("test.mosquitto.org", 1883);
  Serial.println(F("MQTT client configured"));
  //mqttClient.setCallback(callback);
  if(mqttClient.connect(CLIENT_ID)){//, USER, PASSWORD)) 
    mqttClient.publish("flow543g", "RST");
    Serial.println(ip);
  }

}



void setup() {
  // put your setup code here, to run once:
  

  Serial.begin(115200);
  while (!Serial); // for Leonardo/Micro/Zero
  init_RTC();
  init_SD();
  init_MQTT();
  for(byte i = 0; i < NO_METERS; i++){
    pinMode(meter[i].pinNumber, INPUT_PULLUP);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
//  DateTime now = rtc.now();
//  Serial.print(now.unixtime());

  //appendDataToSD(5);
  mqttClient.loop();
  checkMeters();
}


void checkMeters()
{
  for(byte i = 0; i < NO_METERS; i++) {
    
    boolean btnReading = digitalRead(meter[i].pinNumber);
    
    if(btnReading != meter[i].lastState) {
      meter[i].changeTime = millis();
    }
    
    if((millis() - meter[i].changeTime) > DEBOUNCE_TIME) {
      if(btnReading != meter[i].btnState){
        meter[i].btnState = btnReading;
        
        if(meter[i].btnState == LOW){
          Serial.print(i+1);
          sendPulse(i+1);
        }
      }
    }
    meter[i].lastState = btnReading;
  }
}

void sendPulse(byte pumpNumber)
{
  String dataString = "";
  DateTime now = rtc.now();
  
  dataString += String(now.unixtime());
  dataString += ",";
  dataString += String(pumpNumber);

  if(mqttClient.connect(CLIENT_ID)){//, USER, PASSWORD)) 
    mqttClient.publish("flow543g", "msg");
    Serial.println(dataString);
  } else {
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
  
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      Serial.println(dataString);
    }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog.txt");
    }
  }
}
