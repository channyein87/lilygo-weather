#include <epd_driver.h>
#include <esp_adc_cal.h>
#include <lexend10.h>
#include <lexend14.h>
#include <lexend18.h>
#include <lexend28.h>
#include <lexend32.h>
#include <lexend40.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Timezone.h>
#include <StreamUtils.h>

// Weather icons
#include <owm_01d.h>
#include <owm_01n.h>
#include <owm_02d.h>
#include <owm_02n.h>
#include <owm_03d.h>
#include <owm_03n.h>
#include <owm_04d.h>
#include <owm_04n.h>
#include <owm_09d.h>
#include <owm_09n.h>
#include <owm_10d.h>
#include <owm_10n.h>
#include <owm_11d.h>
#include <owm_11n.h>
#include <owm_13d.h>
#include <owm_13n.h>
#include <owm_50d.h>
#include <owm_50n.h>

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

// Middleware configuration (loaded from config.json)
bool middleware_enabled = false;
String middleware_url = "";

// OpenWeatherMap API (loaded from config.json)
String owm_api_key = "";
String city = "";
String country_code = "";
String units = "";

// NTP Configuration (loaded from config.json)
String ntp_server = "";
String timezone = "";

// CoinGecko API (loaded from config.json)
String coingecko_api_key = "";
String crypto_symbol = "";

// MarketStack API (loaded from config.json)
String marketstack_api_key = "";
String stock_symbol = "";

// TransportNSW API (loaded from config.json)
String transportnsw_api_key = "";
String origin_station_id = "";
String destination_station_id = "";

// Australia Eastern Time Zone (Sydney, Melbourne) - DST aware
// AEDT: UTC+11 (Oct to Apr), AEST: UTC+10 (Apr to Oct)
TimeChangeRule aEDT = {"AEDT", First, Sun, Oct, 2, 660};    // UTC + 11 hours
TimeChangeRule aEST = {"AEST", First, Sun, Apr, 3, 600};    // UTC + 10 hours
Timezone ausET(aEDT, aEST);

// Update interval (in milliseconds) - default 5 minutes
unsigned long UPDATE_INTERVAL = 300000;

// Display dimensions
#define EPD_DISPLAY_WIDTH 960
#define EPD_DISPLAY_HEIGHT 540

// Framebuffer for grayscale image (4 bits per pixel)
uint8_t *framebuffer = NULL;

// ===== GLOBALS =====
unsigned long last_update = 0;
int loop_count = 0;  // Counter for loop iterations
String current_temp = "--";
String current_condition = "Loading...";
String current_humidity = "--";
String wind_speed = "--";
String feels_like = "--";
String temp_max = "--";
String temp_min = "--";
String current_date = "---";
String current_day = "---";
String current_icon_code = "01d";  // Default to clear day icon

// Crypto data
String crypto_price = "--";
String crypto_change = "--";

// Stock data
String stock_price = "--";
String stock_currency = "--";
String stock_change = "--";

// Train data
String train_departure_time = "--:--";
String train_via = "--";

// Structure to hold icon data and dimensions
struct IconData {
    const uint8_t *data;
    uint16_t width;
    uint16_t height;
};

