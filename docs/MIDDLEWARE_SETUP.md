# Middleware Setup Guide

## Overview

The middleware service is a lightweight Python Flask API that aggregates data from multiple upstream services (Google Weather API, CoinGecko, MarketStack, Transport NSW) into a single endpoint. This architecture significantly improves battery life on the LILYGO T5 display board by:

1. **Reducing API calls**: Display board makes 1 request instead of 4+ requests
2. **Offloading processing**: Heavy JSON parsing and data processing happens on the middleware
3. **Network efficiency**: Single HTTP connection instead of multiple connections
4. **Lower power consumption**: Less WiFi active time = longer battery life

## Architecture

```
┌─────────────────┐
│  LILYGO T5      │
│  Display Board  │──────┐
└─────────────────┘      │
                         │ Single POST Request
                         │ (WiFi: ~1-2 seconds)
                         ▼
                  ┌──────────────┐
                  │  Middleware  │
                  │   (Docker)   │
                  └──────────────┘
                         │
        ┌────────────────┼────────────────┬──────────────┐
        │                │                │              │
        ▼                ▼                ▼              ▼
┌──────────────┐ ┌──────────────┐ ┌─────────────┐ ┌──────────────┐
│ Google       │ │  CoinGecko   │ │ MarketStack │ │ Transport NSW│
│ Weather API  │ │     API      │ │     API     │ │     API      │
└──────────────┘ └──────────────┘ └─────────────┘ └──────────────┘
```

## Quick Start

### Prerequisites

- Docker and Docker Compose installed
- Raspberry Pi or any Linux machine on the same WiFi network as the display board
- API keys for the services you want to use (at minimum Google Maps Platform for weather)

### Installation Steps

#### 1. Copy Configuration Template

```bash
cd middleware
mkdir -p config
cp config.template.json config/config.json
```

#### 2. Edit Configuration

Edit `config/config.json` with your API keys:

```json
{
  "weather": {
    "api_key": "YOUR_GOOGLE_MAPS_API_KEY",
    "city": "Sydney",
    "country": "Australia",
    "units": "metric"
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
  }
}
```

**Note**: Only the Google Maps API key (for weather) is required. Other services are optional.

#### 3. Build and Start the Container

```bash
cd middleware
docker-compose up -d
```

#### 4. Verify the Service is Running

```bash
# Check container status
docker-compose ps

# Check logs
docker-compose logs -f

# Test health endpoint
curl http://localhost:5000/health

# Test data endpoint
curl http://localhost:5000/api/data
```

#### 5. Find Your Middleware IP Address

```bash
# On the machine running Docker:
hostname -I
# or
ip addr show
```

Note the IP address (e.g., `192.168.1.100`) - you'll need this for the display board configuration.

## API Endpoints

### Health Check
```
GET /health
```

Returns service health status.

**Response:**
```json
{
  "status": "healthy",
  "timestamp": "2026-01-02T12:00:00.000000"
}
```

### Aggregated Data (Main Endpoint)
```
POST /api/data
GET  /api/data
```

Returns all data from all configured APIs in a single response.

**Response:**
```json
{
  "timestamp": "2026-01-02T12:00:00.000000",
  "weather": {
    "temp": 25.3,
    "feels_like": 24.1,
    "temp_max": 28.0,
    "temp_min": 22.0,
    "humidity": 65,
    "wind_speed": 3.2,
    "condition": "Clouds",
    "icon_code": "02d",
    "city": "Sydney",
    "units": "metric"
  },
  "crypto": {
    "symbol": "BTC",
    "price": 45230,
    "change_24h": -1.23
  },
  "stock": {
    "symbol": "AAPL",
    "price": 195.50,
    "currency": "USD",
    "change": 2.30
  },
  "train": {
    "departure_time": "2026-01-02T14:35:00Z",
    "via": "Central Station"
  }
}
```

### Individual Data Endpoints

If you need to fetch data from specific services:

```
GET /api/weather  - Weather data only
GET /api/crypto   - Crypto data only
GET /api/stock    - Stock data only
GET /api/train    - Train data only
```

## Configuration Options

### Using Config File (Recommended)

Create `config/config.json` with your settings:

```json
{
  "weather": {
    "api_key": "...",
    "city": "Sydney",
    "country": "Australia",
    "units": "metric"
  },
  ...
}
```

### Using Environment Variables

Alternatively, set environment variables in `docker-compose.yml`:

```yaml
environment:
  - OPENWEATHERMAP_API_KEY=your_key_here
  - WEATHER_CITY=Sydney
  - WEATHER_COUNTRY=AU
  - WEATHER_UNITS=metric
  - COINGECKO_API_KEY=your_key_here
  - CRYPTO_SYMBOL=btc
  - MARKETSTACK_API_KEY=your_key_here
  - STOCK_SYMBOL=AAPL
  - TRANSPORTNSW_API_KEY=your_key_here
  - TRAIN_ORIGIN=10101100
  - TRAIN_DESTINATION=10101331
```

## Docker Commands

### Start the Service
```bash
docker-compose up -d
```

### Stop the Service
```bash
docker-compose down
```

### View Logs
```bash
docker-compose logs -f
```

### Restart the Service
```bash
docker-compose restart
```

### Rebuild After Changes
```bash
docker-compose down
docker-compose build --no-cache
docker-compose up -d
```

