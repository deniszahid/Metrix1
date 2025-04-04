#include <LedControl.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <ESP32Servo.h>

const char* ssid = "ROBO";
const char* password = "1234567890";

WebServer server(80);
int DIN = 23;
int CS = 15;
int CLK = 18;
LedControl lc = LedControl(DIN, CLK, CS, 2);

byte openEyes[8] = {
  B00000000,
  B00111100,
  B01000010,
  B01000010,
  B01000010,
  B01000010,
  B00111100,
  B00000000
};

byte closedEyes[8] = {
  B00000000,
  B00111100,
  B00111100,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000
};

Adafruit_BMP280 bmp;
Adafruit_AHTX0 aht;
#define SDA_PIN 21
#define SCL_PIN 22
const int mq2AnalogPin = 34;

const int fireSensorPin = 33;
const int laserPin = 32;
const int servoPin1 = 26;
const int servoPin2 = 27;

Servo servo1;
Servo servo2;

const int mq2MinValue = 0;
const int mq2MaxValue = 4095;

void handleRoot() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  float tempAHT20 = temp.temperature;
  float humAHT20 = humidity.relative_humidity;
  float tempBMP280 = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;
  int mq2Value = analogRead(mq2AnalogPin);
  float gasLevelPercent = map(mq2Value, mq2MinValue, mq2MaxValue, 0, 100);
  gasLevelPercent = constrain(gasLevelPercent, 0, 100);
  int fireDetected = digitalRead(fireSensorPin);


  String barColor = "#4caf50";
  if (gasLevelPercent > 70) barColor = "#ff0000";
  else if (gasLevelPercent > 50) barColor = "#ffa500";

  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Розвідувальні дані</title>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #74ABE2, #5563DE); color: #fff; margin: 0; padding: 20px; text-align: center; }";
  html += "h1, h2 { margin-bottom: 20px; }";
  html += "p { font-size: 18px; margin: 8px 0; }";
  html += "button { margin-top: 20px; padding: 10px 25px; font-size: 18px; border: none; border-radius: 10px; background: #ff9800; color: #fff; cursor: pointer; transition: background 0.3s; }";
  html += "button:hover { background: #e68a00; }";
  html += ".progress-container { width: 80%; background-color: #ddd; border-radius: 20px; margin: 20px auto; height: 25px; overflow: hidden; }";
  html += ".progress-bar { height: 100%; text-align: right; padding-right: 10px; line-height: 25px; color: white; border-radius: 20px; transition: width 0.5s; }";
  html += "</style></head><body>";
  html += "<h1>Metrix</h1>";
  html += "<h2>Дані сенсорів</h2>";
  html += "<p>🌡 BMP280 Температура: " + String(tempBMP280) + " °C</p>";
  html += "<p>🔵 Тиск: " + String(pressure) + " hPa</p>";
  html += "<p>🌡 AHT20 Температура: " + String(tempAHT20) + " °C</p>";
  html += "<p>💧 Вологість: " + String(humAHT20) + " %</p>";
  html += fireDetected == HIGH ? "<p style='color: #4caf50;'>✅ Вогню не виявлено</p>" : "<p style='color: #ff4c4c;'>🔥 Вогонь виявлено!</p>";

  if (gasLevelPercent > 50) {
    html += "<p style='color: #ff4c4c;'>⚠️ Рівень газу: " + String(gasLevelPercent) + " %</p>";
  } else {
    html += "<p style='color: #4caf50;'>✅ Рівень газу: " + String(gasLevelPercent) + " %</p>";
  }
  html += "<div class='progress-container'><div class='progress-bar' style='width: " + String(gasLevelPercent) + "%; background-color: " + barColor + ";'>" + String(gasLevelPercent) + "%</div></div>";
  html += "<button onclick=\"redirect()\">Керування Metrix</button>";
  html += "<script>function redirect() { window.location.href = 'http://192.168.4.1'; }</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void displayFace(byte face[8]) {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      bool pixel = bitRead(face[row], 7 - col);
      lc.setLed(0, row, col, pixel);
      lc.setLed(1, row, col, pixel);
    }
  }
}

void blinkSmoothly() {
  displayFace(openEyes);
  delay(1000);
  displayFace(closedEyes);
  delay(300);
  displayFace(openEyes);
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 2; i++) {
    lc.shutdown(i, false);
    lc.setIntensity(i, 8);
    lc.clearDisplay(i);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Підключення...");
  }
  Serial.println(WiFi.localIP());

  Wire.begin(SDA_PIN, SCL_PIN);
  aht.begin();
  bmp.begin(0x77);

  server.on("/", handleRoot);
  server.begin();
  pinMode(mq2AnalogPin, INPUT);
  pinMode(fireSensorPin, INPUT);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, HIGH);
  servo1.setPeriodHertz(50);
  servo1.attach(servoPin1);
  servo2.setPeriodHertz(50);
  servo2.attach(servoPin2);
  servo1.write(0);
  servo2.write(0);
  delay(500);
  servo1.write(180);
  servo2.write(180);
  delay(500);
  servo1.write(90);
  servo2.write(90);
}

void loop() {
  blinkSmoothly();
  server.handleClient();
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }
}