// Get icon data based on icon code from API (e.g., "01d", "02n", etc.)
IconData getIconData(const String &icon_code) {
    // Map icon codes to their respective data and dimensions
    if (icon_code == "01d") {
        return {owm_01d_data, owm_01d_width, owm_01d_height};
    } else if (icon_code == "01n") {
        return {owm_01n_data, owm_01n_width, owm_01n_height};
    } else if (icon_code == "02d") {
        return {owm_02d_data, owm_02d_width, owm_02d_height};
    } else if (icon_code == "02n") {
        return {owm_02n_data, owm_02n_width, owm_02n_height};
    } else if (icon_code == "03d") {
        return {owm_03d_data, owm_03d_width, owm_03d_height};
    } else if (icon_code == "03n") {
        return {owm_03n_data, owm_03n_width, owm_03n_height};
    } else if (icon_code == "04d") {
        return {owm_04d_data, owm_04d_width, owm_04d_height};
    } else if (icon_code == "04n") {
        return {owm_04n_data, owm_04n_width, owm_04n_height};
    } else if (icon_code == "09d") {
        return {owm_09d_data, owm_09d_width, owm_09d_height};
    } else if (icon_code == "09n") {
        return {owm_09n_data, owm_09n_width, owm_09n_height};
    } else if (icon_code == "10d") {
        return {owm_10d_data, owm_10d_width, owm_10d_height};
    } else if (icon_code == "10n") {
        return {owm_10n_data, owm_10n_width, owm_10n_height};
    } else if (icon_code == "11d") {
        return {owm_11d_data, owm_11d_width, owm_11d_height};
    } else if (icon_code == "11n") {
        return {owm_11n_data, owm_11n_width, owm_11n_height};
    } else if (icon_code == "13d") {
        return {owm_13d_data, owm_13d_width, owm_13d_height};
    } else if (icon_code == "13n") {
        return {owm_13n_data, owm_13n_width, owm_13n_height};
    } else if (icon_code == "50d") {
        return {owm_50d_data, owm_50d_width, owm_50d_height};
    } else if (icon_code == "50n") {
        return {owm_50n_data, owm_50n_width, owm_50n_height};
    } else {
        // Default fallback to clear day icon
        return {owm_01d_data, owm_01d_width, owm_01d_height};
    }
}

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

    // Sync time with NTP server
    syncTime();

    // Clear the screen
    epd_clear();

    // Fetch data from middleware or individual APIs
    if (middleware_enabled) {
        Serial.println("Using middleware mode");
        fetchMiddlewareData();
    } else {
        Serial.println("Using direct API mode");
        // Fetch weather data
        fetchWeatherData();
        
        // Fetch crypto data
        fetchCryptoData();
        
        // Fetch stock data
        fetchStockData();
        
        // Fetch train data
        fetchTrainData();
    }
    
    // Display weather
    displayWeather();
    
    // Power down display to save power
    epd_poweroff_all();
    
    Serial.println("Setup complete. Weather displayed.");
}

