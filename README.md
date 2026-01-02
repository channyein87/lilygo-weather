# LILYGO T5 Information Display

A complete information display application for the **LILYGO T5 4.7" e-paper display** featuring weather, cryptocurrency prices, stock market data, and train schedules. Pure C++ firmware with automatic updates, low-power operation, and LittleFS configuration storage.

## Quick Start

### 1. Get API Keys (5 minutes)

**Required:**
- **OpenWeatherMap** (weather data): https://openweathermap.org/api - Free tier: 1000 calls/day

**Optional (for additional features):**
- **CoinGecko** (cryptocurrency prices): https://www.coingecko.com/en/api - Free tier: 30 calls/minute
- **MarketStack** (stock market data): https://marketstack.com/ - Free tier: 100 calls/month
- **Transport NSW** (Sydney train schedules): https://opendata.transport.nsw.gov.au/ - Free with registration

### 2. Configure Device (5 minutes)

Copy the template and edit `data/config.json` with your settings:

```bash
cp data/config.template.json data/config.json
```

```json
{
  "wifi": {
    "ssid": "YOUR_WIFI_SSID",
    "password": "YOUR_WIFI_PASSWORD"
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

**Note:** All API keys except OpenWeatherMap are optional. If you don't provide keys for crypto, stock, or train APIs, those sections will show placeholder values.

### 3. Build & Upload (3 minutes)
```bash
# Upload configuration to device
pio run -t uploadfs

# Upload firmware
pio run -t upload

# Monitor serial output
pio device monitor -b 115200
```

**Done!** Your information dashboard displays on the device and updates automatically at the configured interval.

---

## Features

✅ **Real-time Weather** - Temperature, conditions, humidity, wind speed (OpenWeatherMap)  
✅ **Cryptocurrency Prices** - Live crypto prices with 24h change (CoinGecko)  
✅ **Stock Market Data** - Real-time stock prices and daily changes (MarketStack)  
✅ **Train Schedules** - Next departure times for Sydney trains (Transport NSW)  
✅ **Automatic Time Sync** - NTP-based time synchronization with timezone support  
✅ **Automatic Updates** - Configurable update interval (1-60 minutes)  
✅ **Low Power** - 5-10 day battery life with typical usage  
✅ **WiFi Ready** - Auto-reconnection with error handling  
✅ **Configuration Storage** - All credentials stored in LittleFS, no firmware edits needed  
✅ **Well-Documented** - Complete setup in 15 minutes  

---

## What Gets Displayed

```
LEFT COLUMN:                    RIGHT COLUMN:
Sydney                         Wednesday 27 Dec 2023
25.3°C
                               Partly Cloudy
Max: 28°C | Min: 22°C         Feels like: 24.1°C
                               Humidity: 65%
Train to city: 14:35          Wind: 3.2 m/s NW
via Central
                               AAPL: USD 195.50, +2.30%
                               BTC: USD 45230, -1.20%

