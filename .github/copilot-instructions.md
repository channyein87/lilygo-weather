# AI Coding Agent Instructions for LILYGO T5 Information Display

## Project Overview

This is the **LILYGO T5 Information Display** - a complete information dashboard running on the **LILYGO T5 4.7" e-paper display** with **ESP32-S3**. It displays weather, cryptocurrency prices, stock market data, and train schedules with configurable automatic updates.

### Key Facts
- **Hardware**: LILYGO T5 4.7" with ESP32-S3 and PSRAM
- **Connection**: USB at `/dev/ttyACM0`, 115200 baud serial
- **Build System**: PlatformIO (Arduino framework)
- **Display**: 540×960 pixels, 1-bit (black & white only)
- **Refresh Rate**: ~1-3 seconds for full update
- **Configuration**: JSON file stored in LittleFS (no firmware edits needed)
- **APIs**: 
  - OpenWeatherMap (weather, required)
  - CoinGecko (crypto prices, optional)
  - MarketStack (stock data, optional)
  - Transport NSW (train schedules, optional)

## Build & Upload Workflow

```bash
# First time: Upload configuration to device
pio run -t uploadfs

# Upload firmware
pio run -t upload

# Monitor serial output
pio device monitor -b 115200
```

## Code Structure

```
lilygo-weather/
├── src/
│   └── main.ino                 # Main firmware (weather, crypto, stock, train)
├── data/
│   ├── config.json              # User configuration (WiFi, API keys)
│   └── config.template.json     # Configuration template
├── boards/
│   └── T5-ePaper-S3.json       # Board definition
├── lib/
│   ├── fonts/                   # Lexend font family (10-40 sizes)
│   └── icons/                   # OpenWeatherMap weather icons (32x32)
├── platformio.ini               # Build configuration
├── README.md                    # User guide
├── CONFIG_SETUP.md             # Configuration guide
└── LilyGo_API_Reference.md     # Display library API documentation
```

## Important Implementation Details

### Configuration Loading
- Configuration stored in `data/config.json` via LittleFS
- Device reads WiFi credentials, API keys, preferences from JSON
- Required: WiFi, OpenWeatherMap API, NTP time settings
- Optional: CoinGecko (crypto), MarketStack (stocks), Transport NSW (trains)
- Update interval is configurable (1-60 minutes, default 5)
- **Must upload filesystem first**: `pio run -t uploadfs`

### Firmware Architecture (src/main.ino)
- **Setup**: Initialize display, WiFi, NTP, read config, fetch all data sources
- **Loop**: Sleep for update interval, fetch all data, update display, repeat
- **Data Functions**: 
  - `fetchWeatherData()`: Gets weather from OpenWeatherMap
  - `fetchCryptoData()`: Gets crypto prices from CoinGecko
  - `fetchStockData()`: Gets stock data from MarketStack
  - `fetchTrainData()`: Gets train times from Transport NSW
- **displayWeather()**: Renders all information on screen
- Uses Lexend fonts (sizes 10, 14, 18, 28, 32, 40) for text rendering
- Uses OpenWeatherMap icons (weather_01d, weather_02d, etc.) for conditions

### Display Libraries & Data Sources
- **epd_driver.h**: E-paper control (poweron, poweroff, clear, draw functions)
- **Lexend fonts**: Font files in `lib/fonts/` (lexend10.h through lexend40.h)
- **Weather icons**: Icon files in `lib/icons/` (owm_*.h files for each condition)
- **HTTPClient**: For all API requests
- **ArduinoJson**: For parsing API responses
- **Timezone**: For NTP time conversion

## Configuration Reference

Users must create `data/config.json` from `data/config.template.json` with:
```json
{
  "wifi": {
    "ssid": "YOUR_SSID",
    "password": "YOUR_PASSWORD"
  },
  "weather": {
    "api_key": "YOUR_OPENWEATHERMAP_API_KEY",
    "city": "Sydney",
    "country": "AU",
    "units": "metric"
  },
  "ntp": {
    "server": "au.pool.ntp.org",
    "timezone": "Australia/Sydney"
  },
  "crypto": {
    "api_key": "YOUR_COINGECKO_API_KEY",
    "symbol": "btc"
  },
  "stock": {
    "api_key": "YOUR_MARKETSTACK_API_KEY",
    "symbol": "AAPL"
  },
  "train": {
    "api_key": "YOUR_TRANSPORTNSW_API_KEY",
    "origin": "10101100",
    "destination": "10101331"
  },
  "update_interval_minutes": 5
}
```

