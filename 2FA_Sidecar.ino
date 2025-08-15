// 2FA Sidecar
// Matt Perkins & fujiba - Copyright (C) 2024-2025
// Spawned out of the need to often type a lot of two factor authentication
// but still have some security while remaning mostly isolated from the host
// system. See github for 3D models and wiring diagram.
/*

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/
// --- CHOOSE DEVICE MODE ---
#define MODE_5_KEY 5
#define MODE_2_KEY 2
// Set to MODE_5_KEY or MODE_2_KEY
#define DEVICE_MODE MODE_2_KEY
// --------------------------

#define NUM_KEYS DEVICE_MODE

// Change for your country.
#define NTP_SERVER "ntp.jst.mfeed.ad.jp"
#define TZ "JST-9"

const char* mainver = "1.50f";  // Version updated

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Arduino.h>
#include <PinButton.h>
#include <Preferences.h>
#include <SPI.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

#include <string>

#include "Fonts/FreeMono12pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans9pt7b.h"
#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#endif
#include <ArduinoOTA.h>
// #include <ESPAsyncWebSrv.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <lwip/apps/sntp.h>

#include <TOTP-RC6236-generator.hpp>

USBHIDKeyboard Keyboard;

const int NUM_BANKS = 3;
int current_bank = 0;

// Existing key buttons (maintaining PinButton)
PinButton key1(5);
PinButton key2(6);
#if NUM_KEYS == 5
PinButton key3(9);
PinButton key4(10);
PinButton key5(11);
#endif

// Direct control variables for bank switching buttons
bool prev_d0_state = HIGH;  // D0 initial state is HIGH (pull-up)
bool prev_d1_state = LOW;   // D1 initial state is LOW (pull-down)
bool prev_d2_state = LOW;   // D2 initial state is LOW (pull-down)
unsigned long last_d0_change = 0;
unsigned long last_d1_change = 0;
unsigned long last_d2_change = 0;
const unsigned long DEBOUNCE_DELAY = 50;
bool bank_buttons_initialized = false;

int bargraph_pos;
int updateotp;
long time_x;
int sline = 0;
int pinno = 0;
String in_pin;
int pindelay = 3000;

// AsyncWebServer server(80);
WebServer server(80);

String ssid = "Key-Sidecar";
String password;
String pin;

String tfa_name[NUM_BANKS][NUM_KEYS];
String tfa_seed[NUM_BANKS][NUM_KEYS];

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Preferences preferences;

// Function for direct button control (different detection method for each pin)
bool checkButtonPress(int pin, bool& prev_state, unsigned long& last_change) {
  bool current_state = digitalRead(pin);
  unsigned long current_time = millis();

  // If the state has changed
  if (current_state != prev_state &&
      (current_time - last_change > DEBOUNCE_DELAY)) {
    last_change = current_time;

    bool button_pressed = false;

    if (pin == 0) {
      // D0 is pull-up: button press on HIGH to LOW change
      button_pressed = (prev_state == HIGH && current_state == LOW);
    } else {
      // D1, D2 are pull-down: button press on LOW to HIGH change
      button_pressed = (prev_state == LOW && current_state == HIGH);
    }

    prev_state = current_state;

    if (button_pressed) {
      Serial.printf("Button on pin %d pressed!\n", pin);
      return true;
    }
  }

  return false;
}

void initializeBankButtons() {
  Serial.println("Initializing bank buttons...");

  // For debugging: check pin states
  Serial.println("--- Pin states before initialization ---");
  pinMode(0, INPUT);
  pinMode(1, INPUT);
  pinMode(2, INPUT);
  Serial.printf("D0 (floating): %d\n", digitalRead(0));
  Serial.printf("D1 (floating): %d\n", digitalRead(1));
  Serial.printf("D2 (floating): %d\n", digitalRead(2));

  // Set pin modes (according to hardware wiring)
  pinMode(0, INPUT_PULLUP);    // D0 is pull-up
  pinMode(1, INPUT_PULLDOWN);  // D1 is pull-down
  pinMode(2, INPUT_PULLDOWN);  // D2 is pull-down

  // Wait a bit for the pins to stabilize
  delay(100);

  // Read and record the initial states
  prev_d0_state = digitalRead(0);
  prev_d1_state = digitalRead(1);
  prev_d2_state = digitalRead(2);

  Serial.printf("Initial states - D0: %d, D1: %d, D2: %d\n", prev_d0_state,
                prev_d1_state, prev_d2_state);

  // Initialize the change timestamps
  last_d0_change = millis();
  last_d1_change = millis();
  last_d2_change = millis();

  bank_buttons_initialized = true;
  Serial.println("Bank buttons initialized successfully.");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("2FA Sidecar Booting...");

  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);

  delay(250);
  Serial.println("TFT Power ON");

  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(10);
  digitalWrite(TFT_RST, HIGH);
  delay(10);
  Serial.println("Manual TFT Reset performed.");

  tft.init(135, 240);
  Serial.println("TFT Initialized");

  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setFont(&FreeSans9pt7b);

  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.printf("\n2FA-Sidecar Ver %s\nBy Matt Perkins & fujiba\n", mainver);
  tft.printf("Press K1 to enter config/test\n");
  Serial.println("Boot screen displayed. Waiting for K1 press...");

  int lcount = 0;
  while (lcount < 140) {
    key1.update();
    if (key1.isClick()) {
      Serial.println("K1 pressed, entering setup_test()...");
      setup_test();
      ESP.restart();
    }
    tft.printf(".");
    delay(10);
    lcount++;
  }

  Serial.println("Continuing to normal boot...");
  tft.setFont(&FreeSans9pt7b);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(1, 15);
  tft.printf("2FA-Sidecar V%s - startup\n", mainver);

  preferences.begin("2FA_Sidecar", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  pin = preferences.getString("pin", "");
  Serial.println("Preferences loaded.");

  for (int b = 0; b < NUM_BANKS; b++) {
    for (int k = 0; k < NUM_KEYS; k++) {
      String name_key = "tfa_name_" + String(b) + "_" + String(k);
      String seed_key = "tfa_seed_" + String(b) + "_" + String(k);
      tfa_name[b][k] = preferences.getString(name_key.c_str(), "");
      tfa_seed[b][k] = preferences.getString(seed_key.c_str(), "");
    }
  }
  Serial.println("All bank settings loaded.");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  sline = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    tft.printf("Establishing WiFi\n");
    sline = sline + 1;
    if (sline > 4) {
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(3, 5);
      sline = 0;
    }
  }
  Serial.println(" Connected!");
  tft.print("IP: ");
  tft.println(WiFi.localIP());
  tft.print("Wifi: ");
  tft.print(WiFi.RSSI());

  configTzTime(TZ, NTP_SERVER);
  tft.println();
  tft.printf("NTP started:%s", TZ);
  time_t t = time(NULL);
  tft.printf(":%lld", t);
  tft.println();
  Serial.println("NTP configured.");

  const char* npin = pin.c_str();
  if (strlen(npin) > 3) {
    Serial.println("PIN is set, waiting for input.");
    tft.setCursor(25, 25);
    tft.setFont(&FreeSans12pt7b);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("PIN?");

    while (1) {
      key1.update();
      key2.update();
#if NUM_KEYS == 5
      key3.update();
      key4.update();
      key5.update();
#endif

      if (key1.isClick()) {
        pinno++;
        in_pin += "1";
        tft.print("*");
      }
      if (key2.isClick()) {
        pinno++;
        in_pin += "2";
        tft.print("*");
      }
#if NUM_KEYS == 5
      if (key3.isClick()) {
        pinno++;
        in_pin += "3";
        tft.print("*");
      }
      if (key4.isClick()) {
        pinno++;
        in_pin += "4";
        tft.print("*");
      }
      if (key5.isClick()) {
        pinno++;
        in_pin += "5";
        tft.print("*");
      }
#endif

      if (pinno == 4) {
        if (in_pin == npin) {
          Serial.println("PIN correct.");
          tft.println();
          tft.println("Correct.");
          break;
        } else {
          Serial.println("PIN incorrect, restarting.");
          tft.println();
          tft.print("Incorrect!");
          delay(pindelay);
          ESP.restart();
        }
      }
    }
  }

  tft.setTextColor(ST77XX_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.println("Iniz USB keybaord\n");
  Keyboard.begin();
  USB.begin();
  delay(2000);

  // Initialize bank switching buttons here
  initializeBankButtons();

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  updateotp = 1;
  Serial.println("Setup complete, entering loop().");
}

void loop() {
  // Update existing key buttons (maintaining PinButton)
  key1.update();
  key2.update();
#if NUM_KEYS == 5
  key3.update();
  key4.update();
  key5.update();
#endif

  // Direct control of bank switching buttons
  bool bank_changed = false;
  if (bank_buttons_initialized) {
    if (checkButtonPress(0, prev_d0_state, last_d0_change) &&
        current_bank != 0) {
      current_bank = 0;
      bank_changed = true;
      Serial.println("Bank 1 Selected");
    }
    if (checkButtonPress(1, prev_d1_state, last_d1_change) &&
        current_bank != 1) {
      current_bank = 1;
      bank_changed = true;
      Serial.println("Bank 2 Selected");
    }
    if (checkButtonPress(2, prev_d2_state, last_d2_change) &&
        current_bank != 2) {
      current_bank = 2;
      bank_changed = true;
      Serial.println("Bank 3 Selected");
    }
  }

  if (bank_changed) {
    updateotp = 1;
  }

  time_t t = time(NULL);
  if (t < 1000000) {
    delay(100);
    return;
  };

  static time_t last_t = 0;
  if (t != last_t) {
    last_t = t;

    bargraph_pos = (t % 30);
    bargraph_pos = map(bargraph_pos, 0, 29, 0, 230);

    tft.fillRect(0, 122, 240, 6, ST77XX_BLACK);
    tft.fillRect(5, 125, bargraph_pos, 3,
                 bargraph_pos < 200 ? ST77XX_GREEN : ST77XX_RED);

    if ((t % 30) == 0) {
      updateotp = 1;
    }
  }

  static bool first_run = true;
  if (updateotp == 1) {
    updateotp = 0;

    bool full_redraw = bank_changed || first_run;

    if (full_redraw) {
      first_run = false;
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(3, 17);
      tft.setTextColor(ST77XX_CYAN);
      tft.setFont(&FreeSans9pt7b);
      tft.printf("Bank %d", current_bank + 1);
    }

    for (int i = 0; i < NUM_KEYS; i++) {
      int y_pos = 40 + (i * 20);
      if (String* otp = TOTP::currentOTP(tfa_seed[current_bank][i])) {
        if (tfa_name[current_bank][i].length() > 0) {
          if (full_redraw) {
            tft.setCursor(3, y_pos);
            tft.setTextColor(ST77XX_RED);
            tft.setFont(&FreeSans12pt7b);
            tft.print(tfa_name[current_bank][i]);
          }

          tft.fillRect(140, y_pos - 15, 100, 20, ST77XX_BLACK);
          tft.setCursor(140, y_pos);
          tft.setTextColor(ST77XX_YELLOW);
          tft.setFont(&FreeMono12pt7b);
          tft.println(*otp);
        }
      } else {
        if (current_bank == 0 && i == 0 && tfa_seed[0][0].length() == 0) {
          tft.setCursor(3, 40);
          tft.setTextColor(ST77XX_RED);
          tft.setFont(&FreeSans12pt7b);
          tft.print("NO VALID CONFIG");
          delay(10000);
          ESP.restart();
        }
        if (full_redraw) {
          tft.setCursor(3, y_pos);
          tft.setTextColor(ST77XX_RED);
          tft.setFont(&FreeSans12pt7b);
          tft.print("UNSET");
        }
      }
    }
  }

  auto sendOTP = [&](int key_index) {
    String* otp = TOTP::currentOTP(tfa_seed[current_bank][key_index]);
    if (otp && tfa_seed[current_bank][key_index].length() > 0) {
      pinMode(LED_BUILTIN, OUTPUT);
      digitalWrite(LED_BUILTIN, HIGH);
      Keyboard.println(*otp);
      digitalWrite(LED_BUILTIN, LOW);
    }
  };

  if (key1.isClick()) sendOTP(0);
  if (key2.isClick()) sendOTP(1);
#if NUM_KEYS == 5
  if (key3.isClick()) sendOTP(2);
  if (key4.isClick()) sendOTP(3);
  if (key5.isClick()) sendOTP(4);
  if (key5.isLongClick()) ESP.restart();
#endif

  delay(10);
}