### Update Configuration
```bash
# Edit config/config.json
nano config/config.json

# Restart to apply changes
docker-compose restart
```

## Deployment on Raspberry Pi

### 1. Install Docker

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Add user to docker group
sudo usermod -aG docker $USER

# Install Docker Compose
sudo apt install docker-compose -y

# Reboot
sudo reboot
```

### 2. Clone Repository and Setup

```bash
# Clone the repository
git clone https://github.com/channyein87/lilygo-weather.git
cd lilygo-weather/middleware

# Create config
mkdir -p config
cp config.template.json config/config.json
nano config/config.json  # Edit with your API keys

# Start the service
docker-compose up -d
```

### 3. Configure Auto-Start on Boot

The `docker-compose.yml` already includes `restart: unless-stopped`, which means the container will automatically start on system reboot.

### 4. Find Raspberry Pi IP Address

```bash
hostname -I
```

Use this IP address in your display board configuration.

## Troubleshooting

### Container Won't Start

```bash
# Check logs
docker-compose logs

# Check if port 5000 is already in use
sudo netstat -tulpn | grep 5000

# If port is in use, change it in docker-compose.yml
```

### API Errors

```bash
# Check logs for specific error messages
docker-compose logs middleware

# Test individual API endpoints
curl http://localhost:5000/api/weather
curl http://localhost:5000/api/crypto
curl http://localhost:5000/api/stock
curl http://localhost:5000/api/train
```

### MarketStack API Quota

The free tier of MarketStack allows only 100 requests per month. The middleware automatically caches stock data for 24 hours to stay within this limit.

**Expected behavior**:
- First request each day: Fetches fresh data from MarketStack
- Subsequent requests: Returns cached data from memory
- Cache age shown in logs: `Returning cached stock data for AAPL (age: 2:15:30)`

**If you exceed the quota**:
- Middleware will return stale cached data with a warning in logs
- Stock data will still be displayed (using last successful fetch)
- Wait until next month for quota to reset, or upgrade to paid tier

**To check your quota usage**:
Visit https://marketstack.com/dashboard and view your API usage statistics.

### Configuration Not Loading

```bash
# Verify config file exists and is valid JSON
cat config/config.json | python -m json.tool

# Check file permissions
ls -la config/

# Ensure config directory is mounted
docker-compose down
docker-compose up -d
```

### Network Issues

```bash
# Verify container is on the correct network
docker network ls
docker network inspect lilygo-network

# Test connectivity from display board
# From your computer, try:
curl http://RASPBERRY_PI_IP:5000/health
```

### High Memory Usage

```bash
# Check resource usage
docker stats

# Adjust worker count in Dockerfile if needed
# Change --workers 2 to --workers 1 for lower memory usage
```

## Performance Optimization

### API Caching

The middleware implements intelligent caching to reduce API calls:

**Stock Data (MarketStack)**:
- **Cache Duration**: 24 hours
- **Reason**: Free tier allows only 100 requests per month (~3 per day)
- **Behavior**: First request of the day fetches fresh data, all subsequent requests return cached data
- **Thread-Safe**: Multiple concurrent requests will only trigger one API call
- **Fallback**: Returns stale cached data if API fails or rate limit is exceeded

**Other APIs** (Weather, Crypto, Train):
- Fetched on every request (no caching)
- These APIs have more generous rate limits

### Reduce API Calls

To further optimize:

1. Increase the display board's update interval
2. Use individual endpoints instead of aggregated endpoint if you don't need all data
3. Disable unused services by removing their API keys

### Lower Memory Footprint

Edit `Dockerfile` and change:
```dockerfile
CMD ["gunicorn", "--bind", "0.0.0.0:5000", "--workers", "1", "--timeout", "120", "app:app"]
```

### Use Raspberry Pi Zero

The middleware is lightweight enough to run on a Raspberry Pi Zero W:
- Memory usage: ~50-100MB
- CPU usage: <5% idle, ~20% during requests
- Network: Minimal bandwidth

## Security Considerations

1. **Keep API keys secure**: Never commit `config/config.json` to version control
2. **Use HTTPS**: Consider putting the middleware behind nginx with SSL
3. **Firewall**: Restrict access to port 5000 to only your local network
4. **Update regularly**: Keep Docker images and dependencies updated

```bash
# Update container
docker-compose pull
docker-compose up -d
```

## Battery Life Improvement

### Before (Direct API Calls)
- WiFi active time: ~10-15 seconds per update
- 4+ API requests per update
- JSON parsing on ESP32
- Battery life: ~3-5 days

### After (Middleware)
- WiFi active time: ~1-2 seconds per update
- 1 API request per update
- Minimal JSON parsing on ESP32
- **Battery life: ~7-10 days** (estimated 2-3x improvement)

## Support

For issues or questions:
1. Check the logs: `docker-compose logs -f`
2. Verify API keys are valid
3. Test endpoints individually
4. Check network connectivity
5. Review Raspberry Pi system logs: `journalctl -u docker`

## Links

- **Google Weather**: https://developers.google.com/maps/documentation/weather
- **CoinGecko**: https://www.coingecko.com/en/api
- **MarketStack**: https://marketstack.com/
- **Transport NSW**: https://opendata.transport.nsw.gov.au/
- **Docker Documentation**: https://docs.docker.com/
- **Flask Documentation**: https://flask.palletsprojects.com/
