/*
  raspberry pi pico w + 2.23inch OLED Display clock
  
  Raspberry Pi Pico用 2.23インチ OLEDディスプレイ 128×32
  https://www.switch-science.com/products/7329
  
  ボード:Raspberry Pi Pico W
  
*/
#include "devtools.h"

#include <WiFi.h>

// WiFi credentials.
const char* WIFI_SSID = "E426867F5D3B-2G";
const char* WIFI_PASS = "gapf8fcpgfd2e8";

void wifi_connect() {
	//Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while( WiFi.status() != WL_CONNECTED) {
    delay(500);
    D_print(".");
  }
	D_println(" CONNECTED");
	
  D_print("IP:");
	D_println(WiFi.localIP());
	
}

void wifi_disconnect() {  // なぜか切断されずにアイドル状態になる
  WiFi.mode(WIFI_OFF);
  WiFi.disconnect(true);
}

// NTP
#define JST_9   60 * 60 * 9; // JST-9   60sec * 60min * 9hour
// Ref. https://arduino-pico.readthedocs.io/en/latest/wifintp.html
time_t get_Ntp_Time() {

  NTP.begin("ntp.nict.jp", "time.asia.apple.com");

  D_print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    D_print("*");
    now = time(nullptr);
  }
  D_println("");
  
  now += JST_9
  
  return now;
}




// RTC
#include "defines.h"
#include <Timezone_Generic.h> 

void set_rtc(time_t now) {  
  struct tm *ts;
  ts = gmtime(&now);

  datetime_t currTime;

  currTime = {
  ts->tm_year + 1900,
  ts->tm_mon + 1,
  ts->tm_mday,
  ts->tm_wday,
  ts->tm_hour,
  ts->tm_min,
  ts->tm_sec
  };
   
  rtc_set_datetime(&currTime);
}

time_t get_rtc_time() {
  datetime_t currTime;
  
  rtc_get_datetime(&currTime);
  DateTime now = DateTime(currTime);

  return now.get_time_t();
}


#include <string.h>
void time_to_datetime(time_t currenttime, char *datetime) {

  struct tm *ts;
  ts = gmtime(&currenttime);
  sprintf(datetime, "%4d%2d%02d%2d%02d%02d\n", ts->tm_year+ 1900, ts->tm_mon, ts->tm_mday,
    ts->tm_hour, ts->tm_min, ts->tm_sec);
  D_println(datetime);
  
}

char *datetime_to_hhmmr(char *datetime, char *hhmm)
{  
  char hour[3], min[3];
  strncpy(hour, &datetime[8], 2);
  strncpy(min, &datetime[10], 2);

  time_t t = get_rtc_time();

  struct tm *ts;
  ts = gmtime(&t);

  char colon;
  static bool bl = true;
  if(bl) colon = ':';
  else colon = ' ';
  bl = !bl;
  
  sprintf(hhmm, "%2s%c%02s", hour, colon, min);
  return(hhmm);
}

void ntp_synchro() {
  D_println("wifi_connect");
  wifi_connect();

  D_println("get_Ntp_Time");
  time_t now = get_Ntp_Time();

  D_println("set_rtc");
  set_rtc(now);
  
  D_println("wifi_disconnect");
  wifi_disconnect();
}



//2.23inch OLED Display
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>

// Used for software SPI
#define OLED_CLK 10
#define OLED_MOSI 11

// Used for software or hardware SPI
#define OLED_CS 9
#define OLED_DC 8

// Used for I2C or SPI
#define OLED_RESET 12

// software SPI
Adafruit_SSD1305 display(128, 32, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

void lcd_disp(char *s) {
  display.print(s);
  display.display();
}

void lcd_dispxy(int x, int y, char *s) {
  display.clearDisplay();
  display.setCursor(x,y);
  lcd_disp(s);
  display.display();
}


void setup()   {                
  D_SerialBegin(115200);
  D_println("SSD1305 OLED test");
  

  if ( ! display.begin(0x3C) ) {
     D_println("Unable to initialize OLED");
     while (1) yield();
  }

  // init done
  display.display(); // show splashscreen
  delay(1000);
  display.setRotation(2);
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.clearDisplay();   // clears the screen and buffer
  
  // Start the RTC
  D_println("rtc_init");
  rtc_init();

  ntp_synchro();

}


#define XPOS 5
#define YPOS 0
#define UPDATE_TIME   " 300"      // 3:00 NTPで時刻合わせする時刻
#define UPDATE_TIMER  3600 * 24   // 24hour
void loop() {
  static time_t UpdateTimer;
  time_t currenttime;
  char datetime[32], hhmm[32];
  
  currenttime = get_rtc_time();
  
  time_to_datetime(currenttime, datetime);
  datetime_to_hhmmr(datetime, hhmm);
  
  if(strncmp(&datetime[8], UPDATE_TIME, 4) ==0) {
    if(difftime(currenttime, UpdateTimer) > UPDATE_TIMER) {
      ntp_synchro();
      UpdateTimer = currenttime;
    }
  }
  
  display.clearDisplay();
  display.setCursor(XPOS, YPOS);
  display.println(hhmm);
  display.display();

  delay(1000);
}
