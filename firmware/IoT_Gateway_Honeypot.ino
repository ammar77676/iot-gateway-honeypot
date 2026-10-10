/*
 * IoT Gateway Honeypot Using ESP32
 * Educational prototype for an authorized local lab network only.
 * Configure Wi-Fi and the laptop log collector address before uploading.
 */
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* LOG_SERVER_URL = "http://192.168.1.10:5000/api/log"; // Replace with laptop IPv4
const char* DEVICE_ID = "GW-001";

WebServer server(80);

String jsonEscape(const String& in) {
  String out;
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '\\' || c == '"') out += '\\';
    if (c == '\n' || c == '\r') continue;
    out += c;
  }
  return out;
}

void sendLog(const String& method, const String& path, const String& clientIp, int statusCode) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Log not forwarded: Wi-Fi disconnected.");
    return;
  }
  HTTPClient http;
  http.begin(LOG_SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  String body = "{\"device_id\":\"" + String(DEVICE_ID) +
                "\",\"source_ip\":\"" + jsonEscape(clientIp) +
                "\",\"method\":\"" + jsonEscape(method) +
                "\",\"path\":\"" + jsonEscape(path) +
                "\",\"status_code\":" + String(statusCode) + "}";
  int result = http.POST(body);
  Serial.printf("Forwarded event path=%s, collector response=%d\n", path.c_str(), result);
  http.end();
}

String gatewayPage() {
  return "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
         "<title>IoT Gateway</title></head><body style='font-family:Arial;margin:2rem'>"
         "<h1>IoT Gateway</h1><p>Gateway ID: GW-001</p><p>Status: Online</p>"
         "<p>Firmware: v2.1.0</p><ul><li><a href='/admin'>Device Management</a></li>"
         "<li><a href='/config'>Configuration</a></li><li><a href='/debug'>System Information</a></li>"
         "<li><a href='/login'>Login</a></li></ul></body></html>";
}

void handleRoot() {
  String clientIp = server.client().remoteIP().toString();
  sendLog("GET", "/", clientIp, 200);
  server.send(200, "text/html", gatewayPage());
}

void handleObservedPath() {
  String path = server.uri();
  String clientIp = server.client().remoteIP().toString();
  sendLog("GET", path, clientIp, 200);
  server.send(200, "text/html",
    "<html><body style='font-family:Arial'><h2>Gateway service</h2>"
    "<p>Request observed by the educational honeypot.</p><a href='/'>Home</a></body></html>");
}

void handleNotFound() {
  String path = server.uri();
  String clientIp = server.client().remoteIP().toString();
  sendLog("GET", path, clientIp, 404);
  server.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500); Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Honeypot IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi not connected. Check credentials.");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/admin", HTTP_GET, handleObservedPath);
  server.on("/config", HTTP_GET, handleObservedPath);
  server.on("/debug", HTTP_GET, handleObservedPath);
  server.on("/login", HTTP_GET, handleObservedPath);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("IoT Gateway Honeypot HTTP server started on port 80.");
}

void loop() {
  server.handleClient();
}
