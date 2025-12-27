#include <epd_driver.h>
#include <firasans.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/*
 * LILYGO T5 4.7" E-Paper Display - Weather Display
 * 
 * Connects to WiFi and fetches weather data from OpenWeatherMap API
 * Displays current weather, temperature, and forecast on e-paper display
 */

// ===== CONFIGURATION =====
// Edit these settings for your setup

// Display dimensions
#define EPD_DISPLAY_WIDTH 960
#define EPD_DISPLAY_HEIGHT 540

// Framebuffer for grayscale image (4 bits per pixel)
uint8_t *framebuffer = NULL;

// WiFi credentials
const char* ssid = "Burmese";
const char* password = "sonesone87";

// OpenWeatherMap API
const char* api_key = "ae6bbea13ff0bac412f6f90226852318";           // Get from openweathermap.org
const char* city = "Sydney";                     // City name
const char* country_code = "AU";                // Country code (ISO 3166)
const char* units = "metric";                   // "metric" for Celsius, "imperial" for Fahrenheit

// Update interval (in milliseconds) - 5 minutes (300000 ms)
const unsigned long UPDATE_INTERVAL = 300000;

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
    writeln((GFXfont *)&FiraSans, "Weather Display", &cursor_x, &cursor_y, NULL);
    
    // Connect to WiFi
    cursor_x = 200;
    cursor_y = 200;
    writeln((GFXfont *)&FiraSans, "Connecting to WiFi...", &cursor_x, &cursor_y, NULL);
    connectToWiFi();
    
    // Stop if WiFi connection failed
    if (WiFi.status() != WL_CONNECTED) {
        int cursor_x = 10;
        int cursor_y = 50;
        writeln((GFXfont *)&FiraSans, "WiFi Connection Failed", &cursor_x, &cursor_y, NULL);
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
    WiFi.begin(ssid, password);
    
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
    
    int cursor_x = 20;
    int cursor_y = 40;
    
    // Use a single buffer to avoid heap fragmentation
    char display_buffer[200];
    
    // Line 1: City
    cursor_x = 20;
    writeln((GFXfont *)&FiraSans, city, &cursor_x, &cursor_y, NULL);
    cursor_y += 60;
    
    // Line 2: Temperature
    cursor_x = 20;
    writeln((GFXfont *)&FiraSans, current_temp.c_str(), &cursor_x, &cursor_y, NULL);
    cursor_y += 60;
    
    // Line 3: Condition
    cursor_x = 20;
    writeln((GFXfont *)&FiraSans, current_condition.c_str(), &cursor_x, &cursor_y, NULL);
    cursor_y += 60;
    
    // Line 4-6: Combined weather details
    cursor_x = 20;
    snprintf(display_buffer, sizeof(display_buffer), "Feels: %s", feels_like.c_str());
    writeln((GFXfont *)&FiraSans, display_buffer, &cursor_x, &cursor_y, NULL);
    cursor_y += 60;
    
    cursor_x = 20;
    snprintf(display_buffer, sizeof(display_buffer), "Humidity: %s", current_humidity.c_str());
    writeln((GFXfont *)&FiraSans, display_buffer, &cursor_x, &cursor_y, NULL);
    cursor_y += 60;
    
    cursor_x = 20;
    snprintf(display_buffer, sizeof(display_buffer), "Wind: %s", wind_speed.c_str());
    writeln((GFXfont *)&FiraSans, display_buffer, &cursor_x, &cursor_y, NULL);
}
