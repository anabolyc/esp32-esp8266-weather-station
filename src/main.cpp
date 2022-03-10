#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>

#include "ani.h"
#include "Orbitron_Medium_20.h"

TFT_eSPI tft = TFT_eSPI();

#define TFT_GREY 0x5AEB
#define lightblue 0x01E9
#define darkred 0xA041
#define blue 0x5D9B

#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif

#include <WiFiUdp.h>
#include <NTPClient.h>

#define SCREEN_WIDTH (((TFT_ROTATION == 0) || (TFT_ROTATION == 2)) ? TFT_WIDTH : TFT_HEIGHT)
#define SCREEN_HEIGHT (((TFT_ROTATION == 0) || (TFT_ROTATION == 2)) ? TFT_HEIGHT : TFT_WIDTH)
#define SCREEN_HEIGHT5 (SCREEN_HEIGHT / 5)

// const int pwmFreq = 5000;
// const int pwmResolution = 8;
// const int pwmLedChannelTFT = 0;

const String endpoint = "https://api.openweathermap.org/data/2.5/weather?q=" OPEN_WEATHER_MAP_TOWN "," OPEN_WEATHER_MAP_COUNTRY "&units=metric&APPID=";

String payload = ""; //whole json
String tmp = "";     //temperatur
String hum = "";     //humidity

StaticJsonDocument<1024> doc;
HTTPClient http;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String curDayStamp;
String timeStamp;

int backlight[5] = {10, 30, 60, 120, 220};
byte b = 1;

int i = 0;
String tt = "";
int count = 0;
bool inv = 1;
int press1 = 0;
int press2 = 0; ////

int frame = 0;
String curSeconds = "";

void getData()
{
  if ((WiFi.status() == WL_CONNECTED))
  {
    Serial.printf(" -> %s\n", (endpoint + OPEN_WEATHER_MAP_API_KEY).c_str());
#ifdef ESP32
    http.begin(endpoint + OPEN_WEATHER_MAP_API_KEY);
#endif
#ifdef ESP8266
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    http.begin(*client, endpoint + OPEN_WEATHER_MAP_API_KEY);
#endif

    int httpCode = http.GET();
    if (httpCode > 0)
    {
      payload = http.getString();
      Serial.printf(" <- %s\n", payload.c_str());
    }
    else
    {
      Serial.printf("Error on HTTP request: %d\n", httpCode);
    }

    char inp[1024];
    payload.toCharArray(inp, 1024);
    deserializeJson(doc, inp);
    // deserializeJson(doc, *client);

    http.end();
  }

  String tmp2 = doc["main"]["temp"];
  String hum2 = doc["main"]["humidity"];
  String town2 = doc["name"];
  tmp = tmp2;
  hum = hum2;  

  Serial.printf("Temperature: %s\n", tmp.c_str());
  Serial.printf("Humidity: %s\n", hum.c_str());
  Serial.printf("Town: %s\n", OPEN_WEATHER_MAP_TOWN);
}

void updateTime()
{
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();

  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
}

