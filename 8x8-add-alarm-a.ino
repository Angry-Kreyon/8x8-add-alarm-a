#include <Adafruit_NeoPixel.h>
#include "font.h"  // Include the font header file
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>  // Include the time library

// Define LED matrix details
#define PIN_NEOPIXEL 14
#define NUMPIXELS    64  // 8x8 matrix
#define MATRIX_WIDTH 8
#define MATRIX_HEIGHT 8
#define RGB_BRIGHTNESS 20  // Brightness level (0-255)
uint8_t RGB_Data[3] = {30,30,30}; 
uint8_t Matrix_Data[8][8];  
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_RGB + NEO_KHZ800);

// Replace with your network credentials
const char* ssid = "WaxLand-A2-2G";
const char* password = "DodgeandHonda";

// Timezone details
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000; // Example: GMT-5 for EST
const int daylightOffset_sec = 3600;

//define alarm settings
String alarmTime = "";       // Stores the alarm time (HH:MM)
String alarmMessage = "";    // Stores the custom alarm message
bool alarmTriggered = false; // Flag to indicate if the alarm has been triggered
unsigned long alarmStartTime = 0; // Tracks when the alarm started flashing


// Create a web server object on port 80
WebServer server(80);

// Global variables
String defaultText = "";        // Default to the IP address
String scrollingText = "";      // Custom user input text
String currentMessage = "";     // Message currently displayed
String selectedMessage = "none"; // Predefined message selected
int scrollSpeed = 100;          // Scroll speed in milliseconds
uint8_t textColorR = 50, textColorG = 0, textColorB = 0;  // Text color
bool textUpdated = false;       // Flag for updated custom text

bool isFlashing = false;           // Flag to indicate if flashing is enabled
unsigned long flashStartMillis = 0; // Timestamp when flashing started
unsigned long flashInterval = 1000; // Flashing interval (1 second)
unsigned long flashDuration = 10000; // Flashing duration (10 seconds, adjustable)
unsigned long previousFlashMillis = 0; // Timestamp of the last flash
bool flashOn = false;              // Toggle for flash state

// Display modes
enum DisplayMode { TIME_ONLY, MESSAGE_ONLY, MESSAGE_AND_TIME };
DisplayMode displayMode = MESSAGE_ONLY;

// Non-blocking scrolling variables
unsigned long previousScrollMillis = 0;  // Keeps track of the last scroll update
unsigned long scrollInterval = 100;     // Adjust scroll speed in milliseconds
int currentShift = 0;                   // Tracks the current shift position for scrolling

void setup() {
    pixels.begin();  // Initialize the NeoPixel library
    pixels.setBrightness(RGB_BRIGHTNESS);  // Set brightness for all LEDs
    Serial.begin(115200);
    delay(100);  // Small delay to stabilize Serial
    Serial.println("ESP32 is starting...");
    
    // Connect to WiFi
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Set default text to the IP address and show it on startup
    defaultText = WiFi.localIP().toString();
    scrollingText = defaultText;  // Initially show the IP address
    currentMessage = defaultText; // Set the message for the matrix

    // Synchronize time with NTP server
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Initialize web server endpoints
    initializeWebServer();
    server.begin();
    Serial.println("Server started!");
}

