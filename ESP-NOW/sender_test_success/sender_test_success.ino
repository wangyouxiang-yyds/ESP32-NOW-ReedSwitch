#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#define board_ID "Office_A_1"
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

int switchReed = 35;  // 磁簧開關
//MAC Address of the receiver
uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


typedef struct struct_message {
  String board_id;
  float status;
  float hum;
  int Num_Readings;
} struct_message;

struct_message sensor_readings;

unsigned long previous_time = 0;
const long interval = 1000;

unsigned int Num_Readings = 0;

// 時間設定
const char *ntpserver = "pool.ntp.org";
const uint16_t utcOffest = 28800;
const uint8_t daylightOffest = 0;
String year, month, day, currentSec, currentMin, currentHour;  // 年, 月, 日, 秒, 分, 時
String dateValue;                                              // 取得時間
HTTPClient http;
bool isHoliday;  // 判定是否假日


// wifi設定(抓網路時間必須要連網路)
// const char *ssid = "AGIT";
// const char *pwd = "BarkingCrab";

// const char* ssid = "Apple";
// const char* pwd = "0963982828";


const char *ssid = "三樓的網路";
const char *pwd = "0963982828";


// constexpr char wifi_SSID[] = "AGIT";
// constexpr char wifi_SSID[] = "Apple";
constexpr char wifi_SSID[] = "三樓的網路";

bool door_changed = false;  // 新增 door_changed 變數
int32_t obtain_wifi(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i = 0; i < n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}


float get_status() {

  if (digitalRead(switchReed) == HIGH) {
    Serial.println("Window Open");
    return 1;
  } else if (digitalRead(switchReed) == LOW) {
    Serial.println("Window Closed");
    return 0;
  }
}

/*
void get_holiday_json(String year, String month, String day) {
  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("連線成功");
    Serial.print("Ip Address: ");
    Serial.println(WiFi.localIP());
  }

  // 設置取得網路時間
  configTime(utcOffest, daylightOffest, ntpserver);

  struct tm now;

  if (!getLocalTime(&now)) {
    Serial.println("無法取得時間~");
  } else {
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", &now);
    year = String(buffer + 0, 4);   // 年份
    month = String(buffer + 5, 2);  // 月份
    day = String(buffer + 8, 2);    // 日期

    Serial.println(year + "/" + month + "/" + day);
  }

  
  String url = "https://cdn.jsdelivr.net/gh/ruyut/TaiwanCalendar/data/" + year + ".json";
  DynamicJsonDocument doc(49152);  // json解析大小
  http.begin(url);                 // 指定網址
  int httpCode = http.GET();       // 發請連結請求

  if (httpCode > 0) {
    String payload = http.getString();  // 回傳http 本體(字串格式)

    // 解析 JSON
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {

      String targetDate = year + month + day;

      for (int i = 0; i < doc.size(); i++) {
        String dateValue = doc[i]["date"].as<String>();
        if (dateValue == targetDate) {
          Serial.println("今天日期：" + dateValue);
          bool isHoliday = doc[i]["isHoliday"].as<bool>();
          Serial.println("是否為放假：" + String(isHoliday));
          return;
        }
      }
    } else {
      Serial.println("JSON 解析錯誤");
    }
  } else {
    Serial.println("HTTP 錯誤");
  }
  delay(5000);
  
}

*/

