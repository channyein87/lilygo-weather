# Configuration Setup

## Overview
This project reads configuration (WiFi credentials, API keys, preferences) from a `config.json` file stored in the device's LittleFS filesystem. This prevents accidentally committing credentials to version control and allows easy reconfiguration without recompiling firmware.

The display supports two operating modes:

### Direct API Mode (Default)
The display board directly calls each API service (weather, crypto, stock, train). This is the simplest setup but uses more battery power.

### Middleware Mode (Recommended for Battery Life)
The display board makes a single request to a local middleware service that aggregates data from all upstream APIs. This significantly improves battery life (2-3x improvement).

The display supports multiple data sources:
- **Weather** (Google Weather API) - Required in direct mode
- **Cryptocurrency** (CoinGecko) - Optional
- **Stock Market** (MarketStack) - Optional
- **Train Schedules** (Transport NSW) - Optional, Sydney area only
- **Time Sync** (NTP) - Always required
- **Middleware** - Optional, provides aggregated data

## Setup Steps

### Option 1: Direct API Mode (Default)

#### 1. Copy the Template
```bash
cd data
cp config.template.json config.json
```

#### 2. Edit `config.json`

Open `data/config.json` in your editor and fill in your actual values:

```json
{
  "wifi": {
    "ssid": "YOUR_WIFI_SSID",
    "password": "YOUR_WIFI_PASSWORD"
  },
  "middleware": {
    "enabled": false,
    "url": ""
  },
  "weather": {
    "api_key": "YOUR_GOOGLE_MAPS_API_KEY",
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
  "update_interval_minutes": 5,
  "schedule": {
    "sleep": "23:00",
    "wakeup": "06:00"
  }
}
```

**Important:**
- WiFi SSID and password are case-sensitive
- Google Maps API key is required in direct mode
- NTP server and timezone are required for accurate time display
- CoinGecko, MarketStack, and Transport NSW APIs are optional
- Sleep schedule is optional (leave empty to disable)
- City name should be in English (e.g., "Sydney" not "Sídney")
- Country code must be 2-letter ISO 3166 code

### 3. Get API Keys

