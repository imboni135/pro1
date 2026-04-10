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
#define BUTTON_PIN 14

bool registerMode = false;
bool deleteMode = false;

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

  WiFi.begin(ssid, password);

  display.println("Connecting WiFi...");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  display.clearDisplay();
  display.println("WiFi Connected!");
  display.display();
}

void loop() {

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String tagId = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) tagId += "0";
    tagId += String(mfrc522.uid.uidByte[i], HEX);
  }
  tagId.toUpperCase();

  display.clearDisplay();
  display.println("Scanning...");
  display.display();

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    String url = serverName + "?tagId=" + tagId;

    http.begin(url);
    int code = http.GET();

    if (code == 200) {

      String payload = http.getString();
      DynamicJsonDocument doc(512);
      deserializeJson(doc, payload);

      String status = doc["status"];

      // ================= FOUND =================
      if (status == "found") {

        String name = doc["name"];

        display.clearDisplay();
        display.println("Name: " + name);
        display.println("Press Btn=Delete");
        display.display();

        // wait button
        unsigned long start = millis();
        while (millis() - start < 3000) {
          if (digitalRead(BUTTON_PIN) == LOW) {

            display.clearDisplay();
            display.println("Deleting...");
            display.display();

            HTTPClient delHttp;
            String delUrl = serverName + "?delete=1&tagId=" + tagId;
            delHttp.begin(delUrl);
            int delCode = delHttp.GET();

            if (delCode == 200) {
              display.clearDisplay();
              display.println("Deleted!");
              display.display();
              tone(BUZZER, 1000);
            }

            delHttp.end();
            delay(2000);
            break;
          }
        }
      }

      // ================= NOT FOUND =================
      else {

        display.clearDisplay();
        display.println("Not Found");
        display.println("Press Btn=Register");
        display.display();

        unsigned long start = millis();
        while (millis() - start < 5000) {
          if (digitalRead(BUTTON_PIN) == LOW) {

            display.clearDisplay();
            display.println("Registering...");
            display.display();

            HTTPClient regHttp;
            String regUrl = serverName + "?register=1&tagId=" + tagId;
            regHttp.begin(regUrl);
            int regCode = regHttp.GET();

            if (regCode == 200) {
              display.clearDisplay();
              display.println("Saved!");
              display.display();
              tone(BUZZER, 2000);
            }

            regHttp.end();
            delay(2000);
            break;
          }
        }
      }
    }

    http.end();
  }

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  noTone(BUZZER);

  delay(2000);
}
