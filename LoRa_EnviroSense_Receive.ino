//  LoRa Receiver Module

#include <RH_RF95.h>
#include <SoftwareSerial.h>

// Feather 32u4:
#define RFM95_CS   8
#define RFM95_RST  4
#define RFM95_INT  7

// MATCH TX/RX FREQ
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// UART Communications -> ESP32
const byte rxPin = 2;
const byte txPin = 3;

// SoftwareSerial object
SoftwareSerial commSerial(rxPin, txPin); // Only the Tx pin is important - used for transmitting data!

void setup() {
  // Define pin modes for TX and RX
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

  commSerial.begin(9600); // Begin UART Serial Communications to ESP32
  Serial.begin(9600); 
  
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(100);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

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
}

void loop() {
  if (rf95.available()) {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      // Receive SUCCESS
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(13, HIGH); // turn on LED
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      // Extract values from received string
      String receivedString = String((char*)buf);
      Serial.print("Received string: ");
      Serial.println(receivedString); // print received string
      String values[8];
      int currentIndex = 0;
      int lastIndex = 0;
      int i = 0;
      while (currentIndex != -1) {
        currentIndex = receivedString.indexOf(',', lastIndex);
        if (currentIndex != -1) {
          values[i] = receivedString.substring(lastIndex, currentIndex);
          lastIndex = currentIndex + 1;
        } else {
          values[i] = receivedString.substring(lastIndex);
        }
        i++; // increment 'i'
      }
      // Extracted Values
      int lightValue = values[0].toInt();
      int uvIndex = values[1].toInt();
      int waterValue = values[2].toInt();
      float temperature = values[3].toFloat();
      float humidity = values[4].toFloat();
      float ppm = values[5].toFloat();
      float pressure = values[6].toFloat();
      float ph = values[7].toFloat();

      // UART Serial Communication to the ESP32
      // Send data to ESP32 over UART
      commSerial.print(":LightLevel:");
      commSerial.println(lightValue);
      commSerial.print(":UVIndex:");
      commSerial.println(uvIndex);
      commSerial.print(":WaterLevel:");
      commSerial.println(waterValue);
      commSerial.print(":Temperature:");
      commSerial.println(temperature);
      commSerial.print(":Humidity:");
      commSerial.println(humidity);
      commSerial.print(":AirQuality:");
      commSerial.println(ppm);
      commSerial.print(":Pressure:");
      commSerial.println(pressure);
      commSerial.print(":pH:");
      commSerial.println(ph);
      
      // Printout / UART
      Serial.print("LightLevel:");
      Serial.println(lightValue);
      Serial.print("UVIndex:");
      Serial.println(uvIndex);
      Serial.print("WaterLevel:");
      Serial.println(waterValue);
      Serial.print("Temperature:");
      Serial.println(temperature);
      Serial.print("Humidity:");
      Serial.println(humidity);
      Serial.print("AirQuality:");
      Serial.println(ppm);
      Serial.print("Pressure:");
      Serial.println(pressure);
      Serial.print("pH:");
      Serial.println(ph);
      
      // Send a reply
      uint8_t data[] = "data received :)";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
    } else {
      // Receive FAILED
      Serial.println("Receive failed");
    }
  }
  delay(100);
}
