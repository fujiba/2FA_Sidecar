// 2FA Sidecar
// Matt Perkins & fujiba - Copyright (C) 2024-2025
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
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>2FA-Sidecar Config</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background-color: #1a202c; color: #e2e8f0; margin: 0; padding: 1rem; }
        .container { max-width: 42rem; margin-left: auto; margin-right: auto; background-color: #2d3748; border-radius: 0.5rem; box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06); padding: 1.5rem; }
        header { margin-bottom: 1.5rem; }
        h2 { font-size: 1.5rem; font-weight: bold; color: #63b3ed; }
        h3 { font-size: 1.25rem; font-weight: 600; color: #90cdf4; margin-bottom: 1rem; }
        p.note { color: #a0aec0; margin-bottom: 1.5rem; border-left: 4px solid #f6e05e; padding-left: 1rem; }
        .form-group { display: flex; flex-wrap: wrap; align-items: center; margin-bottom: 0.5rem; }
        .form-group label { width: 100%; margin-bottom: 0.25rem; }
        .form-group input[type="text"] { flex-grow: 1; border-radius: 0.375rem; padding: 0.5rem; border: 1px solid #718096; background-color: #4a5568; color: #f7fafc; width: 100%; }
        .settings-block { margin-bottom: 1.5rem; padding: 1rem; background-color: #1a202c; border-radius: 0.5rem; }
        .submit-container { margin-top: 2rem; text-align: center; }
        input[type="submit"] { width: 100%; padding: 0.75rem 1.5rem; border-radius: 0.5rem; font-weight: bold; font-size: 1.125rem; cursor: pointer; border: none; background-color: #4299e1; color: white; transition: background-color 0.2s; }
        input[type="submit"]:hover { background-color: #2b6cb0; }
        @media (min-width: 640px) {
            .form-group label { width: 33.333333%; margin-bottom: 0; }
            .form-group input[type="text"] { width: auto; }
            input[type="submit"] { width: auto; }
        }
    </style>
</head>
<body>
    <div class='container'>
        <header>
            <h2>2FA-Sidecar Configuration</h2>
            <p style="font-size: 0.875rem; color: #a0aec0; margin-top: 0.25rem;">(C) 2024 Matt Perkins & fujiba - GPL</p>
        </header>
        <p class='note'>Current settings are not displayed for security. Fill in only the fields you wish to set or update.</p>
        <form action='/get'>
            <div class='settings-block'>
                <h3>General Settings</h3>
                <div class='form-group'>
                    <label for='ssid'>SSID:</label>
                    <input type='text' id='ssid' name='ssid'>
                </div>
                <div class='form-group'>
                    <label for='password'>WiFi Password:</label>
                    <input type='text' id='password' name='password'>
                </div>
                <div class='form-group'>
                    <label for='pin'>Access PIN (4 digits):</label>
                    <input type='text' id='pin' name='pin'>
                </div>
            </div>
            <div>
)rawliteral";

const char HTML_FOOTER[] PROGMEM = R"rawliteral(
            </div>
            <div class='submit-container'>
                <input type='submit' value='Save All Settings'>
            </div>
        </form>
    </div>
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
      String bank_html =
          "<div class='settings-block'><h3>Bank " + String(b + 1) + "</h3>";
      html += bank_html;

      for (int k = 0; k < NUM_KEYS; k++) {
        char name_key[20];
        char seed_key[20];
        sprintf(name_key, "tfa_name_%d_%d", b, k);
        sprintf(seed_key, "tfa_seed_%d_%d", b, k);
        String key_html =
            "<div class='form-group'><label>Key " + String(k + 1) +
            " Name:</label><input type='text' name='" + name_key + "'></div>";
        key_html +=
            "<div class='form-group' style='margin-bottom: 1rem;'><label>Key " +
            String(k + 1) + " Seed:</label><input type='text' name='" +
            seed_key + "'></div>";
        html += key_html;
      }
      html += "</div>";
    }

    html += HTML_FOOTER;
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
