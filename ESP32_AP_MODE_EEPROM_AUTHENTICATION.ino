#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

// Wi-Fi access point credentials
const char* ssid = "ESP32_Hotspot";
const char* password = "12345678";

// Basic authentication credentials
const char* authUsername = "admin";
const char* authPassword = "password";

// Static IP configuration
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// GPIO pins for relays
const int relayPins[] = {23, 22, 21, 19};

// GPIO pins for push buttons
const int buttonPins[] = {34, 35, 32, 33};

// Relay states
bool relayStates[4] = {LOW, LOW, LOW, LOW};

// Create a WebServer object on port 80
WebServer server(80);

// Toggle relay function
void toggleRelay(int relayIndex) {
  relayStates[relayIndex] = !relayStates[relayIndex];
  digitalWrite(relayPins[relayIndex], relayStates[relayIndex]);

  // Save relay state to EEPROM
  EEPROM.write(relayIndex, relayStates[relayIndex]);
  EEPROM.commit();
}

// HTML content generation function with feedback and styling
String generateHTML() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; max-width: 400px; margin: auto; text-align: center; }";
  html += "h1 { color: #333; }";
  html += ".button { display: inline-block; width: 80%; padding: 15px; margin: 10px; font-size: 18px; color: white; background-color: #4CAF50; border: none; border-radius: 5px; text-decoration: none; }";
  html += ".button.off { background-color: #f44336; }";
  html += ".refresh { background-color: #2196F3; padding: 10px 20px; margin: 15px; color: white; font-size: 18px; border: none; border-radius: 5px; }";
  html += ".footer { margin-top: 20px; color: #666; font-size: 14px; }";
  html += "</style></head><body>";
  html += "<h1>ESP32 Relay Control</h1>";

  // Relay control buttons
  for (int i = 0; i < 4; i++) {
    html += "<p>Relay " + String(i + 1) + ": " + (relayStates[i] == LOW ? "ON" : "OFF") + "</p>";
    html += "<form action=\"/toggle_relay_" + String(i) + "\" method=\"GET\">";
    html += "<button class=\"button " + String(relayStates[i] == LOW ? "off" : "") + "\" type=\"submit\">" 
            + String(relayStates[i] == LOW ? "Turn OFF" : "Turn ON") + " Relay " + String(i + 1) + "</button>";
    html += "</form><br>";
  }

  // Refresh button
  html += "<form action=\"/refresh\" method=\"GET\">";
  html += "<button class=\"refresh\" type=\"submit\">Refresh</button>";
  html += "</form>";

  // Footer text
  html += "<div class=\"footer\">Technolab Creation</div>";

  html += "</body></html>";
  return html;
}

// Basic authentication check
bool authenticate() {
  if (!server.authenticate(authUsername, authPassword)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

// Setup server routes
void setupServerRoutes() {
  // Route for the main page
  server.on("/", HTTP_GET, []() {
    if (!authenticate()) return;
    server.send(200, "text/html", generateHTML());
  });

  // Route for relay toggle control
  for (int i = 0; i < 4; i++) {
    int relayIndex = i;
    server.on(("/toggle_relay_" + String(i)).c_str(), HTTP_GET, [relayIndex]() {
      if (!authenticate()) return;
      toggleRelay(relayIndex);
      server.send(200, "text/html", generateHTML());
    });
  }

  // Route for refreshing the page without toggling relays
  server.on("/refresh", HTTP_GET, []() {
    if (!authenticate()) return;
    server.send(200, "text/html", generateHTML());
  });
}

void setup() {
  Serial.begin(115200);

  // Initialize EEPROM with size 4 bytes (1 byte per relay state)
  EEPROM.begin(4);

  // Load relay states from EEPROM
  for (int i = 0; i < 4; i++) {
    relayStates[i] = EEPROM.read(i);
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], relayStates[i]); // Set relay to saved state
  }

  // Initialize button pins as input with pull-up resistors
  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Configure Wi-Fi with a fixed IP and set up as an access point
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  // Set up web server routes
  setupServerRoutes();

  // Start the server
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  // Handle incoming client requests
  server.handleClient();

  // Manual button control
  for (int i = 0; i < 4; i++) {
    if (digitalRead(buttonPins[i]) == HIGH) { // Button pressed (active low)
      delay(100); // Debounce delay
      toggleRelay(i); // Toggle the corresponding relay
      while (digitalRead(buttonPins[i]) == HIGH); // Wait until button is released
      delay(100); // Debounce delay
    }
  }
}