Update: 27 Dec 2023 @ 14:30
```

---

## Troubleshooting

### Config File Not Found
- Ensure you ran `pio run -t uploadfs` before `pio run -t upload`
- The config file must be uploaded to the device's LittleFS first
- Check serial output: should show "Config loaded successfully"

### WiFi Won't Connect
- Check SSID and password in `config.json` (case-sensitive)
- Verify device is in WiFi range
- **Must be 2.4GHz WiFi** (5GHz not supported by ESP32)
- View serial output: `pio device monitor -b 115200`
- Try powering on/off the device

### API Error Displayed
- Verify API key is complete (64 characters, no spaces)
- Check OpenWeatherMap account - API enabled?
- Verify city name in English (e.g., "London" not "Londres")
- Verify country code is correct (2-letter ISO code)
- Check if you've exceeded free tier API limits (1000/day)

### Display Blank
- Check USB power connection
- Verify LittleFS was uploaded: `pio run -t uploadfs`
- Erase and reflash: `pio run -t erase && pio run -t uploadfs && pio run -t upload`
- Check serial output for error messages

### No Serial Output
- Check USB cable is data cable (not power-only)
- Try different USB port
- Verify `/dev/ttyACM0` exists: `ls /dev/ttyACM0`
- Check device appears: `pio device list`

---

## Configuration Reference

### WiFi Settings
| Setting | Key | Example | Notes |
|---------|-----|---------|-------|
| WiFi SSID | `wifi.ssid` | "MyNetwork" | Case-sensitive, 2.4GHz only |
| WiFi Password | `wifi.password` | "password123" | Case-sensitive |

### Weather Settings (OpenWeatherMap) - Required
| Setting | Key | Example | Notes |
|---------|-----|---------|-------|
| API Key | `weather.api_key` | "abc123...xyz" | Get from openweathermap.org (64 chars) |
| City | `weather.city` | "Sydney" | English name only |
| Country Code | `weather.country` | "AU" | ISO 3166 (2 letters) |
| Units | `weather.units` | "metric" | "metric" (°C) or "imperial" (°F) |

### Time Settings (NTP) - Required
| Setting | Key | Example | Notes |
|---------|-----|---------|-------|
| NTP Server | `ntp.server` | "au.pool.ntp.org" | Use regional pool for accuracy |
| Timezone | `ntp.timezone` | "Australia/Sydney" | IANA timezone identifier |

### Cryptocurrency Settings (CoinGecko) - Optional
| Setting | Key | Example | Notes |
|---------|-----|---------|-------|
| API Key | `crypto.api_key` | "CG-abc123...xyz" | Get from coingecko.com (free tier available) |
| Symbol | `crypto.symbol` | "btc" | Lowercase crypto symbol (btc, eth, etc.) |

### Stock Market Settings (MarketStack) - Optional
| Setting | Key | Example | Notes |
|---------|-----|---------|-------|
| API Key | `stock.api_key` | "abc123...xyz" | Get from marketstack.com (free: 100/month) |
| Symbol | `stock.symbol` | "AAPL" | Stock ticker symbol (AAPL, MSFT, etc.) |

### Train Schedule Settings (Transport NSW) - Optional
| Setting | Key | Example | Notes |
|---------|-----|---------|-------|
| API Key | `train.api_key` | "abc123...xyz" | Get from opendata.transport.nsw.gov.au |
| Origin | `train.origin` | "10101100" | Station ID (see Transport NSW docs) |
| Destination | `train.destination` | "10101331" | Station ID (see Transport NSW docs) |

### Update Settings
| Setting | Key | Example | Notes |
|---------|-----|---------|-------|
| Update Interval | `update_interval_minutes` | 5 | Minutes between API calls (1-60) |

---

## Common Country Codes

| Country | Code | Example |
|---------|------|---------|
| UK | GB | London |
| USA | US | New York |
| France | FR | Paris |
| Germany | DE | Berlin |
| Japan | JP | Tokyo |
| Australia | AU | Sydney |
| Canada | CA | Toronto |

[Full list →](https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2)

---

## File Structure

```
src/
└── main.ino                 # Firmware (no user edits needed)
data/
├── config.json              # Your settings (created from template)
└── config.template.json     # Configuration template
boards/
└── T5-ePaper-S3.json       # Board definition
lib/
├── fonts/                   # Lexend font family
└── icons/                   # OpenWeatherMap weather icons
platformio.ini              # Build configuration (ready to use)
README.md                   # This file
CONFIG_SETUP.md             # Detailed configuration guide
LilyGo_API_Reference.md     # Display library API documentation
```

---

## Hardware

| Item | Value |
|------|-------|
| Display | LILYGO T5 4.7" e-paper |
| CPU | ESP32-S3 |
| Resolution | 540×960 pixels (B&W) |
| Connection | USB-C serial (115200 baud) |
| Power | 50mW idle, ~1W during update |
| Battery Life | 5-10 days (2500mAh battery) |

---

## API Information

### OpenWeatherMap (Weather Data) - Required
- **Website**: https://openweathermap.org/api
- **Free Tier**: 1000 calls/day, 60 calls/minute
- **Documentation**: https://openweathermap.org/current
- **Endpoint**: `https://api.openweathermap.org/data/2.5/weather`

### CoinGecko (Cryptocurrency Prices) - Optional
- **Website**: https://www.coingecko.com/en/api
- **Free Tier**: 30 calls/minute, 10,000 calls/month
- **Documentation**: https://docs.coingecko.com/reference/simple-price
- **Endpoint**: `https://api.coingecko.com/api/v3/simple/price`
- **Note**: Free tier requires API key (prefix: CG-)

### MarketStack (Stock Market Data) - Optional
- **Website**: https://marketstack.com/
- **Free Tier**: 100 calls/month (limited but sufficient for daily checks)
- **Documentation**: https://marketstack.com/documentation
- **Endpoint**: `http://api.marketstack.com/v2/eod/latest`
- **Note**: Free tier uses HTTP (not HTTPS)

### Transport NSW (Train Schedules) - Optional
- **Website**: https://opendata.transport.nsw.gov.au/
- **Free Tier**: Free with registration
- **Documentation**: https://opendata.transport.nsw.gov.au/documentation
- **Endpoint**: `https://api.transport.nsw.gov.au/v1/tp/trip`
- **Note**: Sydney area trains only, requires station IDs