void loop() {
    // Check if it's time to update
    if (millis() - last_update > UPDATE_INTERVAL) {
        Serial.println("\nTime to refresh data...");
        
        // Power up display
        epd_poweron();
        epd_clear();
        
        // Fetch data from middleware or individual APIs
        if (middleware_enabled) {
            Serial.println("Using middleware mode");
            fetchMiddlewareData();
        } else {
            Serial.println("Using direct API mode");
            // Fetch and display
            fetchWeatherData();
            
            // Fetch train data
            fetchTrainData();
            
            // Only update crypto/stock data every 12 loops
            loop_count++;
            if (loop_count % 12 == 0) {
                fetchCryptoData();
                fetchStockData();
            }
        }
        
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

// Helper function to parse UTC ISO 8601 timestamp and convert to local time string
String parseUTCToLocal(const String& utc_timestamp) {
    if (utc_timestamp.length() < 19) {
        return "--:--";
    }
    
    int year = atoi(utc_timestamp.substring(0, 4).c_str());
    int month = atoi(utc_timestamp.substring(5, 7).c_str());
    int day = atoi(utc_timestamp.substring(8, 10).c_str());
    int hour = atoi(utc_timestamp.substring(11, 13).c_str());
    int minute = atoi(utc_timestamp.substring(14, 16).c_str());
    int second = atoi(utc_timestamp.substring(17, 19).c_str());
    
    struct tm timeinfo = {};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_isdst = 0;
    
    // Use setenv to temporarily set timezone to UTC for mktime
    setenv("TZ", "UTC", 1);
    tzset();
    time_t utc_time = mktime(&timeinfo);
    
    // Convert to local timezone using Timezone library
    TimeChangeRule *tcr;
    time_t local_time = ausET.toLocal(utc_time, &tcr);
    struct tm *local_timeinfo = localtime(&local_time);
    
    // Format as HH:MMAM/PM
    char time_buffer[20];
    int hour_12 = local_timeinfo->tm_hour % 12;
    if (hour_12 == 0) hour_12 = 12;
    const char *am_pm = local_timeinfo->tm_hour >= 12 ? "PM" : "AM";
    snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d%s", hour_12, local_timeinfo->tm_min, am_pm);
    
    return String(time_buffer);
}

void fetchMiddlewareData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping middleware update");
        current_condition = "No WiFi";
        return;
    }
    
    Serial.println("Fetching data from middleware...");
    Serial.print("Middleware URL: ");
    Serial.println(middleware_url);
    
    HTTPClient http;
    http.begin(middleware_url);
    
    // Use GET method for middleware (RESTful data retrieval)
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        Serial.println("Middleware Response received");
        
        // Parse JSON response
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            // Parse weather data
            if (doc.containsKey("weather") && !doc["weather"].containsKey("error")) {
                JsonObject weather = doc["weather"];
                float temp = weather["temp"] | 0.0;
                float feels = weather["feels_like"] | 0.0;
                float t_max = weather["temp_max"] | 0.0;
                float t_min = weather["temp_min"] | 0.0;
                int humidity = weather["humidity"] | 0;
                float wind = weather["wind_speed"] | 0.0;
                const char* condition = weather["condition"] | "Unknown";
                const char* icon = weather["icon_code"] | "01d";
                
                current_temp = String(temp, 0) + "°C";
                feels_like = String(feels, 0) + "°C";
                temp_max = String(t_max, 0);
                temp_min = String(t_min, 0);
                current_humidity = String(humidity) + "%";
                wind_speed = String(wind, 0) + " m/s";
                current_condition = condition;
                current_icon_code = icon;
                
                Serial.println("Weather data parsed from middleware");
            } else {
                Serial.println("Weather data not available from middleware");
            }
            
            // Parse crypto data
            if (doc.containsKey("crypto") && !doc["crypto"].containsKey("error")) {
                JsonObject crypto = doc["crypto"];
                int price = crypto["price"] | 0;
                float change = crypto["change_24h"] | 0.0;
                
                crypto_price = String(price);
                
                char change_buffer[20];
                if (change >= 0) {
                    snprintf(change_buffer, sizeof(change_buffer), "+%.2f", change);
                } else {
                    snprintf(change_buffer, sizeof(change_buffer), "%.2f", change);
                }
                crypto_change = change_buffer;
                
                Serial.println("Crypto data parsed from middleware");
            } else {
                Serial.println("Crypto data not available from middleware");
            }
            
            // Parse stock data
            if (doc.containsKey("stock") && !doc["stock"].containsKey("error")) {
                JsonObject stock = doc["stock"];
                float price = stock["price"] | 0.0;
                const char* currency = stock["currency"] | "USD";
                float change = stock["change"] | 0.0;
                
                stock_currency = currency;
                
                char price_buffer[20];
                snprintf(price_buffer, sizeof(price_buffer), "%.2f", price);
                stock_price = price_buffer;
                
                char change_buffer[20];
                if (change >= 0) {
                    snprintf(change_buffer, sizeof(change_buffer), "+%.2f", change);
                } else {
                    snprintf(change_buffer, sizeof(change_buffer), "%.2f", change);
                }
                stock_change = change_buffer;
                
                Serial.println("Stock data parsed from middleware");
            } else {
                Serial.println("Stock data not available from middleware");
            }
            
            // Parse train data
            if (doc.containsKey("train") && !doc["train"].containsKey("error")) {
                JsonObject train = doc["train"];
                const char* dep_time_utc = train["departure_time"] | "";
                const char* via_dest = train["via"] | "--";
                
                train_via = via_dest;
                
                // Parse ISO 8601 datetime string and convert to local time
                train_departure_time = parseUTCToLocal(dep_time_utc);
                
                Serial.println("Train data parsed from middleware");
            } else {
                Serial.println("Train data not available from middleware");
            }
            
            Serial.println("All data fetched successfully from middleware");
        } else {
            Serial.print("JSON parsing error: ");
            Serial.println(error.c_str());
            current_condition = "Parse Error";
        }
    } else {
        Serial.print("Middleware HTTP Error: ");
        Serial.println(httpCode);
        current_condition = "Middleware Error";
    }
    
    http.end();
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
                 "&appid=" + owm_api_key + 
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
            current_temp = String((float)doc["main"]["temp"], 0) + "°C";
            feels_like = String((float)doc["main"]["feels_like"], 0) + "°C";
            current_humidity = String((int)doc["main"]["humidity"]) + "%";
            wind_speed = String((float)doc["wind"]["speed"], 0) + " m/s";
            temp_max = String((float)doc["main"]["temp_max"], 0);
            temp_min = String((float)doc["main"]["temp_min"], 0);
            
            // Get weather condition safely
            const char* weather = doc["weather"][0]["main"] | "Unknown";
            current_condition = weather;
            
            // Get weather icon code from API response (e.g., "01d", "02n", etc.)
            const char* icon = doc["weather"][0]["icon"] | "01d";
            current_icon_code = icon;
            
            Serial.print("Weather icon code: ");
            Serial.println(current_icon_code);
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

