// LoRa Transmitter Module

#include <SPI.h>
#include <RH_RF95.h>
#include <dht.h>
#include <Adafruit_LPS2X.h>
#include <MQ135.h>

// Feather 32u4:
#define RFM95_CS   8
#define RFM95_RST  4
#define RFM95_INT  7

// MATCH TX/RX FREQ
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Pins
int photoresistorPin = A0;
int UV_PIN = A1;
int waterPIN = A2;
int MQ135PIN = A3;
int phPIN = A4;

// Sensor Configuration
// DHT 11 - Temperature / Humidity
dht DHT;
#define DHT11_PIN 3

// LPS22 - Pressure / Temperature
#define LPS_CS 9
#define LPS_SCK 15
#define LPS_MISO 14
#define LPS_MOSI 16
Adafruit_LPS22 lps;

// MQ135 - Air Quality
MQ135 gasSensor = MQ135(MQ135PIN);

// Variables
// Correction factor
const float correctionFactor = -5.0;

void setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  delay(100);

  Wire.begin();
  
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  // initialize LoRa wireless communications
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  rf95.setTxPower(23, false);

  // initialize LPS22 Sensor
  if (!lps.begin_SPI(LPS_CS)) {
  //if (!lps.begin_SPI(LPS_CS, LPS_SCK, LPS_MISO, LPS_MOSI)) {
    Serial.println("Failed to find LPS22 chip");
    while (1) { delay(10); }
  }
  Serial.println("LPS22 Found!");

  lps.setDataRate(LPS22_RATE_10_HZ);

  pinMode(photoresistorPin, INPUT);
  pinMode(UV_PIN, INPUT);
  pinMode(waterPIN, INPUT);
  pinMode(MQ135PIN, INPUT);
  pinMode(phPIN, INPUT);
}

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop() {
  delay(2000);
  
  // Sensor Measurements-------------------------
  // Photoresistor Light Sensor
  int lightValue = analogRead(photoresistorPin);
  
  // GUVA-S12SD UV Sensor
  float uvValue = analogRead(UV_PIN) / 1024 * 5.0;
  
  // Water Level Sensor
  int waterValue = analogRead(waterPIN);
  
  // DHT11 - Temperature / Humidity
  float temp_F = (DHT.temperature * 9/5) + 32;
  float humid = DHT.humidity;
  int chk = DHT.read11(DHT11_PIN);
  Serial.print("Temperature = ");
  Serial.println(temp_F);
  Serial.print("Humidity = ");
  Serial.println(humid);

  // LPS22 - Pressure / Temperature
  sensors_event_t temp;
  sensors_event_t pressure;
  lps.getEvent(&pressure, &temp);
  float celsius = temp.temperature;
  float fahrenheit = (celsius * 9/5) + 32;

  // MQ135 - Air Quality
  float rzero = gasSensor.getRZero();
  //float correctedRZero = mq135_sensor.getCorrectedRZero(celsius, humidity);
  float resistance = gasSensor.getResistance();
  //float ppm = gasSensor.getPPM();
  //float correctedPPM = mq135_sensor.getCorrectedPPM(celsius, humidity);
  float gasValue = analogRead(MQ135PIN);
  float ppm = gasValue;

  // Gravity pH Analog Sensor
  float phlevel = analogRead(phPIN) / 15; // **15 IS A PLACEHOLDER VALUE TO GET A PROOF OF CONCEPT SINCE WE DO NOT HAVE A PROBE FOR THE SENSOR
  int ph = (-5.6548 * phlevel) + 15.509;
  
  // UV Index
  int uvIndex = 0;
  if (uvValue < 0.05) {
    uvIndex = 0;
  } else if (uvValue < 0.227) {
    uvIndex = 1;
  } else if (uvValue < 0.318) {
    uvIndex = 2;
  } else if (uvValue < 0.408) {
    uvIndex = 3;
  } else if (uvValue < 0.503) {
    uvIndex = 4;
  } else if (uvValue < 0.606) {
    uvIndex = 5;
  } else if (uvValue < 0.696) {
    uvIndex = 6;
  } else if (uvValue < 0.795) {
    uvIndex = 7;
  } else if (uvValue < 0.881) {
    uvIndex = 8;
  } else if (uvValue < 0.976) {
    uvIndex = 9;
  } else {
    uvIndex = 11;
  }

  Serial.println("Transmitting..."); // Send a message to rf95_server

  String dataString = String(lightValue) + "," + String(uvIndex) + "," + String(waterValue) + "," + String(fahrenheit) + "," + String(humid) + "," + String(ppm) + "," + String(pressure.pressure) + "," + String(ph);
  Serial.print("dataString: ");
  Serial.println(dataString);
  
  char radiopacket[dataString.length() + 1];
  strcpy(radiopacket, dataString.c_str());
  
  Serial.print("dataString length: ");
  Serial.println(dataString.length());
  
  itoa(packetnum++, radiopacket+dataString.length()+1, 10);
  Serial.print("Sending "); Serial.println(radiopacket);
  
  Serial.println("Sending...");
  delay(10);
  rf95.send((uint8_t *)radiopacket, dataString.length() + 1);
  
  Serial.println("Waiting for packet to complete...");
  delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply...");
  if (rf95.waitAvailableTimeout(2000)) {
    if (rf95.recv(buf, &len)) {
      // Receive SUCCESS
      digitalWrite(LED_BUILTIN, HIGH); // turn on LED
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
    } else {
      // Receive FAILED
      Serial.println("Receive failed");
    }
  } else {
    // No Listeners
    Serial.println("No reply, is there a listener around?");
  }
  digitalWrite(LED_BUILTIN, LOW); // turn off LED
}
