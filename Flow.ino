//Shield for the Itead IBoard Pro - Arduino Mega clone

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include <RTClib.h>

#include <SPI.h>
#include <SD.h>

#include <Ethernet.h>
#include <PubSubClient.h>
#include "defines.h"

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
#define CLIENT_ID "flowMeter"
#define USER "floodwatch"
#define PASSWORD "floodwatch##"


#define SD_SELECT_PIN 4
uint8_t mac[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x06};

byte errorCode=0;

void init_RTC()
{
  errorCode+=RTC_ERROR;
  Serial.print("Initializing RTC...");
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    return;
  }

  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //  rtc.adjust(DateTime(2017, 1, 21, 3, 0, 0));

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    return;
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
  errorCode-=RTC_ERROR;
}

void init_SD()
{
  errorCode+=SD_ERROR;
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(SD_SELECT_PIN)) {
    Serial.println("Card failed, or not present");
    
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  errorCode-=SD_ERROR;
}

void init_Network()
{
  errorCode+=ETH_ERROR;
  // setup ethernet communication using DHCP
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Unable to configure Ethernet using DHCP"));
    return;
  }
  
  Serial.println(F("Ethernet configured via DHCP"));
  Serial.print("IP address: ");
  Serial.println(Ethernet.localIP());
  Serial.println();
  errorCode-=ETH_ERROR;
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
}

void init_MQTT()
{
  errorCode+=MQTT_ERROR;
  // setup mqtt client
  mqttClient.setClient(ethClient);
  mqttClient.setServer("broker.secluded.io", 1883);
  Serial.println(F("MQTT client configured:Connecting..."));
  //mqttClient.setCallback(callback);
  if(mqttClient.connect(CLIENT_ID, USER, PASSWORD)) {
    mqttClient.publish("flow/Online", "RST");
    errorCode-=MQTT_ERROR;
    //Serial.println(ip);
  } 
}

void initPins()
{
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(ORANGE_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(ORANGE_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, HIGH);
  delay(500);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(ORANGE_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  for(byte i = 0; i < NO_METERS; i++){
    pinMode(meter[i].pinNumber, INPUT_PULLUP);
  }  
}


void setup() {
  // put your setup code here, to run once:
  

  Serial.begin(115200);
  while (!Serial); // for Leonardo/Micro/Zero
  init_RTC();
  init_SD();
  init_Network();
  init_MQTT();
  initPins();
}

void loop() {
  // put your main code here, to run repeatedly:
//  DateTime now = rtc.now();
//  Serial.print(now.unixtime());

  //appendDataToSD(5);
  mqttClient.loop();
  checkMeters();
  handleErrors();
}

static unsigned long orangeOnTime = 0;
byte greenDutyCycle = 100;
byte redDutyCycle = 0;
#define ledPulseWidth 1000

void handleErrors()
{
  static byte lastError = 99;
  switch(errorCode){
    case ERR_OK:  //0 = Normal = Green ON
      greenDutyCycle = 100;
      redDutyCycle = 0;
      break;
    case ERR_RTC: //1 = No RTC = green pulse OFF flash
      greenDutyCycle = 20;
      redDutyCycle = 0;
      //init_RTC();
      break;
    case ERR_SD:  //2 = No SD = green pulse ON flash
      greenDutyCycle = 80;
      redDutyCycle = 0;
      //init_SD();
      break;
    case ERR_RSD: //3 = No RTC/SD = green flash
      greenDutyCycle = 50;
      redDutyCycle = 0;
      break;
    case ERR_MQTT:  //4 = No MQTT = red fast OFF flash
      greenDutyCycle = 0;
      redDutyCycle = 20;
      //init_MQTT();
      break;
    case ERR_ETH: //8/12 = No Eth/Mqtt = red fast ON flash
      greenDutyCycle = 0;
      redDutyCycle = 80;
      init_Network();
      break;
    case ERR_RMQTT: //5 = No MQTT/RTC = red flash
      greenDutyCycle = 0;
      redDutyCycle = 50;
      break;
    default:
      greenDutyCycle = 0;
      redDutyCycle = 100;
      break;
  }
  updateLED();
  if(lastError != errorCode){
    Serial.print("ERR:");
    Serial.println(errorCode);
    Serial.print(greenDutyCycle);
    Serial.print(":");
    Serial.println(redDutyCycle);
  }
  lastError = errorCode;
}

void updateLED()
{
  static unsigned long greenOnTime = 0;
  static unsigned long redOnTime = 0;
  static boolean greenLEDState = LOW;
  static boolean redLEDState = LOW;
  if(greenDutyCycle > 0){
    if(greenLEDState == LOW){
      greenLEDState = HIGH;
      greenOnTime = millis();
    } else {
      if(greenDutyCycle != 100 && millis()-greenOnTime > ((ledPulseWidth / 100)*greenDutyCycle)){
        greenLEDState = LOW;
      }
    }
  } else {
    greenLEDState = LOW;
  }
  digitalWrite(GREEN_LED_PIN, greenLEDState);

  if(redDutyCycle > 0){
    if(redLEDState == LOW){
      redLEDState = HIGH;
      redOnTime = millis();
    } else {
      if(redDutyCycle != 100 && millis()-redOnTime > ((ledPulseWidth / 100)*redDutyCycle)){
        redLEDState = LOW;
      }
    }
  } else {
    redLEDState = LOW;
  }
  digitalWrite(RED_LED_PIN, redLEDState);

  if(millis() - orangeOnTime > 250){
    digitalWrite(ORANGE_LED_PIN, LOW);
  }
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
          Serial.println(i+1);
          orangeOnTime = millis();
          digitalWrite(ORANGE_LED_PIN, HIGH);
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
    mqttClient.publish("flow", pumpNumber);
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