void fetchCryptoData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping crypto update");
        crypto_price = "No WiFi";
        crypto_change = "N/A";
        return;
    }
    
    Serial.println("Fetching crypto data...");
    
    // Build API URL - convert symbol to lowercase for API
    String symbol_lower = crypto_symbol;
    symbol_lower.toLowerCase();
    
    String url = String("https://api.coingecko.com/api/v3/simple/price?vs_currencies=usd&symbols=") + 
                 symbol_lower + 
                 "&include_24hr_change=true";
    
    Serial.println("Crypto API: " + url);

    HTTPClient http;
    http.begin(url);
    
    // Add API key header
    http.addHeader("x-cg-demo-api-key", coingecko_api_key);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        Serial.println("Crypto API Response: " + payload);
        
        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            // Get price from response - the key is the lowercase symbol
            double price = doc[symbol_lower]["usd"] | 0.0;
            double change_24h = doc[symbol_lower]["usd_24h_change"] | 0.0;
            
            // Format price with no decimals and add comma separation
            long price_int = (long)price;
            crypto_price = String(price_int);
            
            // Format change with 2 decimals, add + for positive values
            char change_buffer[20];
            if (change_24h >= 0) {
                snprintf(change_buffer, sizeof(change_buffer), "+%.2f", change_24h);
            } else {
                snprintf(change_buffer, sizeof(change_buffer), "%.2f", change_24h);
            }
            crypto_change = change_buffer;
            
            Serial.print("Crypto price: ");
            Serial.println(crypto_price);
            Serial.print("Crypto change: ");
            Serial.println(crypto_change);
            Serial.println("Crypto data parsed successfully");
        } else {
            Serial.println("JSON parsing error");
            crypto_price = "Parse Error";
            crypto_change = "N/A";
        }
    } else {
        Serial.print("HTTP Error: ");
        Serial.println(httpCode);
        crypto_price = "API Error";
        crypto_change = "N/A";
    }
    
    http.end();
}

