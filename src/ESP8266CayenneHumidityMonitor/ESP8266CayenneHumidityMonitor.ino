//#define DEBUG // uncomment to print to Serial output
//#define CAYENNE_PRINT Serial

#include "DebugUtil.h" // a debug util to remove debug statements from build if DEBUG is not defined
#include "DHT.h"  // using Adafruit's library
#include <MyCredentials.h> // contains my credential information, outside of this project so I don't accidently commit to GitHub
#include <CayenneMQTTESP8266.h>

#define DHT_GPIO 4     // GPIO pin the DHT22 is conected to, D2 on Wemos D1 Mini board
#define DHTTYPE DHT22
#define DHT_RETRY_COUNT 10
#define DHT_RETRY_DELAY 2000 // internet said wait 2 seconds to give the DHT time to get ready

#define STARTUP_TEST_RETRY_COUNT 5
#define STARTUP_TEST_RETRY_DELAY 15000
#define DEEP_SLEEP_TIME 1200000000 // 20minutes = 1200000000us

#define ADC_SAMPLE_COUNT 10
#define ADC_SAMPLE_DELAY 50
#define RESISTOR1 490000 // 270k added to R1 (220k) of the voltage divider of the Wemos D1 Mini before the ADC
#define RESISTOR2 100000 // R2 from the Wemos D1 Mini divider before ADC
#define VOLTAGE_ERROR_CORRECTION 1.000 // set to 1 with debug on, find percentage voltage is off and update

#define BUTTON_GPIO 5
#define LED_GPIO 13

void syncData();

char ssid[] = HOME_WIFI_SSID;
char wifiPassword[] = HOME_WIFI_PASSWORD;

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = CAYENNE_USERNAME;
char password[] = CAYENNE_PASSWORD;
char clientID[] = CAYENNE_CLIENT_ID_HATCHBOX_ABS;

float humidity = 0;
float temperature = 0;
float voltage = 0;

DHT dht(DHT_GPIO, DHTTYPE);

void setup() {
  DEBUG_UTIL_BEGIN;
  DEBUG_UTIL_PRINTLN("");
  DEBUG_UTIL_PRINTLN("Starting...");

  // setup static IP as this cuts down on time trying to obtain an IP through DHCP
  IPAddress espIP(192, 168, 1, 80);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(espIP, gateway, subnet);

  pinMode(BUTTON_GPIO, INPUT_PULLUP);
  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, LOW);
  
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);
  
  DEBUG_UTIL_PRINTLN("---------------------------");
  DEBUG_UTIL_PRINTLN("WiFi connected");
  DEBUG_UTIL_PRINT("IP address: ");
  DEBUG_UTIL_PRINTLN(WiFi.localIP());
  DEBUG_UTIL_PRINT("RSSI: ");
  DEBUG_UTIL_PRINTLN(WiFi.RSSI());
  DEBUG_UTIL_PRINTLN("---------------------------");
}

void loop() {
  Cayenne.loop();
  
  int performTest = !digitalRead(BUTTON_GPIO);
  if(performTest) {
    DEBUG_UTIL_PRINTLN("Perform test routine");
    digitalWrite(LED_GPIO, HIGH);
    for(int count = 0; count < STARTUP_TEST_RETRY_COUNT; count++){
      syncData();
      delay(STARTUP_TEST_RETRY_DELAY);
    }
  }
  
  syncData();

  DEBUG_UTIL_PRINTLN("Entering deep sleeeeeeep...........");

  ESP.deepSleep(DEEP_SLEEP_TIME, WAKE_RF_DEFAULT);
  delay(500);
}

void syncData(){
  DEBUG_UTIL_PRINTLN("syncData() - START");

  // retry a few times for a good reading
  int retries = 0;
  while(isnan(humidity = dht.readHumidity()) && (retries++ < DHT_RETRY_COUNT)){
    delay(DHT_RETRY_DELAY);
  }

  retries = 0;
  while(isnan(temperature = dht.readTemperature()) && (retries++ < DHT_RETRY_COUNT)){
    delay(DHT_RETRY_DELAY);
  }
  
  // get average of analog reads, I would get weird really off values from time to time, probably better algorithms out there but this is good enough for me
  voltage = 0;
  for(int i = 0; i < ADC_SAMPLE_COUNT; i++){
    voltage += analogRead(A0) / 1023.0 * (RESISTOR1 + RESISTOR2) / RESISTOR2 * VOLTAGE_ERROR_CORRECTION;
    delay(ADC_SAMPLE_DELAY);
  }
  voltage /= ADC_SAMPLE_COUNT;
  
  int rssi = WiFi.RSSI();

  DEBUG_UTIL_PRINT("Sending humidity: ");
  DEBUG_UTIL_PRINT(humidity);
  DEBUG_UTIL_PRINT("%  temperature: ");
  DEBUG_UTIL_PRINT(temperature);
  DEBUG_UTIL_PRINT("  voltage: ");
  DEBUG_UTIL_PRINT(voltage);
  DEBUG_UTIL_PRINT("  RSSI: ");
  DEBUG_UTIL_PRINTLN(rssi);

  Cayenne.virtualWrite(0, humidity, TYPE_RELATIVE_HUMIDITY, UNIT_PERCENT);
  Cayenne.celsiusWrite(1, temperature);
  Cayenne.virtualWrite(2, voltage, TYPE_BATTERY, UNIT_VOLTS);
  Cayenne.virtualWrite(3, rssi);

  DEBUG_UTIL_PRINTLN("syncData() - END");
}

//Default function for processing actuator commands from the Cayenne Dashboard.
//You can also use functions for specific channels, e.g CAYENNE_IN(1) for channel 1 commands.
CAYENNE_IN_DEFAULT()
{
  CAYENNE_LOG("CAYENNE_IN_DEFAULT(%u) - %s, %s", request.channel, getValue.getId(), getValue.asString());
  //Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
}
