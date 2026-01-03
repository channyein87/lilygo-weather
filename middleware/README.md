# LILYGO T5 Weather Display - Middleware API

A lightweight Flask middleware service that aggregates data from multiple APIs to optimize battery life on the LILYGO T5 display board.

## Quick Start

```bash
# 1. Create config directory and copy template
mkdir -p config
cp config.template.json config/config.json

# 2. Edit config.json with your API keys
nano config/config.json

# 3. Start with Docker Compose
docker-compose up -d

# 4. Verify it's running
curl http://localhost:5000/health
```

## What This Does

Instead of the display board making 4+ API calls (weather, crypto, stock, train), it now makes **1 single request** to this middleware, which:
- Fetches all data from upstream APIs
- Processes and formats the data
- Returns everything in one optimized response

**Result**: 2-3x battery life improvement! 🔋

## API Endpoint

### Main Endpoint
```
POST http://YOUR_IP:5000/api/data
```

Returns all data in one response.

### Individual Endpoints (optional)
```
GET http://YOUR_IP:5000/api/weather
GET http://YOUR_IP:5000/api/crypto
GET http://YOUR_IP:5000/api/stock
GET http://YOUR_IP:5000/api/train
```

## Configuration

Edit `config/config.json`:

```json
{
  "weather": {
    "api_key": "YOUR_OPENWEATHERMAP_API_KEY",
    "city": "Sydney",
    "country": "AU",
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

Only the weather API key is required. Others are optional.

## Deployment

### Docker Compose (Recommended)
```bash
docker-compose up -d
```

### Docker Only
```bash
docker build -t lilygo-middleware .
docker run -d -p 5000:5000 -v ./config:/app/config lilygo-middleware
```

### Python (Development)
```bash
pip install -r requirements.txt
python app.py
```

## Documentation

See [MIDDLEWARE_SETUP.md](MIDDLEWARE_SETUP.md) for detailed setup instructions, deployment on Raspberry Pi, troubleshooting, and more.

## Requirements

- Docker & Docker Compose
- At least one API key (OpenWeatherMap required)
- Machine on the same WiFi network as the display board

## Support

For issues or questions, see the [Troubleshooting section](MIDDLEWARE_SETUP.md#troubleshooting) in the setup guide.