void initializeWebServer() {
    // Home page
    server.on("/", []() {
    String html = "<!DOCTYPE html><html><head><title>ESP32 Controller</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
    html += "<style>body { font-family: Arial, sans-serif; margin: auto; text-align: center; }";
    html += "input[type=text], input[type=password], input[type=number], select { width: 90%; padding: 10px; margin: 10px 0; font-size: 1em; }";
    html += "h1 { font-size: 2em; }</style></head>";
    html += "<body><h1>ESP32 Scrolling Text Controller</h1>";

    // WiFi settings section
    html += "<form action=\"/update_wifi\" method=\"POST\">";
    html += "<h3>Update WiFi</h3>";
    html += "<p><a href=\"/scan_wifi\">Scan for WiFi Networks</a></p>";
    html += "<form action=\"/update_wifi\" method=\"POST\">";
    html += "<label for=\"ssid\">WiFi SSID:</label><br>";
    html += "<input type=\"text\" id=\"ssid\" name=\"ssid\"><br><br>";
    html += "<label for=\"password\">WiFi Password:</label><br>";
    html += "<input type=\"password\" id=\"password\" name=\"password\"><br><br>";
    html += "<input type=\"submit\" value=\"Update WiFi\">";
    html += "</form><br><br>";

    // Scrolling settings section
    html += "<form action=\"/update\" method=\"POST\">";
    html += "<h3>Scrolling Text Settings</h3>";
    html += "<label for=\"text\">Custom Scrolling Text:</label><br>";
    html += "<input type=\"text\" id=\"text\" name=\"text\" value=\"" + scrollingText + "\"><br><br>";
    html += "<label for=\"speed\">Scroll Speed (ms):</label><br>";
    html += "<input type=\"number\" id=\"speed\" name=\"speed\" value=\"" + String(scrollSpeed) + "\"><br><br>";
    html += "<label for=\"r\">Text Color (R):</label><br>";
    html += "<input type=\"number\" id=\"r\" name=\"r\" value=\"" + String(textColorR) + "\"><br>";
    html += "<label for=\"g\">Text Color (G):</label><br>";
    html += "<input type=\"number\" id=\"g\" name=\"g\" value=\"" + String(textColorG) + "\"><br>";
    html += "<label for=\"b\">Text Color (B):</label><br>";
    html += "<input type=\"number\" id=\"b\" name=\"b\" value=\"" + String(textColorB) + "\"><br><br>";

    // Display mode drop-down
    html += "<label for=\"mode\">Display Mode:</label><br>";
    html += "<select id=\"mode\" name=\"mode\">";
    html += "<option value=\"timeOnly\"" + String(displayMode == TIME_ONLY ? " selected" : "") + ">Time Only</option>";
    html += "<option value=\"messageOnly\"" + String(displayMode == MESSAGE_ONLY ? " selected" : "") + ">Message Only</option>";
    html += "<option value=\"messageAndTime\"" + String(displayMode == MESSAGE_AND_TIME ? " selected" : "") + ">Message + Time</option>";
    html += "</select><br><br>";

    // Predefined messages drop-down
    html += "<label for=\"predefined\">Predefined Message:</label><br>";
    html += "<select id=\"predefined\" name=\"predefined\">";
    html += "<option value=\"none\"" + String(selectedMessage == "none" ? " selected" : "") + ">None</option>";
    html += "<option value=\"welcome\"" + String(selectedMessage == "welcome" ? " selected" : "") + ">Welcome to ESP32</option>";
    html += "<option value=\"time\"" + String(selectedMessage == "time" ? " selected" : "") + ">The current time is</option>";
    html += "<option value=\"connect\"" + String(selectedMessage == "connect" ? " selected" : "") + ">Please connect to</option>";
    html += "<option value=\"created\"" + String(selectedMessage == "created" ? " selected" : "") + ">Created by Angry Kreyon for ESP32</option>";
    html += "</select><br><br>";

    html += "<input type=\"submit\" value=\"Update Settings\">";
    html += "</form><br>";

    // Add the "Flashing" checkbox and duration input
    html += "<form action=\"/update\" method=\"POST\">";
    html += "<label for=\"flashing\">Enable Flashing:</label><br>";
    html += "<input type=\"checkbox\" id=\"flashing\" name=\"flashing\"" + String(isFlashing ? " checked" : "") + "><br><br>";
    html += "<label for=\"flashDuration\">Flashing Duration (seconds): multiply by 2 (IE:20 = 10 sec. enter 0 for indefinate flash)</label><br>";
    html += "<input type=\"number\" id=\"flashDuration\" name=\"flashDuration\" value=\"" + String(flashDuration / 1000) + "\"><br><br>";
    html += "<input type=\"submit\" value=\"Update Settings\">";
    html += "</form><br>";



    // Reset button
    html += "<form action=\"/reset\" method=\"POST\">";
    html += "<input type=\"submit\" value=\"Reset to IP Address\">";
        html += "</form><br>";
            html += "</form><br>";
                html += "<label>Created by Jon (the Angry Kreyon) Ericson:</label><br>";
                html += "</form><br>";
                    html += "</form><br>";
    html += "</form></body></html>";

    server.send(200, "text/html", html);
});

    // Wi-Fi scan route
    server.on("/scan_wifi", []() {
        int n = WiFi.scanNetworks();
        String html = "<!DOCTYPE html><html><head><title>WiFi Scan</title>";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
        html += "<style>body { font-family: Arial, sans-serif; text-align: center; }</style>";
        html += "<body><h1>Available WiFi Networks</h1>";

        if (n == 0) {
            html += "<p>No networks found. Please try again.</p>";
        } else {
            html += "<ul>";
            for (int i = 0; i < n; ++i) {
                html += "<li>" + WiFi.SSID(i) + " (Signal: " + String(WiFi.RSSI(i)) + " dBm)";
                html += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " (Open)" : " (Secured)";
                html += "</li>";
            }
            html += "</ul>";
        }

        html += "<form action=\"/connect_wifi\" method=\"POST\">";
        html += "<label for=\"ssid\">SSID:</label><br>";
        html += "<input type=\"text\" name=\"ssid\"><br>";
        html += "<label for=\"password\">Password:</label><br>";
        html += "<input type=\"password\" name=\"password\"><br>";
        html += "<button type=\"submit\">Connect</button></form>";
        html += "<p><a href=\"/\">Back to Home</a></p></body></html>";
        server.send(200, "text/html", html);
    });

    // Wi-Fi connection route
    server.on("/connect_wifi", HTTP_POST, []() {
        if (server.hasArg("ssid") && server.hasArg("password")) {
            String newSSID = server.arg("ssid");
            String newPassword = server.arg("password");

            WiFi.disconnect();
            WiFi.begin(newSSID.c_str(), newPassword.c_str());
            Serial.println("Connecting to WiFi...");

            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                Serial.print(".");
                attempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nSuccessfully connected to WiFi!");
                defaultText = WiFi.localIP().toString();
                scrollingText = defaultText;

                server.send(200, "text/html", "<h1>Connected Successfully!</h1><p>New IP: " + WiFi.localIP().toString() + "</p><p><a href=\"/\">Back to Home</a></p>");
            } else {
                Serial.println("\nFailed to connect to WiFi.");
                server.send(200, "text/html", "<h1>Connection Failed</h1><p>Check credentials and try again.</p><p><a href=\"/\">Back to Home</a></p>");
            }
        } else {
            server.send(200, "text/html", "<h1>Error: Missing SSID or Password</h1><p><a href=\"/\">Back to Home</a></p>");
        }
    });

    // Updating scrolling text, speed, and colors