void fetchTrainData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping train update");
        train_departure_time = "--:--";
        return;
    }
    
    Serial.println("Fetching train data...");
    
    // Build API URL with required parameters
    String url = String("https://api.transport.nsw.gov.au/v1/tp/trip?") +
                 "outputFormat=rapidJSON&" +
                 "coordOutputFormat=EPSG%3A4326&" +
                 "depArrMacro=dep&" +
                 "type_origin=any&" +
                 "name_origin=" + origin_station_id + "&" +
                 "type_destination=any&" +
                 "name_destination=" + destination_station_id + "&" +
                 "calcNumberOfTrips=1&" +
                 "excludedMeans=checkbox&" +
                 "exclMOT_5=1&" +
                 "TfNSWTR=true&" +
                 "version=10.2.1.42&" +
                 "itOptionsActive=1&" +
                 "cycleSpeed=16";
    
    Serial.println("Train API: " + url);
    
    HTTPClient http;
    const char* keys[] = {"Transfer-Encoding"};
    http.collectHeaders(keys, 1);
    http.begin(url);
    
    // Add authorization header with apikey prefix
    http.addHeader("Authorization", "apikey " + transportnsw_api_key);
    http.addHeader("accept", "application/json");
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        // Get the raw stream
        Stream& rawStream = http.getStream();
        ChunkDecodingStream decodedStream(http.getStream());
        Stream& response = http.header("Transfer-Encoding") == "chunked" ? decodedStream : rawStream;
        
        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response, DeserializationOption::NestingLimit(20));

        if (!error) {
            // Extract train via destination
            const char* via_dest = doc["journeys"][0]["legs"][0]["transportation"]["destination"]["name"] | "--";
            train_via = via_dest;
            
            // Extract departure time in UTC
            const char* dep_time_utc = doc["journeys"][0]["legs"][0]["origin"]["departureTimeEstimated"] | "";
            Serial.print("Departure time UTC: ");
            Serial.println(dep_time_utc);
            
            // Parse ISO 8601 datetime string and convert to local time
            train_departure_time = parseUTCToLocal(dep_time_utc);
            
            Serial.print("Train departure time: ");
            Serial.println(train_departure_time);
            Serial.println("Train data parsed successfully");
        } else {
            Serial.println("JSON parsing error");
            Serial.print("Error code: ");
            Serial.println(error.c_str());
            train_departure_time = "--:--";
        }

        // // Pause for debugging
        // delay(3600000);
        
    } else {
        Serial.print("HTTP Error: ");
        Serial.println(httpCode);
        train_departure_time = "--:--";
    }
    
    http.end();
}

void fetchStockData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping stock update");
        stock_price = "No WiFi";
        stock_currency = "--";
        stock_change = "N/A";
        return;
    }
    
    Serial.println("Fetching stock data...");
    
    // Build API URL
    String url = String("http://api.marketstack.com/v2/eod/latest?access_key=") + 
                 marketstack_api_key + 
                 "&symbols=" + stock_symbol;
    
    Serial.println("Stock API: " + url);

    HTTPClient http;
    http.begin(url);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        Serial.println("Stock API Response: " + payload);
        
        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            // Get stock data from first element in data array
            double open_price = doc["data"][0]["open"] | 0.0;
            double close_price = doc["data"][0]["close"] | 0.0;
            const char* currency = doc["data"][0]["price_currency"] | "--";
            
            // Store currency
            stock_currency = currency;
            
            // Format price with 2 decimals
            char price_buffer[20];
            snprintf(price_buffer, sizeof(price_buffer), "%.2f", close_price);
            stock_price = price_buffer;
            
            // Calculate change: close - open, add + for positive values
            double change = close_price - open_price;
            char change_buffer[20];
            if (change >= 0) {
                snprintf(change_buffer, sizeof(change_buffer), "+%.2f", change);
            } else {
                snprintf(change_buffer, sizeof(change_buffer), "%.2f", change);
            }
            stock_change = change_buffer;
            
            Serial.print("Stock price: ");
            Serial.println(stock_price);
            Serial.print("Stock currency: ");
            Serial.println(stock_currency);
            Serial.print("Stock change: ");
            Serial.println(stock_change);
            Serial.println("Stock data parsed successfully");
        } else {
            Serial.println("JSON parsing error");
            stock_price = "Parse Error";
            stock_currency = "--";
            stock_change = "N/A";
        }
    } else {
        Serial.print("HTTP Error: ");
        Serial.println(httpCode);
        stock_price = "API Error";
        stock_currency = "--";
        stock_change = "N/A";
    }
    
    http.end();
}