#### Google Maps Platform (Required)
- Visit [Google Cloud Console](https://console.cloud.google.com/apis)
- Create a new project or select an existing one
- Enable the **Geocoding API** and **Weather API**
- Go to **Credentials** → **Create Credentials** → **API Key**
- Copy your API key (format: "AIzaSy...")
- Paste it into `config.json` in the `weather.api_key` field
- **Note:** Free tier includes 28,500 API calls per month

#### CoinGecko (Optional)
- Visit [coingecko.com](https://www.coingecko.com/en/api)
- Click "Get Your Free API Key"
- Sign up for a free account
- Copy your API key (starts with "CG-")
- Paste it into `config.json` in the `crypto.api_key` field
- Choose your cryptocurrency symbol (e.g., "btc", "eth", "ada")

#### MarketStack (Optional)
- Visit [marketstack.com](https://marketstack.com/)
- Click "Get Free API Key"
- Sign up for a free account
- Copy your API key from the dashboard
- Paste it into `config.json` in the `stock.api_key` field
- Choose your stock symbol (e.g., "AAPL", "MSFT", "GOOGL")
- **Note:** Free tier limited to 100 API calls per month (HTTP only)

#### Transport NSW (Optional - Sydney Trains Only)
- Visit [opendata.transport.nsw.gov.au](https://opendata.transport.nsw.gov.au/)
- Click "Register" and create an account
- Go to your account dashboard
- Request an API key
- Copy your API key
- Paste it into `config.json` in the `train.api_key` field
- Find your origin and destination station IDs:
  - Browse the [station list](https://opendata.transport.nsw.gov.au/documentation) or use the Trip Planner API
  - Example: Town Hall = "10101100", Central = "10101331"

---

### Option 2: Middleware Mode (Better Battery Life)

#### 1. Deploy Middleware Service

First, set up the middleware service on a Raspberry Pi or Linux machine. See the [Middleware Setup Guide](middleware/MIDDLEWARE_SETUP.md) for detailed instructions.

Quick deployment:
```bash
cd middleware
mkdir -p config
cp config.template.json config/config.json
# Edit config/config.json with your API keys
docker-compose up -d
```

Find your middleware IP address:
```bash
hostname -I
# Example output: 192.168.1.100
```

#### 2. Configure Display Board

Edit `data/config.json` with minimal settings:

```json
{
  "wifi": {
    "ssid": "YOUR_WIFI_SSID",
    "password": "YOUR_WIFI_PASSWORD"
  },
  "middleware": {
    "enabled": true,
    "url": "http://192.168.1.100:5000/api/data"
  },
  "ntp": {
    "server": "au.pool.ntp.org",
    "timezone": "Australia/Sydney"
  },
  "weather": {
    "api_key": "",
    "city": "Sydney",
    "country": "AU",
    "units": "metric"
  },
  "crypto": {
    "api_key": "",
    "symbol": "btc"
  },
  "stock": {
    "api_key": "",
    "symbol": "AAPL"
  },
  "train": {
    "api_key": "",
    "origin": "10101100",
    "destination": "10101331"
  },
  "update_interval_minutes": 5
}
```

**Important for Middleware Mode:**
- Set `middleware.enabled` to `true`
- Set `middleware.url` to your middleware IP and port
- API keys can be left empty (they're configured in the middleware)
- WiFi and NTP settings are still required
- City, symbols, and other settings can be left for reference but are ignored

#### 3. Test Middleware Connection

Before uploading to the device, verify the middleware is accessible:

```bash
# From your computer on the same network:
curl http://192.168.1.100:5000/health
# Should return: {"status":"healthy","timestamp":"..."}

curl http://192.168.1.100:5000/api/data
# Should return aggregated data from all APIs
```

---

### Upload Configuration to Device (Both Modes)

#### 4. Upload Configuration to Device
```bash
pio run -t uploadfs
```

This uploads the LittleFS filesystem containing `config.json` to your ESP32.

**First Time Setup:** You should do this once before uploading firmware. If you see errors, try again - sometimes the first attempt fails.

#### 5. Upload Firmware
```bash
pio run -t upload
```

This uploads the firmware to your ESP32. The device will now read configuration from the stored `config.json`.

#### 6. Verify Success
```bash
pio device monitor -b 115200
```

Watch the serial output for:
- "Config loaded successfully" - configuration was read
- "Middleware enabled: YES" or "Middleware enabled: NO" - mode detected
- "WiFi connected!" - device connected to WiFi
- "Weather data parsed successfully" - Weather API call worked
- "Crypto data parsed successfully" - CoinGecko API call worked (if configured)
- "Stock data parsed successfully" - MarketStack API call worked (if configured)
- "Train data parsed successfully" - Transport NSW API call worked (if configured)

## Configuration Options

### WiFi Settings (Required - Both Modes)
| Key | Type | Example | Notes |
|-----|------|---------|-------|
| `wifi.ssid` | String | "MyNetwork" | Your WiFi network name (2.4GHz only) |
| `wifi.password` | String | "password123" | Your WiFi password |

### Middleware Settings (Optional)
| Key | Type | Example | Notes |
|-----|------|---------|-------|
| `middleware.enabled` | Boolean | true | Enable middleware mode (default: false) |
| `middleware.url` | String | "http://192.168.1.100:5000/api/data" | URL to middleware API endpoint |

**Note:** When middleware is enabled, API keys on the display board are not required - they are configured in the middleware instead.

### Weather Settings (Required in Direct Mode)
| Key | Type | Example | Notes |
|-----|------|---------|-------|
| `weather.api_key` | String | "abc123...xyz" | From openweathermap.org (64 chars, not required in middleware mode) |
| `weather.city` | String | "Sydney" | City name in English |
| `weather.country` | String | "AU" | ISO 3166 country code (2 letters) |
| `weather.units` | String | "metric" | "metric" (°C) or "imperial" (°F) |

### NTP Time Settings (Required)
| Key | Type | Example | Notes |
|-----|------|---------|-------|
| `ntp.server` | String | "au.pool.ntp.org" | NTP server address (use regional pool) |
| `ntp.timezone` | String | "Australia/Sydney" | IANA timezone identifier |

**Common NTP Servers:**
- Australia: `au.pool.ntp.org`
- North America: `north-america.pool.ntp.org`
- Europe: `europe.pool.ntp.org`
- Asia: `asia.pool.ntp.org`

**Common Timezones:**
- Sydney: `Australia/Sydney`
- Melbourne: `Australia/Melbourne`
- Brisbane: `Australia/Brisbane`
- Perth: `Australia/Perth`
- New York: `America/New_York`
- Los Angeles: `America/Los_Angeles`
- London: `Europe/London`
- Paris: `Europe/Paris`
- Tokyo: `Asia/Tokyo`

[Full timezone list →](https://en.wikipedia.org/wiki/List_of_tz_database_time_zones)

### Cryptocurrency Settings (Optional)
| Key | Type | Example | Notes |
|-----|------|---------|-------|
| `crypto.api_key` | String | "CG-abc123...xyz" | From coingecko.com |
| `crypto.symbol` | String | "btc" | Lowercase crypto symbol (auto-mapped to CoinGecko ID) |

**Popular Symbols:** btc, eth, ada, sol, bnb, xrp, doge, dot, matic, avax, link, uni

**Note:** Common symbols are automatically mapped to CoinGecko IDs (e.g., btc → bitcoin). You can also use CoinGecko IDs directly (e.g., "bitcoin", "ethereum").

### Stock Market Settings (Optional)
| Key | Type | Example | Notes |
|-----|------|---------|-------|
| `stock.api_key` | String | "abc123...xyz" | From marketstack.com (free: 100/month) |
| `stock.symbol` | String | "AAPL" | Stock ticker symbol (uppercase) |

**Popular Symbols:** AAPL (Apple), MSFT (Microsoft), GOOGL (Google), TSLA (Tesla), AMZN (Amazon)

### Train Schedule Settings (Optional)
| Key | Type | Example | Notes |
|-----|------|---------|-------|
| `train.api_key` | String | "abc123...xyz" | From opendata.transport.nsw.gov.au |
| `train.origin` | String | "10101100" | Origin station ID |
| `train.destination` | String | "10101331" | Destination station ID |

**Common Sydney Station IDs:**
- Town Hall: `10101100`
- Central: `10101331`
- Circular Quay: `10101120`
- Wynyard: `10101121`
- Martin Place: `10101122`

[Find more station IDs →](https://opendata.transport.nsw.gov.au/documentation)

### Update Settings
| Key | Type | Example | Notes |
|-----|------|---------|-------|
| `update_interval_minutes` | Number | 5 | How often to fetch data (1-60 minutes) |

### Sleep Schedule Settings (Optional)
| Key | Type | Example | Notes |
|-----|------|---------|-------|
| `schedule.sleep` | String | "23:00" | Time to enter sleep mode (24-hour format HH:MM) |
| `schedule.wakeup` | String | "06:00" | Time to resume normal operation (24-hour format HH:MM) |

**Sleep Schedule Feature:**
- **Optional power-saving feature** that pauses display updates during specified hours
- Leave both fields empty or omit the `schedule` section to disable
- Both `sleep` and `wakeup` times must be provided to enable the schedule
- Times must be in 24-hour format: `HH:MM` (e.g., `"23:00"`, `"06:00"`)
- Supports sleep periods that span midnight (e.g., sleep at 23:00, wake at 06:00)
- During sleep period:
  - Display stops fetching data and updating
  - Device enters low-power state with longer check intervals
  - Extends battery life significantly for overnight periods
- At wakeup time, display automatically resumes normal operation

**Example configurations:**
```json
// Sleep overnight from 11 PM to 6 AM
"schedule": {
  "sleep": "23:00",
  "wakeup": "06:00"
}

// Sleep early morning from 1 AM to 5 AM
"schedule": {
  "sleep": "01:00",
  "wakeup": "05:00"
}

// Disabled (no sleep schedule)
"schedule": {
  "sleep": "",
  "wakeup": ""
}
```

## Important Notes
- **DO NOT** commit `config.json` to git - it's listed in `.gitignore`
- Keep `config.template.json` in the repo as a reference
- **Always upload filesystem first** with `pio run -t uploadfs` before uploading firmware
- Configuration persists on the device - you only need to re-upload if you change settings
- Only OpenWeatherMap and NTP settings are required
- Optional APIs will show placeholder values if not configured

## Troubleshooting

### "Config file not found" Error
- You skipped step 4: `pio run -t uploadfs`
- Run that command first, then upload firmware: `pio run -t upload`

### WiFi Won't Connect
- Double-check SSID and password in `config.json` (case-sensitive!)
- Verify WiFi is **2.4GHz** (5GHz not supported by ESP32)
- Try rebooting the device
- Check serial output: `pio device monitor -b 115200`

### "API Error" or "Timeout"
- **Weather API:** Verify OpenWeatherMap API key is complete (64 characters, no spaces)
- **Crypto API:** Check CoinGecko API key format (starts with "CG-")
- **Stock API:** Verify MarketStack API key, check free tier limits (100/month)
- **Train API:** Check Transport NSW API key, verify station IDs are correct
- Check your account - is API enabled and active?
- Verify location/symbol names are in correct format
- Check if you've exceeded API rate limits

### Cannot Read config.json After Update
- Run: `pio run -t erase && pio run -t uploadfs && pio run -t upload`
- This erases everything and uploads fresh filesystem and firmware

### How to Change Configuration Later
1. Edit `data/config.json` with new values
2. Run: `pio run -t uploadfs` (only this, not upload)
3. Reboot the device or wait for next update cycle
4. Device will use new configuration

No firmware recompilation needed!

## API Rate Limits & Recommendations

### Update Interval Guidelines
- **5 minutes**: Good for crypto/stock tracking (if within API limits)
- **10 minutes**: Balanced for all features
- **30 minutes**: Conservative, extends battery life
- **60 minutes**: Maximum supported

### Free Tier Limits
- **OpenWeatherMap**: 1000 calls/day (≈every 1.5 minutes if used constantly)
- **CoinGecko**: 10,000 calls/month (≈every 4 minutes if used constantly)
- **MarketStack**: 100 calls/month (≈3 times per day max)
- **Transport NSW**: Generous limits, safe for frequent updates

**Recommendation:** Use 5-10 minute updates. MarketStack is the limiting factor - check stock data only during market hours or use longer intervals.

