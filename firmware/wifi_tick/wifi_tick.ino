/*
 * esp8266_ohai.ino - Something of a WiFi Hello World.
 *
 */

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <FS.h>
#include <LedControl.h>

#include <RTClib.h>
#include <DS3231.h>
#include <Time.h>
#include <Timezone.h>
#include <NTPClient.h>

#include <string.h>

#include "debug.h"
#include "wifi_setup.h"

// Pin assignments

#define DISP_DATA       13
#define DISP_CLCK       14
#define DISP_LOAD       16
#define DISP_NUM        1
#define DISP_BRIGHTNESS 8

//
// TODO Define these for yourself!
//
const char *MY_WIFI_AP_NAME = "<SSID>";
const char *MY_WIFI_AP_KEY  = "<KEY>";

//
// Global controls/vars
//

LedControl display = LedControl(DISP_DATA, DISP_CLCK, DISP_LOAD, DISP_NUM);
RTC_DS3231 rtc = RTC_DS3231();
NTPClient ntpclient;

void setup_serial() {
  delay(200);
  Serial.begin(SERIAL_BAUD);
  delay(10);
}

void setup() {

#ifdef DEBUG
  setup_serial();
#endif

  //
  // Mount config filesystem
  //
  if (!SPIFFS.begin()) {
    DPRINTLN("Failed to mount config fs!");
    return;
  }

  //
  // Config probably doesn't exist, so try to save it to SPIFFS
  //

  if (!setup_wifi()) {

    size_t len_name = strlen(MY_WIFI_AP_NAME);
    wifi_ap_name = new char[len_name+1];
    strncpy(wifi_ap_name, MY_WIFI_AP_NAME, len_name);
    if (save_ap_name()) {
      DPRINTLN("Saved wifi_ap_name");
    } else {
      DPRINTLN("Failed to save wifi_ap_name!");
    }

    size_t len_key  = strlen(MY_WIFI_AP_KEY);
    wifi_ap_key = new char[len_key+1];
    strncpy(wifi_ap_key, MY_WIFI_AP_KEY, len_key);
    if (save_ap_key()) {
      DPRINTLN("Saved wifi_ap_key");
    } else {
      DPRINTLN("Failed to save wifi_ap_key!");
    }

  }

  //
  // Start I2C & RTC
  //
  Wire.begin();
  rtc.begin();
  rtc.clearControlRegisters();

  //
  // Setup display(s)
  //

  for (int i=0; i<DISP_NUM; i++) {
    display.shutdown(i, false);
    display.setIntensity(i, DISP_BRIGHTNESS);
    display.clearDisplay(0);
  }

  //
  // Connect to Wifi
  //

  connect_wifi();

  delay(500);

}

void loop() {

  for (int i=0; i<10; i++) {
    display.clearDisplay(0);
    display.setDigit(0, 0, i, true);
    delay(500);
  }

  // Has its own internal check to only poll every 60s
  ntpclient.update();

  uint32_t ntp_time = ntpclient.getRawTime();
  uint32_t rtc_time = rtc.now().unixtime();

  DateTime ntp_dt = DateTime(ntp_time);
  DateTime rtc_dt = DateTime(rtc_time);

  // Check if RTC is far enough off to require adjustment
  if (rtc_time > ntp_time || ntp_time - rtc_time > 60) {
    rtc.adjust(ntp_dt);
  }

  // Fart out timestamp
  char buff[64];
  ntp_dt.toString(buff, sizeof(buff));
  DPRINT(buff);
  DPRINT(" ");
  rtc_dt.toString(buff, sizeof(buff));
  DPRINTLN(buff);

}