void drawBattery(int x, int y) {
    uint8_t percentage = 100;
    float voltage = 0;
    int vref = 1100;
    
    // Initialize ADC calibration
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_1, 
        ADC_ATTEN_DB_11, 
        ADC_WIDTH_BIT_12, 
        1100, 
        &adc_chars
    );
    
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        vref = adc_chars.vref;
        Serial.printf("eFuse Vref: %u mV\n", adc_chars.vref);
    }
    
    // Read battery ADC pin (GPIO 14 for ESP32-S3)
    const uint8_t bat_adc_pin = 14;
    voltage = analogRead(bat_adc_pin) / 4096.0 * 6.566 * (vref / 1000.0);
    
    if (voltage > 1) {  // Only display if valid reading
        Serial.printf("Battery Voltage: %.2f V\n", voltage);
        
        // Polynomial formula to convert voltage to percentage
        // (calibrated for Li-Po battery 3.2V min - 4.2V max)
        percentage = 2836.9625 * pow(voltage, 4) 
                   - 43987.4889 * pow(voltage, 3) 
                   + 255233.8134 * pow(voltage, 2) 
                   - 656689.7123 * voltage 
                   + 632041.7303;
        
        // Clamp values
        if (voltage >= 4.20) percentage = 100;
        if (voltage <= 3.20) percentage = 0;
        
        // Format battery text: "100% 4.2v"
        char battery_text[20];
        snprintf(battery_text, sizeof(battery_text), "Battery: %d%% %.1fv", percentage, voltage);
        
        // Display battery info
        int bat_x = x;
        int bat_y = y;
        writeln((GFXfont *)&Lexend10, battery_text, &bat_x, &bat_y, NULL);
        
        Serial.println(battery_text);
    }
}

