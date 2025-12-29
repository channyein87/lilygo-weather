#include <epd_driver.h>
#include "lexend18.h"
#include "lexend40.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

/*
 * LILYGO T5 4.7" E-Paper Display - Weather Display
 * 
 * Connects to WiFi and fetches weather data from OpenWeatherMap API
 * Displays current weather, temperature, and forecast on e-paper display
 * 
 * Configuration is read from config.json (stored in LittleFS)
 */

// ===== CONFIGURATION =====
// WiFi credentials (loaded from config.json)
String ssid = "";
String password = "";

// OpenWeatherMap API (loaded from config.json)
String api_key = "";
String city = "";
String country_code = "";
String units = "";

// Update interval (in milliseconds) - default 5 minutes
unsigned long UPDATE_INTERVAL = 300000;

// Display dimensions
#define EPD_DISPLAY_WIDTH 960
#define EPD_DISPLAY_HEIGHT 540

// Framebuffer for grayscale image (4 bits per pixel)
uint8_t *framebuffer = NULL;

// ===== GLOBALS =====
unsigned long last_update = 0;
String current_temp = "--";
String current_condition = "Loading...";
String current_humidity = "--";
String wind_speed = "--";
String feels_like = "--";

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n=== LILYGO T5 Weather Display ===");
    Serial.println("Starting up...");

    // Load configuration from JSON file
    if (!loadConfig()) {
        Serial.println("ERROR: Failed to load config.json!");
        Serial.println("Please create config.json based on config.template.json");
        while (1) delay(1000);
    }

    // Allocate framebuffer (4 bits per pixel = EPD_WIDTH * EPD_HEIGHT / 2)
    framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_DISPLAY_WIDTH * EPD_DISPLAY_HEIGHT / 2);
    if (!framebuffer) {
        Serial.println("ERROR: Failed to allocate framebuffer!");
        while (1) delay(1000);
    }
    
    // Initialize display
    epd_init();
    
    // Clear display (set all pixels to white = 0xFF)
    memset(framebuffer, 0xFF, EPD_DISPLAY_WIDTH * EPD_DISPLAY_HEIGHT / 2);
    epd_clear();
    
    Serial.println("Display initialized");
    Serial.println("\nDrawing content...");
    
    // Power on display
    epd_poweron();
    
    // Create drawing context
    int32_t cursor_x = 0;
    int32_t cursor_y = 0;
    
    // Show initialization message
    cursor_x = 300;
    cursor_y = 100;
    writeln((GFXfont *)&Lexend18, "Weather Display", &cursor_x, &cursor_y, NULL);
    
    // Connect to WiFi
    cursor_x = 200;
    cursor_y = 200;
    writeln((GFXfont *)&Lexend18, "Connecting to WiFi...", &cursor_x, &cursor_y, NULL);
    connectToWiFi();
    
    // Stop if WiFi connection failed
    if (WiFi.status() != WL_CONNECTED) {
        int cursor_x = 10;
        int cursor_y = 50;
        writeln((GFXfont *)&Lexend18, "WiFi Connection Failed", &cursor_x, &cursor_y, NULL);
        epd_poweroff_all();
        while (1) {
            delay(1000);  // Halt execution
        }
    }

    // Clear the screen
    epd_clear();

    // Fetch weather data
    fetchWeatherData();
    
    // Display weather
    displayWeather();
    
    // Power down display to save power
    epd_poweroff_all();
    
    Serial.println("Setup complete. Weather displayed.");
}

void loop() {
    // Check if it's time to update
    if (millis() - last_update > UPDATE_INTERVAL) {
        Serial.println("\nTime to refresh weather data...");
        
        // Power up display
        epd_poweron();
        epd_clear();
        
        // Fetch and display
        fetchWeatherData();
        displayWeather();
        
        // Power down
        epd_poweroff_all();
        
        last_update = millis();
    }
    
    // Sleep to save power
    delay(10000);  // Check every 10 seconds if update needed
}

void connectToWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect to WiFi");
        current_condition = "WiFi Error";
    }
}