server.on("/update", HTTP_POST, []() {
    // Handle flashing state
    if (server.hasArg("flashing")) {
        isFlashing = true;  // Enable flashing
        flashStartMillis = millis();  // Record the start time
        Serial.println("Flashing enabled.");
    } else {
        isFlashing = false;  // Disable flashing
        Serial.println("Flashing disabled.");
    }

    // Handle flashing duration
    if (server.hasArg("flashDuration")) {
        String durationInput = server.arg("flashDuration");
        if (durationInput.length() > 0) {
            flashDuration = durationInput.toInt() * 1000;  // Convert seconds to milliseconds
        } else {
            flashDuration = 0;  // Indicate "indefinite flashing"
        }
        Serial.println("Flashing duration set to: " + String(flashDuration == 0 ? "Indefinite" : String(flashDuration) + " ms"));
    } else {
        flashDuration = 0;  // Default to indefinite if no duration provided
        Serial.println("No duration provided. Defaulting to indefinite flashing.");
    }

    // Handle other updates (text, speed, colors, etc.)
    if (server.hasArg("text")) {
        scrollingText = server.arg("text");
        textUpdated = true;
    }
    if (server.hasArg("speed")) scrollSpeed = server.arg("speed").toInt();
    if (server.hasArg("r")) textColorR = server.arg("r").toInt();
    if (server.hasArg("g")) textColorG = server.arg("g").toInt();
    if (server.hasArg("b")) textColorB = server.arg("b").toInt();
    if (server.hasArg("mode")) {
        String mode = server.arg("mode");
        if (mode == "timeOnly") displayMode = TIME_ONLY;
        else if (mode == "messageOnly") displayMode = MESSAGE_ONLY;
        else if (mode == "messageAndTime") displayMode = MESSAGE_AND_TIME;
    }
    if (server.hasArg("predefined")) {
        selectedMessage = server.arg("predefined");
    }

    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>ESP32 Controller</title>"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"<style>body { font-family: Arial, sans-serif; margin: auto; text-align: center; }"
"h1 { font-size: 2em; } p { font-size: 1.2em; margin: 20px; }</style></head>"
"<body><h1>Settings Updated!</h1><p>Go back to <a href=\"/\">Home</a>.</p></body></html>");
});




    // Resetting to default text
    server.on("/reset", HTTP_POST, []() {
    // Reset variables to their default states
    scrollingText = defaultText;  // Reset scrolling text to the IP address
    currentMessage = defaultText;  // Display the default IP address message
    textUpdated = false;           // Clear the custom text update flag
    selectedMessage = "none";      // Reset the predefined message selection
    displayMode = MESSAGE_ONLY;    // Default display mode to "Message Only"

    // Update the LED matrix to immediately reflect the reset
    pixels.clear();
    scrollTextStep(currentMessage.c_str(), pixels.Color(textColorR, textColorG, textColorB));
    pixels.show();

    // Send confirmation to the user
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>ESP32 Controller</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<style>body { font-family: Arial, sans-serif; margin: auto; text-align: center; }"
    "h1 { font-size: 2em; } p { font-size: 1.2em; margin: 20px; }</style></head>"
    "<body><h1>Reset Successful!</h1><p>All settings have been reset to default.</p>"
    "<p>Go back to <a href=\"/\">Home</a>.</p></body></html>");
    });

}