void displayWeather() {
    Serial.println("Displaying weather...");
    
    // Update date and time
    updateDate();
    
    // Use a single buffer to avoid heap fragmentation
    char display_buffer[200];
    
    // LEFT COLUMN: City and Temperature
    int left_x = 240;
    int left_y = 40;
    int x1, y1, w, h;
    int xx = left_x, yy = left_y;
    
    // City (large font)
    // left_x = 240;
    // left_y += 40;
    // xx = left_x, yy = left_y;
    get_text_bounds((GFXfont *)&Lexend28, city.c_str(), &xx, &yy, &x1, &y1, &w, &h, NULL);
    int city_x = left_x - w / 2;
    int city_y = left_y + h;
    writeln((GFXfont *)&Lexend28, city.c_str(), &city_x, &city_y, NULL);
    
    // Draw icon based on weather condition
    left_x = 190;
    left_y += 80;
    
    // Get the appropriate icon data based on API response
    IconData icon = getIconData(current_icon_code);
    
    Rect_t area = {
        .x = left_x,
        .y = left_y,
        .width = icon.width,
        .height = icon.height,
    };
    epd_draw_grayscale_image(area, (uint8_t *)icon.data);
    epd_draw_image(area, (uint8_t *)icon.data, BLACK_ON_WHITE);

    // Temperature (large font)
    left_x = 240;
    left_y += 120;
    // left_y = 300;
    xx = left_x, yy = left_y;
    get_text_bounds((GFXfont *)&Lexend40, current_temp.c_str(), &xx, &yy, &x1, &y1, &w, &h, NULL);
    int tmp_x = left_x - w / 2;
    int tmp_y = left_y + h;
    writeln((GFXfont *)&Lexend40, current_temp.c_str(), &tmp_x, &tmp_y, NULL);
    
    // High/Low temperature
    left_x = 240;
    left_y += 80;
    snprintf(display_buffer, sizeof(display_buffer), "H %s - L %s", temp_max.c_str(), temp_min.c_str());
    xx = left_x, yy = left_y;
    get_text_bounds((GFXfont *)&Lexend18, display_buffer, &xx, &yy, &x1, &y1, &w, &h, NULL);
    int tmpr_x = left_x - w / 2;
    int tmpr_y = left_y + h;
    writeln((GFXfont *)&Lexend18, display_buffer, &tmpr_x, &tmpr_y, NULL);

    // Train schedule
    left_x = 20;
    left_y += 100;
    snprintf(display_buffer, sizeof(display_buffer), "Train to city : %s", train_departure_time.c_str());
    writeln((GFXfont *)&Lexend14, display_buffer, &left_x, &left_y, NULL);

    // Train info
    left_x = 20;
    left_y += 40;
    writeln((GFXfont *)&Lexend14, train_via.c_str(), &left_x, &left_y, NULL);

    // RIGHT COLUMN: Condition and details
    int right_x = 500;
    int right_y = 80;

    // Day date
    writeln((GFXfont *)&Lexend28, (current_day + " " + current_date).c_str(), &right_x, &right_y, NULL);

    // Condition
    right_x = 500;
    right_y += 80;
    writeln((GFXfont *)&Lexend18, current_condition.c_str(), &right_x, &right_y, NULL);

    // Feels like
    right_x = 500;
    right_y += 60;
    snprintf(display_buffer, sizeof(display_buffer), "Feels like: %s", feels_like.c_str());
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

    // Stock market
    right_x = 500;
    right_y += 80;
    String upper_stock = stock_symbol;
    upper_stock.toUpperCase();
    snprintf(display_buffer, sizeof(display_buffer), "%s: %s %s, %s", upper_stock.c_str(), stock_currency.c_str(), stock_price.c_str(), stock_change.c_str());
    writeln((GFXfont *)&Lexend14, display_buffer, &right_x, &right_y, NULL);

    // Crypto market
    right_x = 500;
    right_y += 40;
    String upper_symbol = crypto_symbol;
    upper_symbol.toUpperCase();
    snprintf(display_buffer, sizeof(display_buffer), "%s: USD %s, %s", upper_symbol.c_str(), crypto_price.c_str(), crypto_change.c_str());
    writeln((GFXfont *)&Lexend14, display_buffer, &right_x, &right_y, NULL);
    
    // BOTTOM: Last updated timestamp
    time_t utc = time(nullptr);
    TimeChangeRule *tcr;
    time_t local = ausET.toLocal(utc, &tcr);
    
    struct tm *timeinfo = localtime(&local);
    char time_buffer[30];
    strftime(time_buffer, sizeof(time_buffer), "Update: %d %b %Y @ %H:%M", timeinfo);
    
    left_x = 20;
    left_y = EPD_DISPLAY_HEIGHT - 20;
    writeln((GFXfont *)&Lexend10, time_buffer, &left_x, &left_y, NULL);

    // Battery info
    right_x = 500;
    right_y = EPD_DISPLAY_HEIGHT - 20;
    drawBattery(right_x, right_y);
}

void syncTime() {
    Serial.println("Syncing time with NTP server...");
    Serial.print("Timezone: ");
    Serial.println(timezone);
    
    // Configure NTP sync with UTC
    configTime(0, 0, ntp_server.c_str());
    
    // Wait for time to be set
    Serial.print("Waiting for NTP time sync: ");
    time_t utc = time(nullptr);
    int attempts = 0;
    while (utc < 24 * 3600 && attempts < 30) {
        delay(500);
        Serial.print(".");
        utc = time(nullptr);
        attempts++;
    }
    Serial.println();
    
    if (utc > 24 * 3600) {
        Serial.println("Time synchronized!");
        
        // Convert UTC to local time using Timezone library
        TimeChangeRule *tcr;
        time_t local = ausET.toLocal(utc, &tcr);
        
        // Print current time in local timezone
        char time_buffer[50];
        snprintf(time_buffer, sizeof(time_buffer), "Local time: %d-%02d-%02d %02d:%02d:%02d %s",
                 year(local), month(local), day(local),
                 hour(local), minute(local), second(local),
                 tcr->abbrev);
        Serial.println(time_buffer);
    } else {
        Serial.println("WARNING: Could not sync time with NTP server");
    }
}

