// 2FA Sidecar
// Matt Perkins & T.Fujiba - Copyright (C) 2024
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

const char HTML_HEADER[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='ja'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>2FA-Sidecar Config</title>
</head>
<body>
    <h2>2FA-Sidecar Configuration</h2>
    <p>(C) 2024 Matt Perkins & T.Fujiba - GPL</p>
    <hr>
    <p><i>Fill in only the fields you wish to set or update.</i></p>
    <form action='/get'>
        <h3>General Settings</h3>
        <p>SSID: <input type='text' name='ssid'></p>
        <p>WiFi Password: <input type='text' name='password'></p>
        <p>Access PIN (4 digits): <input type='text' name='pin'></p>
        <hr>
)rawliteral";

const char HTML_FOOTER[] PROGMEM = R"rawliteral(
        <br>
        <input type='submit' value='Save All Settings' style='font-size: 1.2em; padding: 10px;'>
    </form>
</body>
</html>
)rawliteral";

void setup_test() {
  PinButton key1(5);
  PinButton key2(6);
#if NUM_KEYS == 5
  PinButton key3(9);
  PinButton key4(10);
  PinButton key5(11);
#endif

  int keytest_flags = 0;
  int pass_condition = (1 << NUM_KEYS) - 1;  // For 2 keys: 3, for 5 keys: 31

  tft.fillScreen(ST77XX_BLACK);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(ST77XX_WHITE);

  const char* key_names[] = {"Key 1", "Key 2", "Key 3", "Key 4", "Key 5"};
  for (int i = 0; i < NUM_KEYS; i++) {
    tft.setCursor(3, 18 + (i * 20));
    tft.print(key_names[i]);
  }

  while (1) {
    key1.update();
    key2.update();
#if NUM_KEYS == 5
    key3.update();
    key4.update();
    key5.update();
#endif

    if (key1.isClick()) {
      tft.setCursor(65, 18);
      tft.print("OK");
      keytest_flags |= 1;
    }
    if (key2.isClick()) {
      tft.setCursor(65, 38);
      tft.print("OK");
      keytest_flags |= 2;
    }
#if NUM_KEYS == 5
    if (key3.isClick()) {
      tft.setCursor(65, 58);
      tft.print("OK");
      keytest_flags |= 4;
    }
    if (key4.isClick()) {
      tft.setCursor(65, 78);
      tft.print("OK");
      keytest_flags |= 8;
    }
    if (key5.isClick()) {
      tft.setCursor(65, 98);
      tft.print("OK");
      keytest_flags |= 16;
    }
#endif

    if (keytest_flags == pass_condition) {
      tft.setCursor(105, 58);
      tft.print("Test Pass");
      delay(2000);
      tft.fillScreen(ST77XX_RED);
      delay(500);
      tft.setTextColor(ST77XX_WHITE);
      break;
    }
  }
  wifi_setup();
}

void wifi_setup() {
  Serial.println("Entering wifi_setup()...");
  WiFi.mode(WIFI_AP);
  Serial.println("WiFi mode set to AP.");

  WiFi.softAP(ssid);
  Serial.println("Soft AP started.");

  tft.setCursor(3, 35);
  tft.fillScreen(ST77XX_RED);
  tft.printf("Connect via Wifi\nSSID:%s\nthen browse to \nhttp://192.168.4.1",
             ssid.c_str());
  Serial.println("TFT message displayed.");

  Serial.println("Setting up server routes...");
  server.on("/", HTTP_GET, []() {
    String html = FPSTR(HTML_HEADER);
    for (int b = 0; b < NUM_BANKS; b++) {
      html += "<h3>Bank " + String(b + 1) + "</h3>";
      for (int k = 0; k < NUM_KEYS; k++) {
        char name_key[20];
        char seed_key[20];
        sprintf(name_key, "tfa_name_%d_%d", b, k);
        sprintf(seed_key, "tfa_seed_%d_%d", b, k);
        html += "<p>Key " + String(k + 1) + " Name: <input type='text' name='" +
                name_key + "'></p>";
        html += "<p>Key " + String(k + 1) + " Seed: <input type='text' name='" +
                seed_key + "'></p>";
      }
      html += "<hr>";
    }
    html += FPSTR(HTML_FOOTER);
    server.send(200, "text/html", html);
  });
  Serial.println("'/' route configured.");

  server.on("/get", HTTP_GET, []() {
    Serial.println("Received GET request to save settings...");
    preferences.begin("2FA_Sidecar", false);
    String log = "";
    int saved_count = 0;

    for (int i = 0; i < server.args(); i++) {
      if (server.arg(i).length() > 0) {
        preferences.putString(server.argName(i).c_str(), server.arg(i));
        log += server.argName(i) + ", ";
        saved_count++;
      }
    }

    if (log.length() > 2) {
      log = log.substring(0, log.length() - 2);
    }
    Serial.printf("%d parameters saved.\n", saved_count);

    tft.fillScreen(ST77XX_BLUE);
    tft.setCursor(5, 30);
    tft.printf("%d parameters saved.\nRebooting...", saved_count);

    String response_html =
        "<!DOCTYPE html><html><head><title>Saved</title><meta "
        "http-equiv='refresh' content='3;url=/'></head><body>";
    response_html += "<h2>" + String(saved_count) +
                     " settings saved.</h2><p>Device is rebooting. You can "
                     "close this window.</p>";
    response_html += "<p>Saved: " + log + "</p></body></html>";
    server.send(200, "text/html", response_html);

    delay(3000);
    ESP.restart();
  });
  Serial.println("'/get' route configured.");

  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  Serial.println("'notFound' route configured.");

  server.begin();
  Serial.println("Web server started.");

  Serial.println("Setup mode active. Entering 10-minute wait loop...");
  unsigned long setup_start_time = millis();
  while (millis() - setup_start_time < 600000) {  // 10 minute timeout
    server.handleClient();
    delay(10);
  }

  Serial.println("Setup timeout reached. Rebooting.");
  ESP.restart();
}
