# QUICKSTART - Get Weather Display Working in 10 Minutes

## Step 1: Get API Key (2 minutes)

1. Go to: https://openweathermap.org/api
2. Click "Sign Up" and create a free account
3. Go to API keys section
4. Copy your default API key (64-character string starting with letters/numbers)

## Step 2: Configure Your Settings (3 minutes)

Open **`src/main.ino`** and edit **lines 20-26** ONLY:

```cpp
// Line 20: Your WiFi network name
const char* ssid = "YOUR_WIFI_NAME";

// Line 21: Your WiFi password
const char* password = "YOUR_WIFI_PASSWORD";

// Line 24: Paste your API key here (64 characters)
const char* api_key = "YOUR_API_KEY_HERE";

// Line 25: Your city name (English)
const char* city = "London";

// Line 26: Your country code (2-letter ISO code)
const char* country_code = "GB";

// Line 27: Temperature units
const char* units = "metric";  // "metric" for °C, "imperial" for °F
```

**Important:**
- WiFi name and password are **case-sensitive**
- API key must be complete (no spaces, all 64 characters)
- City name must be in **English** (e.g., "London" not "Londres")
- Country code must be **2 letters** (GB, US, FR, DE, JP, AU, CA, etc.)

## Step 3: Build & Upload (3 minutes)

```bash
pio run -t upload
```

Wait for it to compile and upload. Should take ~30 seconds.

## Step 4: Verify It Works (1-2 minutes)

```bash
pio device monitor -b 115200
```

You should see:
```
=== LILYGO T5 Weather Display ===
Starting up...
Connecting to WiFi: YourWiFi
WiFi connected!
IP address: 192.168.1.X
Fetching weather data...
Weather data parsed successfully
Displaying weather...
Setup complete. Weather displayed.
```

**Check your display** - weather should be showing! ✅

---

## It's Working! 🎉

Weather will:
- ✅ Display current temperature, condition, humidity, wind
- ✅ Update automatically every 30 minutes
- ✅ Show timestamp of last update
- ✅ Continue working for 5-10 days on battery

---

## If Something Doesn't Work

### "WiFi Error" on Display or Serial
- Check WiFi name (line 20) - spelling and case matter
- Check WiFi password (line 21) - spelling and case matter
- Check WiFi is 2.4GHz (not 5GHz)
- Make sure device is in WiFi range

### "API Error" on Display or Serial
- Check API key (line 24) is completely pasted (64 characters, no spaces)
- Go to openweathermap.org dashboard - make sure API is activated
- Check city name (line 25) is correct English spelling
- Check country code (line 26) is correct (e.g., "GB" for UK, "US" for USA)

### Display is Blank
- Check USB cable is connected and powered
- Try: `pio run -t erase` then `pio run -t upload` again
- Check serial output for error messages: `pio device monitor -b 115200`

### No Serial Output
- Check USB cable (must be data cable, not just power)
- Try different USB port
- Check if `/dev/ttyACM0` exists: `ls /dev/ttyACM*`

### Still Stuck?
Check the serial output for specific error messages - it usually tells you exactly what's wrong!

---

## Common Country Codes

```
GB = United Kingdom        JP = Japan
US = United States         AU = Australia
CA = Canada               IN = India
FR = France               DE = Germany
```

[Full list →](https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2)

---

## Next Steps

### To Customize:
- Change update interval: Edit line 30 in `src/main.ino`
- Use Fahrenheit: Change line 27 to `"imperial"`
- Change city: Edit lines 25-26 with different location
- Modify display layout: Edit `displayWeather()` function

### To Understand the Code:
- Read inline comments in `src/main.ino`
- Check `README.md` for complete documentation

---

## That's It!

You now have a working weather display that:
- Fetches real-time weather data
- Shows on your e-paper display
- Updates every 30 minutes automatically
- Uses very little power
- Works for days on battery

Enjoy! 🌤️
