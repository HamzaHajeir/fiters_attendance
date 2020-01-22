#include <Arduino.h>
#include <ESP8266WiFi.h>

#if 1

void setup(){
    Serial.begin(115200);

    WiFi.begin("My ASUS", "123456789");

    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
}

void loop(){

}

#else
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
ESP8266WiFiMulti WiFiMulti;
#define IP  "192.168.1.13"

#include <WiFiClient.h>

#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16,2);


#define RST_PIN D3  // Configurable, see typical pin layout above
#define SS_PIN D8   // Configurable, see typical pin layout above
#define G_LIGHT D0
#define R_LIGHT D4
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

const int cardSize = 2;
String cardsArray[cardSize];


int show = -1;

void printResult(String message, String message2) {
    lcd.clear();  // go home
    lcd.print(message);
    lcd.setCursor(0, 1);  // go to the next line
    lcd.print(message2);
}


void setup() {
  // put your setup code here, to run once:
  pinMode(R_LIGHT, OUTPUT);
  pinMode(G_LIGHT, OUTPUT);

  cardsArray[0] = "03 FE AD 1B";
  cardsArray[1] = "93 F2 1C 1A";

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 1; t > 0; t--) {
      Serial.printf("[SETUP] WAIT %d...\n", t);
      Serial.flush();
      delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("Saif's iPhone", "12345678912");
//   WiFiMulti.addAP("ORANGE_1", "123456789");

  SPI.begin();  // Init SPI bus

  mfrc522.PCD_Init();                 // Init MFRC522 card
  delay(4);                           // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();

  printResult("", "Scan your ID");
}

void loop() {
  // put your main code here, to run repeatedly:
  boolean isPresent=false;
  boolean isReadCard = false;

  boolean isRegistered = false;
  isPresent = mfrc522.PICC_IsNewCardPresent();
  // Select one of the cards
  if (isPresent) {
      isReadCard = mfrc522.PICC_ReadCardSerial();
      if(!isReadCard)  {
        WiFiMulti.run();
          return;
      }
  }else {
      WiFiMulti.run();
      return;
  }

  String content = "";

  Serial.println(mfrc522.uid.size);
 
  for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println("");
  Serial.println("MESSAGE : ");
  content.toUpperCase();
  Serial.println(content);
  bool checkCard = false;

  for (int i = 0; i < cardSize; i++) {
      if (content.indexOf(cardsArray[i]) >= 0) {
          checkCard = true;
          break;
      }
  }
  if (checkCard) {
      Serial.println("AUTHERIZED");
      isRegistered = true;

      printResult("Welcome", "");
      digitalWrite(G_LIGHT, HIGH);
      delay(1000);
      digitalWrite(G_LIGHT, LOW);
      delay(1000);
      printResult("", "Scan your ID");
  } else {
      Serial.println("NOT AUTHERIZED");
      
      printResult("You are not in", "this course");
      digitalWrite(R_LIGHT, HIGH);
      delay(300);
      delay(1000);
      digitalWrite(R_LIGHT, LOW);
      delay(1000);
      printResult("", "Scan your ID");
  }

  if ((WiFiMulti.run() == WL_CONNECTED) && isRegistered) {
      isRegistered = false;
      Serial.println("");
      Serial.print("Connected! IP address: ");
      Serial.println(WiFi.localIP());

      WiFiClient client;

      HTTPClient http;

      Serial.print("[HTTP] begin...\n");
      http.begin(client, IP, 3002, "/api/attendance/confirm/AX32B7");  //HTTP
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Accept", "application/json");
      http.addHeader("Connection", "keep-alive");
      // http.addHeader("Content-Disposition", "form-data; name=\"code\"");

      Serial.print("[HTTP] POST...\n");
      // start connection and send HTTP header
      int httpCode = http.POST("");
      // int httpCode = http.POST("Hello world!");

      // httpCode will be negative on error
      if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTP] POST... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              String payload = http.getString();
              Serial.println(payload);
          
          } else {
              Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
          }

          http.end();
      } else {
          Serial.printf("[HTTP} Unable to connect\n");
      }
      delay(500);
  }
}

#endif