void render()
{
  tft.pushImage(SCREEN_WIDTH - ANI_WIDTH, 0, ANI_WIDTH, ANI_HEIGHT, ani[frame++ % ANI_FRAMES]);

  tft.setFreeFont(&Orbitron_Medium_20);
  tft.setCursor(2, SCREEN_HEIGHT5 * 3 + 40);
  tft.fillRect(2, SCREEN_HEIGHT5 * 3 + 20, 64, 20, TFT_BLACK);
  tft.println(tmp.substring(0, 4));

  tft.setCursor(2, SCREEN_HEIGHT5 * 4 + 40);
  tft.fillRect(2, SCREEN_HEIGHT5 * 4 + 20, 64, 20, TFT_BLACK);
  tft.println(hum + "%");

  if (curDayStamp != dayStamp)
  {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setTextFont(2);
    tft.setCursor(6, SCREEN_HEIGHT5);
    tft.fillRect(6, SCREEN_HEIGHT5, SCREEN_WIDTH >> 1, 16, TFT_BLACK);
    tft.println(dayStamp);
    curDayStamp = dayStamp;
  }

  if (curSeconds != timeStamp.substring(6, 8))
  {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setFreeFont(&Orbitron_Light_24);
    tft.fillRect(SCREEN_WIDTH >> 1, SCREEN_HEIGHT5 * 3 + 20, 48, 28, TFT_BLACK);
    tft.setCursor(SCREEN_WIDTH >> 1, SCREEN_HEIGHT5 * 3 + 40);
    tft.println(timeStamp.substring(6, 8));
    curSeconds = timeStamp.substring(6, 8);
  }

  tft.setFreeFont(&Orbitron_Light_32);
  String current = timeStamp.substring(0, 5);
  if (current != tt)
  {
    Serial.printf("Date: %s\n", formattedDate.c_str());
    tft.fillRect(0, 8, SCREEN_WIDTH - ANI_WIDTH, 32, TFT_BLACK);
    tft.setCursor(5, 32 + 8);
    tft.println(timeStamp.substring(0, 5));
    tt = timeStamp.substring(0, 5);
  }
}

void setup(void)
{
  pinMode(0, INPUT_PULLUP);
  pinMode(35, INPUT);

  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  // ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  // ledcAttachPin(TFT_BL, pwmLedChannelTFT);
  // ledcWrite(pwmLedChannelTFT, backlight[b]);

  Serial.begin(SERIAL_BAUD);
  tft.print("Connecting to ");
  tft.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    tft.print(".");
  }

  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(TZ_OFFSET_SEC);

  tft.println("");
  tft.println("WiFi connected.");
  tft.println("IP address: ");
  tft.println(WiFi.localIP());
  delay(3000);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

  // tft.setCursor(2, SCREEN_HEIGHT - 8, 1);
  // tft.println(WiFi.localIP());

  tft.setCursor(SCREEN_WIDTH >> 1, SCREEN_HEIGHT5 * 4, 1);
  tft.println("BRIGHT:");

  tft.setCursor(SCREEN_WIDTH >> 1, SCREEN_HEIGHT5 * 3, 2);
  tft.println("SEC:");
  tft.setTextColor(TFT_WHITE, lightblue);
  tft.setCursor(4, SCREEN_HEIGHT5 * 3, 2);
  tft.println("TEMP:");

  tft.setCursor(4, SCREEN_HEIGHT5 * 4, 2);
  tft.println("HUM: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setFreeFont(&Orbitron_Medium_20);
  tft.setCursor(6, SCREEN_HEIGHT5 * 2);
  tft.println(OPEN_WEATHER_MAP_TOWN);

  // tft.fillRect(SCREEN_WIDTH >> 1, SCREEN_HEIGHT5 * 4, 1, 74, TFT_GREY);
  for (int i = 0; i < b + 1; i++)
    tft.fillRect((SCREEN_WIDTH >> 1) + (i * 7), SCREEN_HEIGHT5 * 4 + 8, 3, 10, blue);

  delay(500);
}

void loop()
{
  frame++;

  if (digitalRead(35) == 0)
  {
    if (press2 == 0)
    {
      press2 = 1;
      tft.fillRect(78, 216, 44, 12, TFT_BLACK);

      b++;
      if (b >= 5)
        b = 0;

      for (int i = 0; i < b + 1; i++)
        tft.fillRect((SCREEN_WIDTH >> 1) + (i * 7), SCREEN_HEIGHT5 * 4 + 8, 3, 10, blue);
      // ledcWrite(pwmLedChannelTFT, backlight[b]);
    }
  }
  else
    press2 = 0;

  if (digitalRead(0) == 0)
  {
    if (press1 == 0)
    {
      press1 = 1;
      inv = !inv;
      tft.invertDisplay(inv);
    }
  }
  else
    press1 = 0;

  if (count == 0)
    getData();
  count++;
  if (count > 2000)
    count = 0;

  updateTime();
  render();

  delay(100);
}