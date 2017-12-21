#define GREEN_LED_PIN 30   
#define ORANGE_LED_PIN 31  //Pulse Pin
#define RED_LED_PIN 32
#define CONFIGURE_BTN_PIN 33



#define RTC_ERROR 1
#define SD_ERROR 2
#define MQTT_ERROR 4
#define ETH_ERROR 8


#define ERR_OK      0    //0 = Normal = Green ON
#define ERR_RTC     1    //1 = No RTC = green pulse OFF flash
#define ERR_SD      2    //2 = No SD = green pulse ON flash
#define ERR_RSD     3    //3 = No RTC/SD = green flash
#define ERR_MQTT    4    //4 = No MQTT = red fast OFF flash
#define ERR_ETH     12   //8/12 = No Eth/Mqtt = red fast ON flash
#define ERR_RMQTT   5    //5 = No MQTT/RTC = red flash
//#define ERR_DEAD     1    //6/7/9/10/11/13/14/15 = red solid = DEAD

