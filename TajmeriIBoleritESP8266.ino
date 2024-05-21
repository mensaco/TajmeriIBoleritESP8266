/*

  Mundeson rregullimin e temperatures se bojlerit rrjedhes permes internetit si dhe
  zvogelimin e asaj temperature pas kalimit te nje numri te caktuar te minutave

  Complete project details: https://RandomNerdTutorials.com/esp8266-nodemcu-https-requests/ 
  Based on the BasicHTTPSClient.ino Created on: 20.08.2018 (ESP8266 examples)
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "C:\Users\MensuriWS\.arduinoIDE\WiFiCreds.h"



int L = D1;
int R = D2;

int INTERR_PIN = D6;

bool disable = false;
bool allowAllowance = true;


int DEGREE = 61;

int maxT = 60;
int minT = 20;

int REPEAT_TIME_MS = 2000;
unsigned long ALLOWANCE_TIME = 30 * 60 * 1000;  // kohezgjatja prej vendosjes se nje temperature > 20°C, deri te rivendosja automatike ne 20°C
unsigned long msCounter = 0;

// number of consecutive failures to connect to WiFi before reseting
int failures = 10;

String bearerToken = TOKEN;



void resetAllowance() {
  msCounter = millis();
  allowAllowance = true;
}

void IRAM_ATTR handleInterrupt() {
  resetAllowance();
}

void turnOff() {
  digitalWrite(R, LOW);
  digitalWrite(L, LOW);
}

void turnRight() {
  digitalWrite(R, HIGH);
  digitalWrite(L, LOW);
}

void turnLeft() {
  digitalWrite(L, HIGH);
  digitalWrite(R, LOW);
}

void turnRight(int grade) {
  int korrigjim = (maxT - grade) / 10 - 1;
  turnRight();
  delay((grade - minT + korrigjim) * DEGREE);
  turnOff();
}

void turnLeft(int grade) {
  int korrigjim = (maxT - grade) / 10 - 1;
  turnLeft();
  delay((grade - minT + korrigjim) * DEGREE);
  turnOff();
}

void setTemperatureOff() {
  turnLeft(60);
}

void setTemperature(int grade, bool dontResetAllowance = false) {
  detachInterrupt(INTERR_PIN);
  if (!disable) {
    if (grade < minT) grade = minT;
    if (grade > maxT) grade = maxT;

    setTemperatureOff();
    delay(500);
    turnRight(grade);
    if (!dontResetAllowance) {
      resetAllowance();
    }
  }
  attachInterrupt(INTERR_PIN, handleInterrupt, RISING);
}



void setup() {
  // setup pin orientation
  pinMode(R, OUTPUT);
  pinMode(L, OUTPUT);
  pinMode(2, OUTPUT);
  // turn off inbuilt led
  digitalWrite(2, HIGH);
  // turn off the motor
  turnOff();
  delay(100);



  Serial.begin(115200);
  //Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();



  //Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.println("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();

  resetAllowance();
  attachInterrupt(INTERR_PIN, handleInterrupt, RISING);
}

void getNewToken() {
  Serial.println("I need a new authentication token");
}


// the loop function runs over and over again forever
void loop() {
  if (allowAllowance) {
    if (millis() - msCounter > ALLOWANCE_TIME) {
      allowAllowance = false;      // prevent repeated subsequent execution of the following command
      setTemperature(minT, true);  // set to minimal temperature
    }
  }


  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    Serial.println("Connected");
    WiFiClientSecure client;

    // Ignore SSL certificate validation
    client.setInsecure();

    //create an HTTPClient instance
    HTTPClient https;

    String url = GET_BOILER_URL;
    Serial.println(url);

    //Initializing an HTTPS communication using the secure client
    if (https.begin(client, url)) {  // HTTPS

      https.addHeader("accept", "text/plain");
      https.addHeader("Authorization", "Bearer " + bearerToken);

      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        //Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();


          JsonDocument doc;

          DeserializationError error = deserializeJson(doc, payload);

          if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return;
          }

          //int data_0_id = doc["data"][0]["id"];        // 0
          int data_0_value = doc["data"][0]["value"];  // 0

          //bool succeeded = doc["succeeded"];  // true
          // doc["errors"] is null
          //const char* message = doc["message"];  // "skdjfh aldskjfh asdlkfjhasd lkfh asdlkfhasd fsladkjfh asldkhf ...

          if (data_0_value > 0) {
            setTemperature(data_0_value, true);
            Serial.println("setTemperature(data_0_value, true);");
            Serial.println(data_0_value);
          }

          delay(100);


          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  } else {
    failures = failures - 1;
    if (failures < 1) {
      ESP.restart();
    }
  }
  //Serial.println();
  //Serial.println("Waiting REPEAT_TIME_MS milliseconds before the next round...");
  delay(REPEAT_TIME_MS);
}
