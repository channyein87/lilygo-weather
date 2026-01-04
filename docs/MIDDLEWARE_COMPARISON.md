# Middleware vs Direct API Mode Comparison

## Quick Comparison

| Feature | Direct API Mode | Middleware Mode |
|---------|----------------|-----------------|
| **Setup Complexity** | Simple | Requires Raspberry Pi/Linux |
| **Battery Life** | 3-5 days | 7-10 days (2-3x improvement) |
| **WiFi Active Time** | 10-15 seconds/update | 1-2 seconds/update |
| **API Calls per Update** | 4+ calls | 1 call |
| **Network Traffic** | High | Low |
| **Processing on ESP32** | High (4 JSON parsers) | Low (1 simple JSON) |
| **Additional Hardware** | None | Raspberry Pi or Linux PC |
| **API Key Location** | On display board | On middleware server |
| **Reconfiguration** | Re-flash device | Edit config & restart container |

## When to Use Each Mode

### Use Direct API Mode When:
- ✅ You want the simplest setup
- ✅ You don't have a Raspberry Pi or always-on Linux machine
- ✅ Battery life of 3-5 days is acceptable
- ✅ You're just getting started
- ✅ You only need weather data (fewer API calls)

### Use Middleware Mode When:
- ✅ You want maximum battery life (7-10 days)
- ✅ You have a Raspberry Pi or Linux machine available
- ✅ You frequently update API keys or configuration
- ✅ You want to minimize ESP32 processing
- ✅ You're using all features (weather, crypto, stocks, trains)
- ✅ You want centralized API management

## Migration Path

### From Direct API to Middleware

1. **Deploy middleware** on Raspberry Pi:
   ```bash
   cd middleware
   mkdir -p config
   cp config.template.json config/config.json
   # Edit config.json with your current API keys
   docker-compose up -d
   ```

2. **Update display board config** (`data/config.json`):
   ```json
   {
     "middleware": {
       "enabled": true,
       "url": "http://192.168.1.100:5000/api/data"
     },
     ...
   }
   ```

3. **Re-upload config**:
   ```bash
   pio run -t uploadfs
   ```

4. **Reboot display** or wait for next update cycle

### From Middleware Back to Direct API

1. **Update display board config** (`data/config.json`):
   ```json
   {
     "middleware": {
       "enabled": false,
       "url": ""
     },
     "weather": {
       "api_key": "YOUR_OPENWEATHERMAP_API_KEY",
       ...
     },
     ...
   }
   ```

2. **Re-upload config**:
   ```bash
   pio run -t uploadfs
   ```

3. **Stop middleware** (optional):
   ```bash
   cd middleware
   docker-compose down
   ```

## Configuration Examples

### Direct API Mode Config
```json
{
  "wifi": {
    "ssid": "MyWiFi",
    "password": "password123"
  },
  "middleware": {
    "enabled": false,
    "url": ""
  },
  "weather": {
    "api_key": "abc123...xyz",
    "city": "Sydney",
    "country": "AU",
    "units": "metric"
  },
  "ntp": {
    "server": "au.pool.ntp.org",
    "timezone": "Australia/Sydney"
  },
  "crypto": {
    "api_key": "CG-abc...xyz",
    "symbol": "btc"
  },
  "stock": {
    "api_key": "def456...uvw",
    "symbol": "AAPL"
  },
  "train": {
    "api_key": "ghi789...rst",
    "origin": "10101100",
    "destination": "10101331"
  },
  "update_interval_minutes": 5
}
```

### Middleware Mode Config
```json
{
  "wifi": {
    "ssid": "MyWiFi",
    "password": "password123"
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

**Note:** In middleware mode, API keys on the display board can be empty - they're configured in the middleware instead.

## Network Traffic Comparison

### Direct API Mode (per update cycle)
```
Display → OpenWeatherMap     (2-4 seconds)
Display → CoinGecko          (1-2 seconds)
Display → MarketStack        (2-3 seconds)
Display → Transport NSW      (2-4 seconds)
────────────────────────────────────────
Total WiFi Active: ~10-15 seconds
```

### Middleware Mode (per update cycle)
```
Display → Middleware         (1-2 seconds)
  ├─ Middleware → OpenWeatherMap
  ├─ Middleware → CoinGecko
  ├─ Middleware → MarketStack
  └─ Middleware → Transport NSW
────────────────────────────────────────
Total Display WiFi Active: ~1-2 seconds
```

## Battery Consumption Breakdown

### Direct API Mode
- **WiFi Connection**: ~200mA for 1-2 seconds
- **Data Transfer**: ~150mA for 10-15 seconds
- **JSON Parsing**: ~80mA for 2-3 seconds
- **Display Update**: ~400mA for 1-2 seconds
- **Deep Sleep**: ~0.1mA between updates
- **Average per 5-minute cycle**: ~25mAh
- **Battery Life (2500mAh)**: ~3-5 days

### Middleware Mode
- **WiFi Connection**: ~200mA for 1-2 seconds
- **Data Transfer**: ~150mA for 1-2 seconds
- **JSON Parsing**: ~50mA for 0.5-1 second
- **Display Update**: ~400mA for 1-2 seconds
- **Deep Sleep**: ~0.1mA between updates
- **Average per 5-minute cycle**: ~10mAh
- **Battery Life (2500mAh)**: ~7-10 days

## Troubleshooting

### Direct API Mode Issues
- **API timeouts**: Check internet connection
- **Rate limiting**: Reduce update interval
- **Parse errors**: Verify API response format hasn't changed

### Middleware Mode Issues
- **Connection refused**: Check middleware IP and port
- **No data**: Verify middleware is running (`docker-compose ps`)
- **Partial data**: Check middleware logs (`docker-compose logs`)
- **Timeout**: Increase middleware timeout or check network

## Best Practices

### Direct API Mode
- Set update interval to 10+ minutes to conserve battery
- Only enable APIs you actually need (comment out unused ones)
- Monitor API rate limits closely

### Middleware Mode
- Use a stable, always-on device for middleware (Raspberry Pi recommended)
- Put middleware on static IP or use hostname
- Monitor middleware health endpoint periodically
- Keep middleware Docker image updated

## Frequently Asked Questions

### Q: Can I use both modes simultaneously?
**A:** No, you must choose one mode. The `middleware.enabled` flag determines which mode is active.

### Q: Does middleware mode require internet on the display board?
**A:** Yes, the display board needs WiFi to connect to the middleware. The middleware then handles internet access to APIs.

### Q: Can I run middleware on the same network as the display?
**A:** Yes, and this is recommended. Both devices should be on the same local network for best performance.

### Q: What if my middleware goes down?
**A:** The display board will show an error. You can either fix the middleware or switch back to direct API mode.

### Q: Can I use middleware for multiple display boards?
**A:** Yes! The middleware can serve multiple display boards simultaneously. Each board makes its own request.

### Q: Is there a mobile app to control the middleware?
**A:** No, but you can use any HTTP client (browser, curl, Postman) to test and monitor the middleware endpoints.

## Summary

**Middleware mode is recommended for:**
- Users who want maximum battery life
- Users with a Raspberry Pi or always-on Linux machine
- Users who frequently update configuration

**Direct API mode is recommended for:**
- New users getting started
- Users without additional hardware
- Users okay with 3-5 day battery life

Both modes are fully supported and maintained. Choose the one that fits your needs!