// Function to scroll text across the 8x8 LED matrix
void scrollTextStep(const char* text, uint32_t color) {
    int len = strlen(text);
    pixels.clear();  // Clear the matrix before drawing the next frame

    for (int i = 0; i < len; i++) {  // Loop through each character in the text
        char c = text[i];
        const uint8_t* bitmap;

        // Map characters to their corresponding font bitmaps
        if (c >= '0' && c <= '9') {
            bitmap = numbers[c - '0'];
        } else if (c >= 'a' && c <= 'z') {
            bitmap = lettersL[c - 'a'];
        } else if (c >= 'A' && c <= 'Z') {
            bitmap = letters[c - 'A'];
        } else if (c == '!') {
            bitmap = specialChars[0];  // Exclamation mark
        } else if (c == ' ') {
            bitmap = specialChars[6];  // Space
        } else if (c == '.') {
            bitmap = specialChars[7];  // Dot
        } else if (c == ':') {
            bitmap = specialChars[8];  // colon
        } else {
            continue;  // Skip unsupported characters
        }

        // Render each character's pixels on the matrix
        for (int y = 0; y < 8; y++) {  // Rows
            for (int x = 0; x < 8; x++) {  // Columns
                int columnPosition = i * 8 + x - currentShift;

                if (columnPosition >= 0 && columnPosition < MATRIX_WIDTH) {
                    if (bitmap[y] & (1 << (7 - x))) {  // Check if the bit is set
                        int pixelIndex = y * MATRIX_WIDTH + columnPosition;
                        pixels.setPixelColor(pixelIndex, color);  // Set the pixel color
                    }
                }
            }
        }
    }

    pixels.show();  // Display the updated matrix

    // Update the shift for the next frame
    currentShift++;
    if (currentShift >= len * 8 + MATRIX_WIDTH) {
        currentShift = 0;  // Reset the shift to loop the message
    }
}

// Function to get the current time and format it
String getCurrentTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return "Time Error";
    }
    char timeStr[11];  // Buffer for 12-hour format with AM/PM
    strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p", &timeinfo); // Format: HH:MM:SS AM/PM
    return String(timeStr);
}

// Function to retrieve predefined messages
String getPredefinedMessage(String key) {
    if (key == "welcome") {
        return "Welcome to ESP32";
    } else if (key == "time") {
        return "The current time is:";
    } else if (key == "connect") {
        return "Please connect to: " + defaultText;
    } else if (key == "created") {
        return "Created by Angry Kreyon for ESP32";
    } else {
        return "";  // Default to empty string if no match
    }
}

void handleFlashingLights() {
    static bool lightState = false;
    static unsigned long lastToggleTime = 0;

    if (millis() - lastToggleTime > 500) {  // Toggle every 500ms
        lightState = !lightState;
        for (int i = 0; i < 64; i++) {
            if (lightState) {
                pixels.setPixelColor(i, pixels.Color(255, 0, 0)); // Flash red
            } else {
                pixels.setPixelColor(i, pixels.Color(0, 0, 0));   // Turn off
            }
        }
        pixels.show();
        lastToggleTime = millis();
    }
}
void displayAlarmMessage() {
    // Clear the matrix
    memset(Matrix_Data, 0, sizeof(Matrix_Data));

    // Display the alarm message (e.g., scrolling or static)
    for (int i = 0; i < alarmMessage.length(); i++) {
        char c = alarmMessage.charAt(i);
        //displayCharacter(c, i);  // Function to display characters on the matrix
    }

    pixels.show();
}


