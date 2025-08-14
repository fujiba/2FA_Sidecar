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

  const char *key_names[] = {"Key 1", "Key 2", "Key 3", "Key 4", "Key 5"};
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
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);

  tft.setCursor(3, 35);
  tft.fillScreen(ST77XX_RED);
  tft.printf("Connect via Wifi\nSSID:%s\nthen browse to \nhttp://192.168.4.1",
             ssid.c_str());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang='ja'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>2FA-Sidecar Config</title>
    <script src='https://cdn.tailwindcss.com'></script>
    <style>
        body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif}
        input[type="text"]{background-color:#4a5568;border-color:#718096;color:#f7fafc}
        input[type="submit"]{background-color:#4299e1;color:white;transition:background-color .2s}
        input[type="submit"]:hover{background-color:#2b6cb0}
    </style>
</head>
<body class='bg-gray-800 text-gray-200 p-4 sm:p-6 md:p-8'>
    <div class='max-w-2xl mx-auto bg-gray-900 rounded-lg shadow-lg p-6'>
        <header class='mb-6'>
            <h2 class='text-2xl font-bold text-blue-400'>2FA-Sidecar Configuration</h2>
            <p class='text-sm text-gray-400 mt-1'>(C) 2024 Matt Perkins & T.Fujiba - GPL</p>
        </header>
        <p class='text-gray-400 mb-6 border-l-4 border-yellow-500 pl-4'>
            Current settings are not displayed for security. Fill in only the fields you wish to set or update.
        </p>
        <form action='/get'>
            <div class='mb-8 p-4 bg-gray-800 rounded-lg'>
                <h3 class='text-xl font-semibold text-blue-300 mb-4'>General Settings</h3>
                <div class='flex flex-wrap items-center mb-2'>
                    <label for='ssid' class='w-full sm:w-1/3 mb-2 sm:mb-0'>SSID:</label>
                    <input type='text' id='ssid' name='ssid' class='flex-grow rounded-md p-2 border'>
                </div>
                <div class='flex flex-wrap items-center mb-2'>
                    <label for='password' class='w-full sm:w-1/3 mb-2 sm:mb-0'>WiFi Password:</label>
                    <input type='text' id='password' name='password' class='flex-grow rounded-md p-2 border'>
                </div>
                <div class='flex flex-wrap items-center'>
                    <label for='pin' class='w-full sm:w-1/3 mb-2 sm:mb-0'>Access PIN (4 digits):</label>
                    <input type='text' id='pin' name='pin' class='flex-grow rounded-md p-2 border'>
                </div>
            </div>
            <div>
    )rawliteral";

    for (int b = 0; b < NUM_BANKS; b++) {
      html +=
          "<div class='mb-6 p-4 bg-gray-800 rounded-lg'><h3 class='text-xl "
          "font-semibold text-blue-300 mb-4'>Bank " +
          String(b + 1) + "</h3>";
      for (int k = 0; k < NUM_KEYS; k++) {
        String name_key = "tfa_name_" + String(b) + "_" + String(k);
        String seed_key = "tfa_seed_" + String(b) + "_" + String(k);
        html +=
            "<div class='flex flex-wrap items-center mb-2'><label "
            "class='w-full sm:w-1/3 mb-2 sm:mb-0'>2FA Key " +
            String(k + 1) + " Name:</label><input type='text' name='" +
            name_key + "' class='flex-grow rounded-md p-2 border'></div>";
        html +=
            "<div class='flex flex-wrap items-center mb-4'><label "
            "class='w-full sm:w-1/3 mb-2 sm:mb-0'>2FA Key " +
            String(k + 1) + " Seed:</label><input type='text' name='" +
            seed_key + "' class='flex-grow rounded-md p-2 border'></div>";
      }
      html += "</div>";
    }

    html += R"rawliteral(
            </div>
            <div class='mt-8 text-center'>
                <input type='submit' value='Save All Settings' class='w-full sm:w-auto px-6 py-3 rounded-lg font-bold text-lg cursor-pointer'>
            </div>
        </form>
    </div>
</body>
</html>
    )rawliteral";
    request->send(200, "text/html", html);
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    preferences.begin("2FA_Sidecar", false);
    int params = request->params();
    String log = "";
    int saved_count = 0;

    for (int i = 0; i < params; i++) {
      AsyncWebParameter *p = request->getParam(i);
      // Only save if the value is not empty. This prevents overwriting existing
      // settings with blank values.
      if (p->value().length() > 0) {
        preferences.putString(p->name().c_str(), p->value());
        log += p->name() + ", ";
        saved_count++;
      }
    }

    if (log.length() > 2) {
      log = log.substring(0,
                          log.length() - 2);  // Remove trailing comma and space
    }

    tft.fillScreen(ST77XX_BLUE);
    tft.setCursor(5, 30);
    tft.printf("%d parameters saved.\nRebooting...", saved_count);

    request->send(200, "text/html",
                  "<!DOCTYPE html><html><head><title>Saved</title><meta "
                  "http-equiv='refresh' content='3;url=/'></head><body>"
                  "<h2>" +
                      String(saved_count) +
                      " settings saved.</h2><p>Device is rebooting. You can "
                      "close this window.</p>"
                      "<p>Saved: " +
                      log + "</p></body></html>");
    delay(3000);
    ESP.restart();
  });

  server.onNotFound(notFound);
  server.begin();

  delay(600000);
  ESP.restart();
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
