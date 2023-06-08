#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <HardwareSerial.h>

// Apartment WiFi
const char* ssid = "";
const char* password = "";

// Northwestern WiFi
//const char* ssid = ""; 

WebSocketsClient websockets_client;

String getTimeStamp() {
  unsigned long currentMillis = millis();
  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;

  String timeStamp = String(hours) + ":" + String(minutes % 60) + ":" + String(seconds % 60);
  return timeStamp;
}

void websockets_event(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server");
      break;
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server");
      break;
    case WStype_TEXT:
      Serial.println("Received message from WebSocket server: " + String((char*)payload));
      break;
  }
}

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 17, 16); // RX = GPIO17, TX = GPIO16

  WiFi.begin(ssid, password);
  //WiFi.begin(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  websockets_client.begin("18.218.206.202", 8888, "/websocket_esp32"); // Server IP
  websockets_client.onEvent(websockets_event);
}

void loop() {
  if (Serial2.available()) {
    String receivedData = getTimeStamp() + " " + Serial2.readString();
    websockets_client.sendTXT(receivedData);
  }
  websockets_client.loop();
}
