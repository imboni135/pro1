#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define SS_PIN 5
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// WiFi
const char* ssid = "muhura121";
const char* password = "134567890";

// Server
String serverName = "http://192.168.0.140/livestock/caw1.php";

// Outputs
#define GREEN_LED 12
#define RED_LED 13
#define BUZZER 27

// Button
#define BUTTON_PIN 14

bool registerMode = false;
bool deleteConfirm = false; // 🆕 NEW

void setup() {
  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  SPI.begin();
  mfrc522.PCD_Init();

  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  Serial.print("Connecting to WiFi ");
  display.println("Connecting WiFi...");
  display.display();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.println("WiFi Connected!");
  display.display();
}

void loop() {

  // 👉 Check button (manual register mode)
  if (digitalRead(BUTTON_PIN) == LOW) {
    registerMode = true;

    display.clearDisplay();
    display.println("REGISTER MODE");
    display.display();

    delay(800);
  }

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String tagId = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) tagId += "0";
    tagId += String(mfrc522.uid.uidByte[i], HEX);
  }
  tagId.toUpperCase();

  Serial.println("Tag: " + tagId);

  display.clearDisplay();
  display.println("Scanning...");
  display.display();

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    String url;

    if (registerMode) {
      url = serverName + "?register=1&tagId=" + tagId;
    } else {
      url = serverName + "?tagId=" + tagId;
    }

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {

      String payload = http.getString();
      Serial.println(payload);

      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        String status = doc["status"];

        // ================= FOUND =================
        if (status == "found") {

          String name = doc["name"];
          bool isSick = doc["isSick"];

          display.clearDisplay();
          display.println("Name: " + name);
          display.println("Press Btn=Delete");
          display.display();

          // 👉 WAIT BUTTON FOR DELETE
          unsigned long start = millis();
          while (millis() - start < 4000) {
            if (digitalRead(BUTTON_PIN) == LOW) {

              if (!deleteConfirm) {
                deleteConfirm = true;

                display.clearDisplay();
                display.println("Press again");
                display.println("to DELETE");
                display.display();

                delay(1500);
              } else {

                HTTPClient delHttp;
                String delUrl = serverName + "?delete=1&tagId=" + tagId;
                delHttp.begin(delUrl);
                int delCode = delHttp.GET();

                if (delCode == 200) {
                  display.clearDisplay();
                  display.println("Deleted!");
                  display.display();

                  digitalWrite(RED_LED, HIGH);
                  tone(BUZZER, 1000);
                  delay(1500);
                }

                delHttp.end();
                deleteConfirm = false;
                break;
              }
            }
          }

          if (isSick) {
            digitalWrite(RED_LED, HIGH);
            tone(BUZZER, 1000);
          } else {
            digitalWrite(GREEN_LED, HIGH);
            tone(BUZZER, 2000);
          }

          delay(2000);
        }

        // ================= REGISTERED =================
        else if (status == "registered") {

          display.clearDisplay();
          display.println("Animal Saved!");
          display.display();

          digitalWrite(GREEN_LED, HIGH);
          tone(BUZZER, 1500);

          delay(2000);
        }

        // ================= NOT FOUND =================
        else {

          display.clearDisplay();
          display.println("Not Found");
          display.println("Press Btn=Register");
          display.display();

          // 👉 WAIT BUTTON FOR REGISTER
          unsigned long start = millis();
          while (millis() - start < 4000) {
            if (digitalRead(BUTTON_PIN) == LOW) {

              HTTPClient regHttp;
              String regUrl = serverName + "?register=1&tagId=" + tagId;
              regHttp.begin(regUrl);
              int regCode = regHttp.GET();

              if (regCode == 200) {
                display.clearDisplay();
                display.println("Registered!");
                display.display();

                digitalWrite(GREEN_LED, HIGH);
                tone(BUZZER, 2000);
                delay(1500);
              }

              regHttp.end();
              break;
            }
          }
        }
      }
    }

    http.end();
  }

  // Reset
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  noTone(BUZZER);

  registerMode = false;

  delay(3000);
}