void loop() {
    // Handle web server requests
    server.handleClient();
    // Check if alarm is set and current time matches
    String currentTime = getCurrentTime();  // Function to get current time as HH:MM
    if (currentTime == alarmTime && !alarmTriggered) {
        alarmTriggered = true;              // Activate alarm
        alarmStartTime = millis();          // Start flashing timer
    }

    // Handle flashing lights for alarm
    if (alarmTriggered) {
        handleFlashingLights();             // Function to control flashing
        displayAlarmMessage();              // Show the custom message
    }

    // Stop flashing after 2 minutes
    if (alarmTriggered && millis() - alarmStartTime > 120000) {
        alarmTriggered = false;             // Reset alarm
        resetScrollingText();               // Restore scrolling text
    }

    // Determine message to display based on mode
    if (displayMode == MESSAGE_ONLY) {
      if (selectedMessage == "none" && textUpdated) {
        currentMessage = scrollingText;  // Use custom text if no predefined message is selected
      } else if (selectedMessage != "none") {
        currentMessage = getPredefinedMessage(selectedMessage);  // Use predefined message
      } else {
        currentMessage = defaultText;  // Default to IP address
      }
    } else if (displayMode == TIME_ONLY) {
        currentMessage = getCurrentTime();  // Display only the time
    } else if (displayMode == MESSAGE_AND_TIME) {
        if (selectedMessage != "none") {
            currentMessage = getPredefinedMessage(selectedMessage) + " " + getCurrentTime();  // Predefined message + time
        } else if (textUpdated) {
            currentMessage = scrollingText + " " + getCurrentTime();  // Custom message + time
        } else {
            currentMessage = defaultText + " " + getCurrentTime();  // Default to IP + time
        }
    }
// Handle flashing logic
if (isFlashing) {
    unsigned long currentMillis = millis();

    // Stop flashing only if a specific duration is set (non-zero) and the duration is exceeded
    if (flashDuration > 0 && currentMillis - flashStartMillis >= flashDuration) {
        isFlashing = false;  // Disable flashing automatically
        Serial.println("Flashing stopped automatically after timeout.");
    }

    // Handle the flash state
    if (currentMillis - previousFlashMillis >= flashInterval) {
        previousFlashMillis = currentMillis;
        flashOn = !flashOn;  // Toggle flash state

        if (flashOn) {
            // Turn on the 4 corner LEDs (indices 0, 7, 56, 63 for 8x8 matrix)
            pixels.setPixelColor(0, pixels.Color(255, 255, 255));  // Top-left
            pixels.setPixelColor(7, pixels.Color(255, 255, 255));  // Top-right
            pixels.setPixelColor(56, pixels.Color(255, 255, 255)); // Bottom-left
            pixels.setPixelColor(63, pixels.Color(255, 255, 255)); // Bottom-right
        } else {
            // Turn off the 4 corner LEDs
            pixels.setPixelColor(0, pixels.Color(0, 0, 0));  // Top-left
            pixels.setPixelColor(7, pixels.Color(0, 0, 0));  // Top-right
            pixels.setPixelColor(56, pixels.Color(0, 0, 0)); // Bottom-left
            pixels.setPixelColor(63, pixels.Color(0, 0, 0)); // Bottom-right
        }
        pixels.show();  // Update the LED matrix
    }
}

    // Non-blocking scrolling
    unsigned long currentMillis = millis();
    if (currentMillis - previousScrollMillis >= scrollInterval) {
        previousScrollMillis = currentMillis;
        scrollTextStep(currentMessage.c_str(), pixels.Color(textColorR, textColorG, textColorB));
    }
}
// Utility function to reset scrolling text
void resetScrollingText() {
    scrollingText = defaultText;  // Reset to the default IP address
    textUpdated = false;          // Clear the text update flag
    currentMessage = defaultText; // Restore the default message displayed on the LED matrix

    pixels.clear();
    scrollTextStep(currentMessage.c_str(), pixels.Color(textColorR, textColorG, textColorB));
    pixels.show();
    Serial.println("Scrolling text reset to default.");
}