**Important constraints:**
- WiFi must be 2.4GHz (ESP32 doesn't support 5GHz)
- City name in English only
- Country code is ISO 3166 (2 letters)
- Units: "metric" (°C) or "imperial" (°F)
- OpenWeatherMap API key: 64 characters (required)
- CoinGecko API key: starts with "CG-" (optional)
- MarketStack API key: free tier 100 calls/month (optional)
- Transport NSW: Sydney trains only (optional)
- NTP timezone: IANA timezone identifier (required)

## Common Development Tasks

### Modify Display Layout
Edit `displayWeather()` function in `src/main.ino` (around line 700):
- Change font sizes (use lexend10 through lexend40)
- Reposition text or icons
- Add/remove displayed information
- Current display shows: weather, crypto, stocks, trains

### Change Update Interval
Edit `data/config.json`:
```json
"update_interval_minutes": 10
```
Then run: `pio run -t uploadfs` (only the filesystem, no firmware change)

### Add New Font Size
1. Add new font file to `lib/fonts/`
2. Include in `src/main.ino`: `#include <newfont.h>`
3. Use in display code: `writeln(&NewFont, "text", &x, &y, NULL)`

### Troubleshooting Serial Output

**Startup messages (should see):**
- "Config loaded successfully" - configuration file found
- "WiFi connected!" - WiFi connected
- "Fetching weather data..." - OpenWeatherMap API call initiated
- "Weather data parsed successfully" - Weather JSON parsing worked
- "Fetching crypto data..." - CoinGecko API call (if configured)
- "Crypto data parsed successfully" - Crypto JSON parsing worked
- "Fetching stock data..." - MarketStack API call (if configured)
- "Stock data parsed successfully" - Stock JSON parsing worked
- "Fetching train data..." - Transport NSW API call (if configured)
- "Train data parsed successfully" - Train JSON parsing worked
- "Displaying information..." - Successfully rendered

**Error messages:**
- "Config file not found" - Forgot to run `pio run -t uploadfs`
- "WiFi connection failed" - Check SSID/password
- "API Error" - Check API keys, rate limits, invalid parameters
- "Failed to parse JSON" - API response format changed or invalid

## Important Constraints

1. **E-Paper is Slow**: Refresh takes 1-3 seconds, don't update more than 1-2× per minute
2. **Display is 1-bit B&W**: No grayscale, colors, or anti-aliasing
3. **LittleFS Required**: Always upload filesystem before firmware
4. **Power Management**: Device sleeps between updates to save battery
5. **WiFi 2.4GHz Only**: ESP32-S3 doesn't support 5GHz networks

## File Organization Rules

- **Firmware**: `src/main.ino` (only file that matters)
- **Configuration**: `data/config.json` (user-created from template)
- **Fonts**: `lib/fonts/lexend*.h` (10, 14, 18, 28, 32, 40 point sizes)
- **Icons**: `lib/icons/owm_*.h` (weather condition icons)
- **Documentation**: README.md, CONFIG_SETUP.md, LilyGo_API_Reference.md
- **Temporary**: `temp/` directory (test files, logs, conversions)

## Debugging Workflow

### Device Not Found
```bash
pio device list              # List connected devices
ls /dev/ttyACM*              # Check USB device exists
```

### Upload Fails
```bash
pio run -t erase            # Erase flash
pio run -t uploadfs         # Upload filesystem
pio run -t upload           # Upload firmware
```

### Config File Not Found on Boot
```bash
pio run -t erase            # Erase everything
pio run -t uploadfs         # Re-upload filesystem
pio run -t upload           # Re-upload firmware
```

### Display Blank / No Weather Showing
1. Check serial output: `pio device monitor -b 115200`
2. Look for error messages (WiFi, API, JSON parsing)
3. Verify config.json has correct values
4. Check API key is active on openweathermap.org

### No Serial Output
- Check USB cable is data cable (not power-only)
- Try different USB port
- Verify device appears in: `pio device list`

## Common Pitfalls

| Issue | Cause | Fix |
|-------|-------|-----|
| "Config file not found" | Skipped `pio run -t uploadfs` | Run filesystem upload first |
| WiFi won't connect | Wrong SSID/password or 5GHz | Check config.json, use 2.4GHz |
| "API Error" on display | Invalid API key or exceeded limits | Verify key on openweathermap.org |
| Display stays blank | Display init failure or bad config | Check serial for errors |
| Weather won't update | Update interval not passed | Wait for configured interval |
| Can't compile | Missing fonts or icons | Check #include statements |

## References

### API Documentation
- **OpenWeatherMap**: https://openweathermap.org/api (weather, free 1000 calls/day)
- **CoinGecko**: https://www.coingecko.com/en/api (crypto, free 10k calls/month)
- **MarketStack**: https://marketstack.com/ (stocks, free 100 calls/month)
- **Transport NSW**: https://opendata.transport.nsw.gov.au/ (trains, free)

### Technical Documentation
- **LittleFS**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/littlefs.html
- **ESP32-S3**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/
- **PlatformIO**: https://docs.platformio.org
- **ArduinoJson**: https://arduinojson.org/
- **Timezone Library**: https://github.com/JChristensen/Timezone

## Project Status

✅ **Production Ready** - Complete information display application  
✅ **Pure C++ Firmware** - No Python dependencies  
✅ **User-Friendly Setup** - 15 minutes from start to working  
✅ **Well-Documented** - README, configuration guide, API reference  
✅ **Configurable** - No firmware edits needed after initial setup  
✅ **Multi-Source Data** - Weather, crypto, stocks, and trains in one display

---

**Project**: LILYGO T5 Information Display  
**Last Updated**: 2026-01-02  
**Status**: Complete and ready for use