void updateDate() {
    // Get current UTC time from system clock
    time_t utc = time(nullptr);
    
    // Convert to local time using Timezone library
    TimeChangeRule *tcr;
    time_t local = ausET.toLocal(utc, &tcr);
    
    // Convert to struct tm for easier manipulation
    struct tm *timeinfo = localtime(&local);
    
    // Format: "Tue 30 Dec"
    char day_buffer[4];
    char date_buffer[12];
    
    strftime(day_buffer, sizeof(day_buffer), "%a", timeinfo);
    strftime(date_buffer, sizeof(date_buffer), "%d %b", timeinfo);
    
    current_day = day_buffer;
    current_date = date_buffer;
    
    Serial.print("Current date/time: ");
    Serial.print(current_day);
    Serial.print(" ");
    Serial.println(current_date);
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
    
    // Extract middleware config (optional)
    if (doc.containsKey("middleware")) {
        middleware_enabled = doc["middleware"]["enabled"] | false;
        middleware_url = doc["middleware"]["url"].as<String>();
        
        Serial.print("Middleware enabled: ");
        Serial.println(middleware_enabled ? "YES" : "NO");
        if (middleware_enabled) {
            Serial.print("Middleware URL: ");
            Serial.println(middleware_url);
        }
    }
    
    // Extract weather config
    owm_api_key = doc["weather"]["api_key"].as<String>();
    city = doc["weather"]["city"].as<String>();
    country_code = doc["weather"]["country"].as<String>();
    units = doc["weather"]["units"].as<String>();
    
    // Extract NTP config
    ntp_server = doc["ntp"]["server"].as<String>();
    timezone = doc["ntp"]["timezone"].as<String>();

    // Extract CoinGecko config
    coingecko_api_key = doc["crypto"]["api_key"].as<String>();
    crypto_symbol = doc["crypto"]["symbol"].as<String>();
    
    // Extract MarketStack config
    marketstack_api_key = doc["stock"]["api_key"].as<String>();
    stock_symbol = doc["stock"]["symbol"].as<String>();
    
    // Extract Transport NSW train trip config
    transportnsw_api_key = doc["train"]["api_key"].as<String>();
    origin_station_id = doc["train"]["origin"].as<String>();
    destination_station_id = doc["train"]["destination"].as<String>();
    
    // Extract update interval (convert minutes to milliseconds)
    int update_minutes = doc["update_interval_minutes"] | 5;
    UPDATE_INTERVAL = update_minutes * 60 * 1000;
    
    // Validate loaded config
    // WiFi and NTP are always required
    if (ssid.isEmpty() || password.isEmpty() || ntp_server.isEmpty() || timezone.isEmpty()) {
        Serial.println("ERROR: WiFi or NTP configuration missing in config.json");
        return false;
    }
    
    // If middleware is enabled, validate middleware URL
    if (middleware_enabled) {
        if (middleware_url.isEmpty()) {
            Serial.println("ERROR: Middleware enabled but URL not provided");
            return false;
        }
        
        // Basic URL format validation: require http:// or https:// prefix
        if (!(middleware_url.startsWith("http://") || middleware_url.startsWith("https://"))) {
            Serial.println("ERROR: Middleware URL must start with \"http://\" or \"https://\"");
            return false;
        }
        
        Serial.println("Config validated for middleware mode");
        Serial.print("Middleware URL: ");
        Serial.println(middleware_url);
    } else {
        // Direct API mode: validate required API keys
        if (owm_api_key.isEmpty() || city.isEmpty()) {
            Serial.println("ERROR: Weather API configuration missing in config.json");
            return false;
        }
        Serial.println("Config validated for direct API mode");
    }
    
    Serial.println("Config loaded successfully!");
    Serial.print("City: ");
    Serial.println(city);
    Serial.print("Update interval: ");
    Serial.print(update_minutes);
    Serial.println(" minutes");
    
    return true;
}