---

## Customization

### Change Update Interval
Edit `data/config.json`:
```json
"update_interval_minutes": 10
```
Then run `pio run -t uploadfs` to apply (no firmware recompile needed).

### Use Fahrenheit
Edit `data/config.json`:
```json
"weather": {
  "units": "imperial"
}
```

### Different Location
Edit `data/config.json`:
```json
"weather": {
  "city": "Paris",
  "country": "FR"
}
```

### Change Timezone
Edit `data/config.json`:
```json
"ntp": {
  "server": "europe.pool.ntp.org",
  "timezone": "Europe/Paris"
}
```
See [IANA Timezone Database](https://en.wikipedia.org/wiki/List_of_tz_database_time_zones) for valid timezone names.

### Different Cryptocurrency
Edit `data/config.json`:
```json
"crypto": {
  "symbol": "eth"
}
```
Use lowercase symbols: btc, eth, ada, sol, etc. See [CoinGecko API](https://www.coingecko.com/en/api) for available symbols.

### Different Stock
Edit `data/config.json`:
```json
"stock": {
  "symbol": "MSFT"
}
```
Use uppercase ticker symbols. US stocks only for free tier.

### Different Train Route
Edit `data/config.json`:
```json
"train": {
  "origin": "10101100",
  "destination": "10101331"
}
```
Find station IDs in [Transport NSW documentation](https://opendata.transport.nsw.gov.au/documentation).

### Customize Display Layout
Edit the `displayWeather()` function in [src/main.ino](src/main.ino) (around line 700) to change font sizes, positions, or data displayed.

---

## Commands

```bash
# Upload configuration file to device
pio run -t uploadfs

# Upload firmware to device
pio run -t upload

# Upload both config and firmware (recommended first time)
pio run -t uploadfs && pio run -t upload

# Monitor serial output
pio device monitor -b 115200

# Erase flash memory (if upload fails)
pio run -t erase

# Clean build files
pio run -t clean
```

---

## Serial Output Example

**Startup (should see):**
```
=== LILYGO T5 Information Display ===
Starting up...
Connecting to WiFi: MyNetwork
.....
WiFi connected!
IP address: 192.168.1.100
Config loaded successfully
Fetching weather data...
Weather data parsed successfully
Fetching crypto data...
Crypto data parsed successfully
Fetching stock data...
Stock data parsed successfully
Fetching train data...
Train data parsed successfully
Displaying information...
Setup complete. Display updated.
```

**Auto-Update (every configured interval):**
```
Time to refresh data...
Fetching weather data...
Fetching crypto data...
Fetching stock data...
Fetching train data...
Display updated.
```

---

## Success Checklist

### Basic Setup
- [ ] Have WiFi SSID and password ready
- [ ] Got OpenWeatherMap API key
- [ ] Know your city name (English) and country code
- [ ] Created `data/config.json` from template
- [ ] Configured NTP server and timezone
- [ ] Ran `pio run -t uploadfs`
- [ ] Ran `pio run -t upload`
- [ ] Serial shows "WiFi connected!"
- [ ] Serial shows "Config loaded successfully"
- [ ] Weather displays on device

### Optional Features
- [ ] Got CoinGecko API key for crypto prices
- [ ] Got MarketStack API key for stock data
- [ ] Got Transport NSW API key for train schedules
- [ ] Configured desired crypto symbol
- [ ] Configured desired stock symbol
- [ ] Configured train origin and destination
- [ ] Verified all data displays correctly
- [ ] Device updates at configured interval

---

## Links

### API Documentation
- **OpenWeatherMap**: https://openweathermap.org/api
- **CoinGecko**: https://www.coingecko.com/en/api
- **MarketStack**: https://marketstack.com/documentation
- **Transport NSW**: https://opendata.transport.nsw.gov.au/documentation

### Reference
- **Country Codes**: https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
- **Timezone Database**: https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
- **NTP Pool Servers**: https://www.ntppool.org/
- **ESP32 Docs**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/
- **PlatformIO Docs**: https://docs.platformio.org

---

## Status

✅ **Production Ready** - Complete, tested, and ready to use  
✅ **Pure C++** - No Python dependencies  
✅ **Easy Setup** - 10 minutes from start to working weather display  
✅ **Well-Documented** - Clear inline code comments  

---

**Ready to start?** Copy `data/config.template.json` to `data/config.json`, edit your settings (at minimum: WiFi, weather API, and NTP), then run `pio run -t uploadfs && pio run -t upload`!

**Note:** Only the OpenWeatherMap API key is required. CoinGecko, MarketStack, and Transport NSW APIs are optional features.
