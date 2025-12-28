# Configuration Setup

## Overview
This project reads sensitive configuration (WiFi credentials, API keys) from a `config.json` file instead of hardcoding them in the firmware. This prevents accidentally committing credentials to version control.

## Setup Steps

### 1. Copy the Template
```bash
cd data
cp config.template.json config.json
```

### 2. Edit `config.json`
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
  "update_interval_minutes": 5
}
```

### 3. Get OpenWeatherMap API Key
- Visit [openweathermap.org](https://openweathermap.org)
- Sign up for a free account
- Go to API section and copy your API key
- Paste it in `config.json`

### 4. Upload Configuration to Device
```bash
pio run -t uploadfs
```

This uploads the LittleFS filesystem containing `config.json` to your ESP32.

### 5. Upload Firmware
```bash
pio run -t upload
```

## Important Notes
- **DO NOT** commit `config.json` to git - it's in `.gitignore`
- Keep `config.template.json` in the repo as a reference
- Always use `uploadfs` first to transfer the config file
- If the device boots and says "config.json not found", the filesystem upload failed - try again

## Configuration Options
| Key | Type | Description |
|-----|------|-------------|
| `wifi.ssid` | String | Your WiFi network name |
| `wifi.password` | String | Your WiFi password |
| `weather.api_key` | String | OpenWeatherMap API key |
| `weather.city` | String | City name for weather |
| `weather.country` | String | ISO 3166 country code |
| `weather.units` | String | `metric` (°C) or `imperial` (°F) |
| `update_interval_minutes` | Number | How often to fetch new weather (1-60) |