// 取的現在的時間並計算需要睡眠的秒數
void get_time_current() {
  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("連線成功");
    Serial.print("Ip Address: ");
    Serial.println(WiFi.localIP());
  }
  // 設置取得網路時間
  configTime(utcOffest, daylightOffest, ntpserver);

  struct tm now;

  if (!getLocalTime(&now)) {
    Serial.println("無法取得時間~");
  } else {
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", &now);

    year = String(buffer + 0, 4);          // 年份
    month = String(buffer + 5, 2);         // 月份
    day = String(buffer + 8, 2);           // 日期
    currentHour = String(buffer + 11, 2);  // 時
    currentMin = String(buffer + 14, 2);   // 分
    currentSec = String(buffer + 17, 2);   // 秒

    Serial.println(year + "/" + month + "/" + day + "\t" + currentHour + ":" + currentMin + ":" + currentSec);
  }

  String url = "https://cdn.jsdelivr.net/gh/ruyut/TaiwanCalendar/data/" + year + ".json";
  DynamicJsonDocument doc(49152);  // json解析大小
  http.begin(url);                 // 指定網址
  int httpCode = http.GET();       // 發請連結請求

  if (httpCode > 0) {
    String payload = http.getString();  // 回傳http 本體(字串格式)

    // 解析 JSON
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {

      String targetDate = year + month + day;

      for (int i = 0; i < doc.size(); i++) {
        String dateValue = doc[i]["date"].as<String>();
        if (dateValue == targetDate) {
          Serial.println("今天日期：" + dateValue);
          bool isHoliday = doc[i]["isHoliday"].as<bool>();
          Serial.println("是否為放假：" + String(isHoliday));
          return;
        }
      }
    } else {
      Serial.println("JSON 解析錯誤");
    }
  } else {
    Serial.println("HTTP 錯誤");
  }
  /*  
  if (isHoliday == false) {
    if (currentHour.toInt() > 9 && currentHour.toInt() <= 20) {
      // 計算離晚上八點20:00:00有多少秒
      int eight_PM_Hour = 20;
      int eight_PM_Min = 60;
      int eight_PM_Sec = 60;

      // 需要睡眠的時間 時分秒加一起放進needToSleepSec
      long needToSleepSec = ((eight_PM_Hour - currentHour.toInt()) * 3600) + ((eight_PM_Min - currentMin.toInt()) * 60) + (eight_PM_Sec - currentSec.toInt());

      Serial.println("需睡眠的時間" + String(needToSleepSec) + " 秒");
      esp_sleep_enable_timer_wakeup(needToSleepSec * 1000000);  // 將秒轉換為微秒
      delay(1000);
      Serial.println("進入睡眠");
      esp_deep_sleep_start();
    }
  } 
  // 斷開 Wi-Fi 連接
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("網路已被斷開");*/
}


void data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Readings Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

esp_now_peer_info_t peerInfo;

void setup() {

  Serial.begin(115200);
  get_time_current();  // 擷取現在網路時間
  bool isHoliday = false;  // 判定是否假日
  if (isHoliday == false) {
    if (currentHour.toInt() > 8 && currentHour.toInt() <= 20) {
      // 計算離晚上八點20:00:00有多少秒
      int eight_PM_Hour = 20;
      int eight_PM_Min = 60;
      int eight_PM_Sec = 60;

      // 需要睡眠的時間 時分秒加一起放進needToSleepSec
      long needToSleepSec = ((eight_PM_Hour - currentHour.toInt()) * 3600) + ((eight_PM_Min - currentMin.toInt()) * 60) + (eight_PM_Sec - currentSec.toInt());

      Serial.println("需睡眠的時間" + String(needToSleepSec) + " 秒");
      esp_sleep_enable_timer_wakeup(needToSleepSec * 1000000);  // 將秒轉換為微秒
      delay(1000);
      Serial.println("進入睡眠");
      esp_deep_sleep_start();
    }
  }
  // 斷開 Wi-Fi 連接
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("網路已被斷開");

  WiFi.mode(WIFI_STA);
  pinMode(switchReed, INPUT);
  int32_t channel = obtain_wifi(wifi_SSID);
  WiFi.printDiag(Serial);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }


  esp_now_register_send_cb(data_sent);


  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 1);  // 第二個參數是觸發方式（高電平觸發）
}

void loop() {
  // 此部分為判斷高低電位差(意思為開關門, 如果關門的話要送一個出去再進入睡眠)
  if (digitalRead(switchReed) == HIGH) {
    unsigned long current_time = millis();
    if (current_time - previous_time >= interval) {
      previous_time = current_time;
      sensor_readings.board_id = board_ID;
      sensor_readings.status = get_status();
      sensor_readings.Num_Readings = Num_Readings++;

      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&sensor_readings, sizeof(sensor_readings));
      if (result == ESP_OK) {
        Serial.println("Sent with success");
      } else {
        Serial.println("Error sending the data");
      }
    }
  } else {

    sensor_readings.board_id = board_ID;
    sensor_readings.status = get_status();
    // sensor_readings.Num_Readings = Num_Readings++;

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&sensor_readings, sizeof(sensor_readings));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    } else {
      Serial.println("Error sending the data");
    }
    // 資料傳送出去在進入睡眠
    delay(5000);
    esp_deep_sleep_start();
  }
}