/*
 * IoT Gateway Honeypot Using ESP32
 * Educational prototype for an authorized local lab network only.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

// ========================================
// 1. Wi-Fi and Python server configuration
// ========================================

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* LOG_SERVER_URL =
    "http://192.168.43.199:5000/api/log";

const char* DEVICE_ID = "GW-001";

// ========================================
// 2. HTTP server
// ========================================

WebServer server(80);

// ========================================
// 3. Escape strings for JSON
// ========================================

String jsonEscape(const String& input) {
  String output;

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];

    if (c == '\\' || c == '"') {
      output += '\\';
    }

    if (c == '\n' || c == '\r') {
      continue;
    }

    output += c;
  }

  return output;
}

// ========================================
// 4. Forward events to the Python collector
// ========================================

void sendLog(
    const String& method,
    const String& path,
    const String& clientIp,
    int statusCode) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Log not forwarded: Wi-Fi disconnected.");
    return;
  }

  HTTPClient http;

  http.setTimeout(3000);

  if (!http.begin(LOG_SERVER_URL)) {
    Serial.println("Could not initialize HTTP connection.");
    return;
  }

  http.addHeader("Content-Type", "application/json");

  String body =
      "{\"device_id\":\"" + String(DEVICE_ID) +
      "\",\"source_ip\":\"" + jsonEscape(clientIp) +
      "\",\"method\":\"" + jsonEscape(method) +
      "\",\"path\":\"" + jsonEscape(path) +
      "\",\"status_code\":" + String(statusCode) + "}";

  int result = http.POST(body);

  Serial.printf(
      "Event: %s %s | Collector response: %d\n",
      method.c_str(),
      path.c_str(),
      result
  );

  if (result > 0) {
    String response = http.getString();

    if (response.length() > 0) {
      Serial.println("Collector response:");
      Serial.println(response);
    }
  } else {
    Serial.printf("Collector connection failed: %s\n",
                  http.errorToString(result).c_str());
  }

  http.end();
}

// ========================================
// 5. Fake IoT gateway home page
// ========================================

String gatewayPage() {
  return
      "<!doctype html>"
      "<html>"
      "<head>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>IoT Gateway</title>"
      "</head>"
      "<body style='font-family:Arial;margin:2rem'>"
      "<h1>IoT Gateway</h1>"
      "<p>Gateway ID: GW-001</p>"
      "<p>Status: Online</p>"
      "<p>Firmware: v2.1.0</p>"
      "<h3>Gateway Services</h3>"
      "<ul>"
      "<li><a href='/admin'>Device Management</a></li>"
      "<li><a href='/config'>Configuration</a></li>"
      "<li><a href='/debug'>System Information</a></li>"
      "<li><a href='/login'>Login</a></li>"
      "</ul>"
      "</body>"
      "</html>";
}

// ========================================
// 6. Home page handler
// ========================================

void handleRoot() {
  String clientIp = server.client().remoteIP().toString();

  sendLog("GET", "/", clientIp, 200);

  server.send(200, "text/html", gatewayPage());
}

// ========================================
// 7. Observed gateway service handler
// ========================================

void handleObservedPath() {
  String path = server.uri();
  String clientIp = server.client().remoteIP().toString();

  sendLog("GET", path, clientIp, 200);

  server.send(
      200,
      "text/html",
      "<!doctype html><html><body style='font-family:Arial'>"
      "<h2>Gateway Service</h2>"
      "<p>Request observed by the educational honeypot.</p>"
      "<a href='/'>Home</a>"
      "</body></html>"
  );
}

// ========================================
// 8. Unknown path handler
// ========================================

void handleNotFound() {
  String path = server.uri();
  String clientIp = server.client().remoteIP().toString();
  String method = "GET";

  sendLog(method, path, clientIp, 404);

  server.send(404, "text/plain", "Not found");
}

// ========================================
// 9. Setup
// ========================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println(" IoT Gateway Honeypot");
  Serial.println("================================");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 20000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected successfully.");

    Serial.print("Honeypot IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("Collector URL: ");
    Serial.println(LOG_SERVER_URL);
  } else {
    Serial.println("Wi-Fi connection failed.");
    Serial.println("Check Wi-Fi SSID and password.");
  }

  server.on("/", HTTP_GET, handleRoot);

  server.on("/admin", HTTP_GET, handleObservedPath);
  server.on("/config", HTTP_GET, handleObservedPath);
  server.on("/debug", HTTP_GET, handleObservedPath);
  server.on("/login", HTTP_GET, handleObservedPath);

  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("HTTP server started on port 80.");
  Serial.println("Setup complete.");
}

// ========================================
// 10. Main loop
// ========================================

void loop() {
  server.handleClient();
}
