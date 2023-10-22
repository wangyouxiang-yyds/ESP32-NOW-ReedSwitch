#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebSrv.h"
#include <Arduino_JSON.h>


// const char* ssid = "Apple";
// const char* password = "0963982828";

const char *ssid = "三樓的網路";
const char *password = "0963982828";

// const char *ssid = "AGIT";
// const char *password = "BarkingCrab";

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  String board_id;
  float status;
  float hum;
  unsigned int Num_Readings;
} struct_message;

struct_message sensor_readings;

JSONVar board;

AsyncWebServer server(80);
AsyncEventSource events("/events");

void data_receive(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Sensor Readings received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&sensor_readings, incomingData, sizeof(sensor_readings));

  if (sensor_readings.board_id.indexOf("Office_A") != -1) {
    board["board_id"] = sensor_readings.board_id;
    board["status"] = sensor_readings.status;
    // board["humidity"] = sensor_readings.hum;
    // board["Num_Readings"] = String(sensor_readings.Num_Readings);
    String jsonString = JSON.stringify(board);
    events.send(jsonString.c_str(), "new_readings", millis());

    Serial.printf("裝置名稱 %s: %u bytes\n", sensor_readings.board_id, len);
    Serial.printf("門的狀態: %f \n", sensor_readings.status);
    // Serial.printf("Humidity value: %4.2f \n", sensor_readings.hum);
    // Serial.printf("讀取數量: %d \n", sensor_readings.Num_Readings);
    Serial.println();
  }
}

const char index_html[] PROGMEM = "<!DOCTYPE HTML><html><head>"
                                  "<title>ESP-NOW SENSOR READINGS</title>"
                                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                                  "<style>"
                                  "html {font-family: New Times Roman; display: inline-block; text-align: center;}"
                                  "p {  font-size: 1.2rem;}"
                                  "body {  margin: 0;}"
                                  ".topnav { overflow: hidden; background-color: #030103; color: white; font-size: 1.7rem; }"
                                  ".content { padding: 20px; }"
                                  ".card { outline: 1px solid; margin: 10px; padding: 10px; }"
                                  ".reading { font-size: 2.8rem; }"
                                  "</style>"
                                  "</head>"
                                  "<body>"
                                  "<div class=\"topnav\">"
                                  "<h3>ESP-NOW SENSOR READINGS</h3>"
                                  "</div>"
                                  "<div class=\"content\">"
                                  "<h4>Connected Device: <span id=\"deviceCount\">0</span></h4>"
                                  "<div id=\"devicesContainer\">"
                                  "</div>"
                                  "</div>"
                                  "<script>"
                                  "var deviceCount = 0;"
                                  "if (!!window.EventSource) {"
                                  " var source = new EventSource('/events');"
                                  " source.addEventListener('open', function(e) {"
                                  "  console.log('Events Connected');"
                                  " }, false);"
                                  " source.addEventListener('error', function(e) {"
                                  "  if (e.target.readyState != EventSource.OPEN) {"
                                  "    console.log('Events Disconnected');"
                                  "  }"
                                  " }, false);"
                                  " source.addEventListener('message', function(e) {"
                                  "  console.log('message', e.data);"
                                  " }, false);"
                                  " source.addEventListener('new_readings', function(e) {"
                                  "  console.log('new_readings', e.data);"
                                  "  var obj = JSON.parse(e.data);"
                                  "  var devicesContainer = document.getElementById('devicesContainer');"
                                  "  var deviceDiv = document.getElementById('device' + obj.board_id);"
                                  "  if (!deviceDiv) {"
                                  "    deviceDiv = document.createElement('div');"
                                  "    deviceDiv.id = 'device' + obj.board_id;"
                                  "    deviceDiv.className = 'card';"
                                  "    devicesContainer.appendChild(deviceDiv);"
                                  "    deviceCount++;"
                                  "    document.getElementById('deviceCount').textContent = deviceCount;"
                                  "  }"
                                  "  var statusValue = obj.status;"
                                  "  deviceDiv.innerHTML = '<h4>ESP32 #' + obj.board_id + ' --- STATUS</h4>' +"
                                  "    '<p><span class=\"reading\">[' + statusValue + ']</span></p>' +"
                                  "    '<p class=\"packet\">Reading Number: ' + obj.Num_Readings + '</p>';"
                                  " }, false);"
                                  "}"
                                  "</script>"
                                  "</body>"
                                  "</html>";



void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(data_receive);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.on("/prtg", HTTP_GET, [](AsyncWebServerRequest *request) {
    JSONVar prtgData;
    JSONVar resultArray;
    JSONVar resultObj;

    // 填入第一個 channel 的資料
    resultObj["channel"] = "First channel";
    resultObj["value"] = sensor_readings.status;
    resultArray[0] = resultObj;
    prtgData["prtg"]["result"] = resultArray;

    String prtgJson = JSON.stringify(prtgData);

    request->send(200, "application/json", prtgJson);
  });
  events.onConnect([](AsyncEventSourceClient *client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }

    client->send("Welcome!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
}

void loop() {
  static unsigned long last_time = millis();
  static const unsigned long interval = 5000;
  if ((millis() - last_time) > interval) {
    events.send("ping", NULL, millis());
    last_time = millis();
  }
}

//54:43:B2:AC:F9:30


// #include <WiFi.h>

// void setup(){
//   Serial.begin(115200);
//   Serial.println();
//   Serial.println(WiFi.macAddress());
// }

// void loop(){

// }