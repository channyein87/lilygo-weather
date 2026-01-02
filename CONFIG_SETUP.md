# Configuration Setup

## Overview
This project reads sensitive configuration (WiFi credentials, API keys) from a `config.json` file stored in the device's LittleFS filesystem. This prevents accidentally committing credentials to version control and allows easy reconfiguration without recompiling firmware.

## Setup Steps

### 1. Copy the Template
```bash
cd data
cp config.template.json config.json
```

### 2. Edit `config.json`

Open `data/config.json` in your editor and fill in your actual values:

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
  "update_interval_minutes": 30
}
```

**Important:**
- WiFi SSID and password are case-sensitive
- API key must be complete (64 characters)
- City name should be in English (e.g., "London" not "Londres")
- Country code must be 2-letter ISO 3166 code

### 3. Get OpenWeatherMap API Key
- Visit [openweathermap.org](https://openweathermap.org)
- Click "Sign Up" or "Log In"
- Go to your account → API Keys section
- Copy your API key (should be 64 characters)
- Paste it into `config.json` in the `weather.api_key` field

### 4. Upload Configuration to Device
```bash
pio run -t uploadfs
```

This uploads the LittleFS filesystem containing `config.json` to your ESP32.

**First Time Setup:** You should do this once before uploading firmware. If you see errors, try again - sometimes the first attempt fails.

### 5. Upload Firmware
```bash
pio run -t upload
```

This uploads the firmware to your ESP32. The device will now read configuration from the stored `config.json`.

### 6. Verify Success
```bash
pio device monitor -b 115200
```

Watch the serial output for:
- "Config loaded successfully" - configuration was read
- "WiFi connected!" - device connected to WiFi
- "Weather data parsed successfully" - API call worked

## Configuration Options

| Key | Type | Example | Notes |
|-----|------|---------|-------|
| `wifi.ssid` | String | "MyNetwork" | Your WiFi network name (2.4GHz only) |
| `wifi.password` | String | "password123" | Your WiFi password |
| `weather.api_key` | String | "abc123...xyz" | From openweathermap.org (64 chars) |
| `weather.city` | String | "Sydney" | City name in English |
| `weather.country` | String | "AU" | ISO 3166 country code (2 letters) |
| `weather.units` | String | "metric" | "metric" (°C) or "imperial" (°F) |
| `update_interval_minutes` | Number | 30 | How often to fetch weather (1-60) |

## Important Notes
- **DO NOT** commit `config.json` to git - it's listed in `.gitignore`
- Keep `config.template.json` in the repo as a reference for other developers
- **Always upload filesystem first** with `pio run -t uploadfs` before uploading firmware
- Configuration persists on the device - you only need to re-upload if you change settings

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
- Verify API key is complete (64 characters, no spaces)
- Check your OpenWeatherMap account - is API enabled?
- Verify city name is in English
- Check country code is correct

### Cannot Read config.json After Update
- Run: `pio run -t erase && pio run -t uploadfs && pio run -t upload`
- This erases everything and uploads fresh filesystem and firmware

### How to Change Configuration Later
1. Edit `data/config.json` with new values
2. Run: `pio run -t uploadfs` (only this, not upload)
3. Device will use new configuration on next boot

No firmware recompilation needed!