void fetchWeatherData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping update");
        current_condition = "No WiFi";
        return;
    }
    
    Serial.println("Fetching weather data...");
    
    // Build API URL
    String url = String("https://api.openweathermap.org/data/2.5/weather?q=") + 
                 city + "," + country_code + 
                 "&appid=" + api_key + 
                 "&units=" + units;
    
    HTTPClient http;
    http.begin(url);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        Serial.println("API Response: " + payload);
        
        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            // Extract weather data
            current_temp = String((float)doc["main"]["temp"], 1) + "°C";
            feels_like = String((float)doc["main"]["feels_like"], 1) + "°C";
            current_humidity = String((int)doc["main"]["humidity"]) + "%";
            wind_speed = String((float)doc["wind"]["speed"], 1) + " m/s";
            
            // Get weather condition safely
            const char* weather = doc["weather"][0]["main"] | "Unknown";
            current_condition = weather;
            
            Serial.println("Weather data parsed successfully");
        } else {
            Serial.println("JSON parsing error");
            current_condition = "Parse Error";
        }
    } else {
        Serial.print("HTTP Error: ");
        Serial.println(httpCode);
        current_condition = "API Error";
    }
    
    http.end();
}

void displayWeather() {
    Serial.println("Displaying weather...");
    
    // Use a single buffer to avoid heap fragmentation
    char display_buffer[200];
    
    // LEFT COLUMN: City and Temperature
    int left_x = 20;
    int left_y = 80;
    
    // City (large font)
    writeln((GFXfont *)&Lexend40, city.c_str(), &left_x, &left_y, NULL);
    
    // Temperature (large font)
    left_x = 20;
    left_y += 100;
    writeln((GFXfont *)&Lexend40, current_temp.c_str(), &left_x, &left_y, NULL);
    
    // RIGHT COLUMN: Condition and details
    int right_x = 500;
    int right_y = 80;

    // Condition
    writeln((GFXfont *)&Lexend18, current_condition.c_str(), &right_x, &right_y, NULL);

    // Feels like
    right_x = 500;
    right_y += 80;
    snprintf(display_buffer, sizeof(display_buffer), "Feels: %s", feels_like.c_str());
    writeln((GFXfont *)&Lexend18, display_buffer, &right_x, &right_y, NULL);

    // Humidity
    right_x = 500;
    right_y += 60;
    snprintf(display_buffer, sizeof(display_buffer), "Humidity: %s", current_humidity.c_str());
    writeln((GFXfont *)&Lexend18, display_buffer, &right_x, &right_y, NULL);

    // Wind
    right_x = 500;
    right_y += 60;
    snprintf(display_buffer, sizeof(display_buffer), "Wind: %s", wind_speed.c_str());
    writeln((GFXfont *)&Lexend18, display_buffer, &right_x, &right_y, NULL);
}

bool loadConfig() {
    Serial.println("Loading config.json...");
    
    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println("ERROR: Failed to mount LittleFS");
        return false;
    }
    
    // Check if config.json exists
    if (!LittleFS.exists("/config.json")) {
        Serial.println("ERROR: config.json not found");
        Serial.println("Please upload config.json to the device");
        return false;
    }
    
    // Read config.json
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("ERROR: Could not open config.json");
        return false;
    }
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        Serial.print("ERROR: JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Extract WiFi config
    ssid = doc["wifi"]["ssid"].as<String>();
    password = doc["wifi"]["password"].as<String>();
    
    // Extract weather config
    api_key = doc["weather"]["api_key"].as<String>();
    city = doc["weather"]["city"].as<String>();
    country_code = doc["weather"]["country"].as<String>();
    units = doc["weather"]["units"].as<String>();
    
    // Extract update interval (convert minutes to milliseconds)
    int update_minutes = doc["update_interval_minutes"] | 5;
    UPDATE_INTERVAL = update_minutes * 60 * 1000;
    
    // Validate loaded config
    if (ssid.isEmpty() || password.isEmpty() || api_key.isEmpty() || city.isEmpty()) {
        Serial.println("ERROR: Incomplete configuration in config.json");
        return false;
    }
    
    Serial.println("Config loaded successfully!");
    Serial.print("City: ");
    Serial.println(city);
    Serial.print("Update interval: ");
    Serial.print(update_minutes);
    Serial.println(" minutes");
    
    return true;
}
