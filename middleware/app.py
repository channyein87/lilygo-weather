"""
LILYGO T5 Weather Display - Middleware API
Aggregates data from multiple upstream APIs to optimize battery life on display board
"""

from flask import Flask, jsonify, request
import requests
import os
import json
import logging
from datetime import datetime, timedelta
from concurrent.futures import ThreadPoolExecutor
import threading

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

app = Flask(__name__)

# Load configuration from environment variables or config file
def load_config():
    """Load API keys and configuration"""
    config_file = os.getenv('CONFIG_FILE', 'config.json')
    
    # Prefer config file if it exists
    if os.path.exists(config_file):
        with open(config_file, 'r') as f:
            config_data = json.load(f)
    else:
        # Fallback to environment variables
        config_data = {
            'weather': {
                'api_key': os.getenv('OPENWEATHERMAP_API_KEY', ''),
                'city': os.getenv('WEATHER_CITY', 'Sydney'),
                'country': os.getenv('WEATHER_COUNTRY', 'AU'),
                'units': os.getenv('WEATHER_UNITS', 'metric')
            },
            'crypto': {
                'api_key': os.getenv('COINGECKO_API_KEY', ''),
                'symbol': os.getenv('CRYPTO_SYMBOL', 'btc')
            },
            'stock': {
                'api_key': os.getenv('MARKETSTACK_API_KEY', ''),
                'symbol': os.getenv('STOCK_SYMBOL', 'AAPL')
            },
            'train': {
                'api_key': os.getenv('TRANSPORTNSW_API_KEY', ''),
                'origin': os.getenv('TRAIN_ORIGIN', ''),
                'destination': os.getenv('TRAIN_DESTINATION', '')
            }
        }
    
    # Validate that required configuration is present
    weather_cfg = {}
    if isinstance(config_data, dict):
        weather_cfg = config_data.get('weather', {}) or {}
    api_key = weather_cfg.get('api_key')
    if not api_key:
        raise RuntimeError(
            "Weather API key not configured. "
            "Set 'weather.api_key' in the config file or the OPENWEATHERMAP_API_KEY environment variable."
        )
    
    # Warn about placeholder values
    if api_key.startswith('YOUR_'):
        print("WARNING: Weather API key appears to be a placeholder value. Please update config.json with your actual API key.")
    
    # Check other optional API keys for placeholders
    crypto_key = config_data.get('crypto', {}).get('api_key', '')
    if crypto_key and crypto_key.startswith('YOUR_'):
        print("WARNING: CoinGecko API key appears to be a placeholder value.")
    
    stock_key = config_data.get('stock', {}).get('api_key', '')
    if stock_key and stock_key.startswith('YOUR_'):
        print("WARNING: MarketStack API key appears to be a placeholder value.")
    
    train_key = config_data.get('train', {}).get('api_key', '')
    if train_key and train_key.startswith('YOUR_'):
        print("WARNING: Transport NSW API key appears to be a placeholder value.")
    
    return config_data

config = load_config()

# Cache for stock data (MarketStack has 100 requests/month limit on free tier)
# We cache for 24 hours to make ~1 request per day
stock_cache = {
    'data': None,
    'timestamp': None,
    'lock': threading.Lock()
}

# Cache duration: 24 hours
STOCK_CACHE_DURATION = timedelta(hours=24)

def _get_stale_cached_stock_data(error_type):
    """
    Helper function to retrieve stale cached stock data when API fails.
    Returns cached data if available, otherwise None.
    """
    with stock_cache['lock']:
        if stock_cache['data'] is not None:
            cache_age = datetime.utcnow() - stock_cache['timestamp'] if stock_cache['timestamp'] else None
            logger.warning(f"{error_type}, returning stale cached data (age: {cache_age})")
            return stock_cache['data']
    return None

def fetch_weather_data():
    """Fetch weather data from Google Weather API"""
    try:
        weather_config = config.get('weather', {})
        api_key = weather_config.get('api_key')
        city = weather_config.get('city', 'Sydney')
        country = weather_config.get('country', 'AU')
        units = weather_config.get('units', 'metric')
        
        if not api_key:
            logger.warning("Weather API key not configured")
            return {'error': 'Weather API key not configured'}
        
        logger.info(f"Fetching weather data for {city}, {country}")
        
        # Step 1: Get latitude and longitude using Geocoding API
        geocode_url = f"https://maps.googleapis.com/maps/api/geocode/json?address={city},{country}&key={api_key}"
        logger.info(f"Geocoding request: {city}, {country}")
        
        geocode_response = requests.get(geocode_url, timeout=10)
        geocode_response.raise_for_status()
        geocode_data = geocode_response.json()
        
        if geocode_data.get('status') != 'OK':
            logger.error(f"Geocoding error: {geocode_data.get('status')}")
            return {'error': f"Geocoding error: {geocode_data.get('status')}"}
        
        location = geocode_data['results'][0]['geometry']['location']
        latitude = location['lat']
        longitude = location['lng']
        
        logger.info(f"Location found: Lat={latitude}, Lng={longitude}")
        
        # Step 2: Get weather data using coordinates
        weather_url = f"https://weather.googleapis.com/v1/currentConditions:lookup?key={api_key}&location.latitude={latitude}&location.longitude={longitude}"
        weather_response = requests.get(weather_url, timeout=10)
        weather_response.raise_for_status()
        
        data = weather_response.json()
        
        # Extract weather data from Google Weather API format
        temp_celsius = data.get('temperature', {}).get('degrees', 0.0)
        feels_celsius = data.get('feelsLikeTemperature', {}).get('degrees', 0.0)
        precipitation_prob = data.get('precipitation', {}).get('probability', {}).get('percent', 0)
        
        # Wind speed - check unit and convert to m/s if needed
        wind_value = data.get('wind', {}).get('speed', {}).get('value', 0.0)
        wind_unit = data.get('wind', {}).get('speed', {}).get('unit', 'KILOMETERS_PER_HOUR')
        wind_ms = wind_value
        
        # Convert from km/h to m/s if unit is KILOMETERS_PER_HOUR
        if wind_unit == 'KILOMETERS_PER_HOUR':
            wind_ms = wind_value / 3.6
        
        # Get max/min from history
        history = data.get('currentConditionsHistory', {})
        max_celsius = history.get('maxTemperature', {}).get('degrees', temp_celsius)
        min_celsius = history.get('minTemperature', {}).get('degrees', temp_celsius)
        
        # Get weather condition
        weather_condition = data.get('weatherCondition', {})
        condition_desc = weather_condition.get('description', {}).get('text', 'Unknown')
        condition_type = weather_condition.get('type', 'UNKNOWN')
        is_daytime = data.get('isDaytime', True)
        
        # Convert to imperial if needed
        if units == 'imperial':
            temp_celsius = temp_celsius * 9.0 / 5.0 + 32.0
            feels_celsius = feels_celsius * 9.0 / 5.0 + 32.0
            max_celsius = max_celsius * 9.0 / 5.0 + 32.0
            min_celsius = min_celsius * 9.0 / 5.0 + 32.0
        
        logger.info(f"Weather data fetched successfully - {city}: {temp_celsius}°{units[0].upper()}")
        
        return {
            'temp': round(temp_celsius, 1),
            'feels_like': round(feels_celsius, 1),
            'temp_max': round(max_celsius, 1),
            'temp_min': round(min_celsius, 1),
            'precipitation_prob': precipitation_prob,
            'wind_speed': round(wind_ms, 1),
            'condition': condition_desc,
            'condition_type': condition_type,
            'is_daytime': is_daytime,
            'city': city,
            'units': units
        }
    except requests.exceptions.HTTPError as e:
        status_code = e.response.status_code if e.response else 'unknown'
        logger.error(f"HTTP error fetching weather data: {status_code} - {e}")
        return {'error': f'HTTP {status_code}: {str(e)}'}
    except requests.exceptions.RequestException as e:
        logger.error(f"Network error fetching weather data: {e}")
        return {'error': f'Network error: {str(e)}'}
    except Exception as e:
        logger.error(f"Error fetching weather data: {e}")
        return {'error': str(e)}

def fetch_crypto_data():
    """Fetch cryptocurrency data from CoinGecko API"""
    try:
        crypto_config = config.get('crypto', {})
        api_key = crypto_config.get('api_key')
        symbol = crypto_config.get('symbol', 'btc').lower()
        
        if not api_key:
            logger.warning("Crypto API key not configured")
            return {'error': 'Crypto API key not configured'}
        
        # Map common symbols to CoinGecko IDs
        symbol_to_id = {
            'btc': 'bitcoin',
            'eth': 'ethereum',
            'ada': 'cardano',
            'sol': 'solana',
            'bnb': 'binancecoin',
            'xrp': 'ripple',
            'doge': 'dogecoin',
            'dot': 'polkadot',
            'matic': 'matic-network',
            'avax': 'avalanche-2',
            'link': 'chainlink',
            'uni': 'uniswap'
        }
        
        # Convert symbol to CoinGecko ID if it's a known symbol, otherwise use as-is
        coin_id = symbol_to_id.get(symbol, symbol)
        
        logger.info(f"Fetching crypto data for {symbol.upper()} (ID: {coin_id})")
        url = f"https://api.coingecko.com/api/v3/simple/price?ids={coin_id}&vs_currencies=usd&include_24hr_change=true"
        headers = {'x-cg-demo-api-key': api_key}
        response = requests.get(url, headers=headers, timeout=10)
        response.raise_for_status()
        
        data = response.json()
        
        if coin_id in data:
            price = int(data[coin_id].get('usd', 0))
            change = round(data[coin_id].get('usd_24h_change', 0), 2)
            logger.info(f"Crypto data fetched successfully - {symbol.upper()}: ${price} ({change:+.2f}%)")
            return {
                'symbol': symbol.upper(),
                'price': price,
                'change_24h': change
            }
        else:
            logger.warning(f"Crypto symbol/ID not found: {symbol}")
            return {'error': f'Crypto symbol/ID not found: {symbol}'}
    except requests.exceptions.HTTPError as e:
        status_code = e.response.status_code if e.response else 'unknown'
        logger.error(f"HTTP error fetching crypto data: {status_code} - {e}")
        return {'error': f'HTTP {status_code}: {str(e)}'}
    except requests.exceptions.RequestException as e:
        logger.error(f"Network error fetching crypto data: {e}")
        return {'error': f'Network error: {str(e)}'}
    except Exception as e:
        logger.error(f"Error fetching crypto data: {e}")
        return {'error': str(e)}

def fetch_stock_data():
    """
    Fetch stock market data from MarketStack API with 24-hour caching.
    
    MarketStack free tier allows only 100 requests per month (~3 per day).
    To conserve API quota, we cache the stock data for 24 hours and only
    fetch new data once per day.
    """
    global stock_cache
    
    try:
        stock_config = config.get('stock', {})
        api_key = stock_config.get('api_key')
        symbol = stock_config.get('symbol', 'AAPL').upper()
        
        if not api_key:
            logger.warning("Stock API key not configured")
            return {'error': 'Stock API key not configured'}
        
        # Use lock to ensure thread-safe access and prevent duplicate API calls
        with stock_cache['lock']:
            # Check if we have valid cached data
            if (stock_cache['data'] is not None and 
                stock_cache['timestamp'] is not None):
                
                cache_age = datetime.utcnow() - stock_cache['timestamp']
                
                if cache_age < STOCK_CACHE_DURATION:
                    # Cache is still valid, return cached data
                    logger.info(f"Returning cached stock data for {symbol} (age: {cache_age})")
                    return stock_cache['data']
                else:
                    logger.info(f"Stock cache expired (age: {cache_age}), fetching fresh data")
            
            # Cache miss or expired - fetch fresh data while holding the lock
            # This ensures only one thread fetches data at a time
            logger.info(f"Fetching stock data for {symbol} from MarketStack API")
            url = f"http://api.marketstack.com/v1/eod/latest?access_key={api_key}&symbols={symbol}"
            response = requests.get(url, timeout=10)
            response.raise_for_status()
            
            data = response.json()
            
            if 'data' in data and len(data['data']) > 0:
                stock_data = data['data'][0]
                open_price = stock_data.get('open', 0)
                close_price = stock_data.get('close', 0)
                change = close_price - open_price
                
                result = {
                    'symbol': symbol,
                    'price': round(close_price, 2),
                    'currency': 'USD',  # MarketStack free tier is US stocks only
                    'change': round(change, 2)
                }
                
                # Update cache
                stock_cache['data'] = result
                stock_cache['timestamp'] = datetime.utcnow()
                
                logger.info(f"Stock data fetched and cached successfully - {symbol}: ${close_price:.2f} ({change:+.2f})")
                return result
            else:
                logger.warning(f"Stock symbol not found: {symbol}")
                return {'error': 'Stock symbol not found'}
    except requests.exceptions.HTTPError as e:
        status_code = e.response.status_code if e.response else 'unknown'
        logger.error(f"HTTP error fetching stock data: {status_code} - {e}")
        
        # Try to return stale cached data as fallback
        stale_data = _get_stale_cached_stock_data("API error")
        if stale_data:
            return stale_data
        
        return {'error': f'HTTP {status_code}: {str(e)}'}
    except requests.exceptions.RequestException as e:
        logger.error(f"Network error fetching stock data: {e}")
        
        # Try to return stale cached data as fallback
        stale_data = _get_stale_cached_stock_data("Network error")
        if stale_data:
            return stale_data
        
        return {'error': f'Network error: {str(e)}'}
    except Exception as e:
        logger.error(f"Error fetching stock data: {e}")
        
        # Try to return stale cached data as fallback
        stale_data = _get_stale_cached_stock_data("Unexpected error")
        if stale_data:
            return stale_data
        
        return {'error': str(e)}

def fetch_train_data():
    """Fetch train schedule data from Transport NSW API"""
    try:
        train_config = config.get('train', {})
        api_key = train_config.get('api_key')
        origin = train_config.get('origin')
        destination = train_config.get('destination')
        
        if not api_key or not origin or not destination:
            logger.warning("Train API not fully configured")
            return {'error': 'Train API not fully configured'}
        
        logger.info(f"Fetching train data from {origin} to {destination}")
        url = f"https://api.transport.nsw.gov.au/v1/tp/trip"
        params = {
            'outputFormat': 'rapidJSON',
            'coordOutputFormat': 'EPSG:4326',
            'depArrMacro': 'dep',
            'type_origin': 'any',
            'name_origin': origin,
            'type_destination': 'any',
            'name_destination': destination,
            'calcNumberOfTrips': '1',
            'excludedMeans': 'checkbox',
            'exclMOT_5': '1',
            'TfNSWTR': 'true',
            'version': '10.2.1.42',
            'itOptionsActive': '1',
            'cycleSpeed': '16'
        }
        
        headers = {
            'Authorization': f'apikey {api_key}',
            'Accept': 'application/json'
        }
        
        response = requests.get(url, params=params, headers=headers, timeout=10)
        response.raise_for_status()
        
        data = response.json()
        
        if 'journeys' in data and len(data['journeys']) > 0:
            journey = data['journeys'][0]
            if 'legs' in journey and len(journey['legs']) > 0:
                leg = journey['legs'][0]
                
                departure_time_utc = leg['origin'].get('departureTimeEstimated', '')
                via_destination = leg['transportation']['destination'].get('name', '')
                
                logger.info(f"Train data fetched successfully - Next departure at {departure_time_utc}")
                return {
                    'departure_time': departure_time_utc,
                    'via': via_destination
                }
        
        logger.warning("No train data available")
        return {'error': 'No train data available'}
    except requests.exceptions.HTTPError as e:
        status_code = e.response.status_code if e.response else 'unknown'
        logger.error(f"HTTP error fetching train data: {status_code} - {e}")
        return {'error': f'HTTP {status_code}: {str(e)}'}
    except requests.exceptions.RequestException as e:
        logger.error(f"Network error fetching train data: {e}")
        return {'error': f'Network error: {str(e)}'}
    except Exception as e:
        logger.error(f"Error fetching train data: {e}")
        return {'error': str(e)}

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.utcnow().isoformat()
    })

@app.route('/api/data', methods=['POST', 'GET'])
def get_aggregated_data():
    """
    Main endpoint that aggregates all data sources.
    
    This endpoint is read-only and has no side effects, so GET is the
    semantically correct and preferred HTTP method.
    
    POST is also accepted for backward compatibility with existing
    ESP32 firmware, which issues POST requests with an empty body
    when fetching the aggregated data.
    """
    try:
        # Log incoming request
        client_ip = request.remote_addr
        method = request.method
        logger.info(f"Received {method} request from {client_ip} for aggregated data")
        
        # Fetch all data in parallel for better performance
        with ThreadPoolExecutor(max_workers=4) as executor:
            weather_future = executor.submit(fetch_weather_data)
            crypto_future = executor.submit(fetch_crypto_data)
            stock_future = executor.submit(fetch_stock_data)
            train_future = executor.submit(fetch_train_data)
            
            # Wait for all results
            weather = weather_future.result()
            crypto = crypto_future.result()
            stock = stock_future.result()
            train = train_future.result()
        
        response = {
            'timestamp': datetime.utcnow().isoformat(),
            'weather': weather,
            'crypto': crypto,
            'stock': stock,
            'train': train
        }
        
        logger.info(f"Successfully aggregated data for {client_ip}")
        return jsonify(response)
    except Exception as e:
        logger.error(f"Error aggregating data: {e}")
        return jsonify({
            'error': str(e),
            'timestamp': datetime.utcnow().isoformat()
        }), 500

@app.route('/api/weather', methods=['GET'])
def get_weather():
    """Endpoint for weather data only"""
    logger.info(f"Received request from {request.remote_addr} for weather data")
    return jsonify(fetch_weather_data())

@app.route('/api/crypto', methods=['GET'])
def get_crypto():
    """Endpoint for crypto data only"""
    logger.info(f"Received request from {request.remote_addr} for crypto data")
    return jsonify(fetch_crypto_data())

@app.route('/api/stock', methods=['GET'])
def get_stock():
    """Endpoint for stock data only"""
    logger.info(f"Received request from {request.remote_addr} for stock data")
    return jsonify(fetch_stock_data())

@app.route('/api/train', methods=['GET'])
def get_train():
    """Endpoint for train data only"""
    logger.info(f"Received request from {request.remote_addr} for train data")
    return jsonify(fetch_train_data())

if __name__ == '__main__':
    # Run the Flask app
    port = int(os.getenv('PORT', 5000))
    host = os.getenv('HOST', '0.0.0.0')
    debug = os.getenv('DEBUG', 'False').lower() == 'true'
    
    logger.info(f"Starting middleware server on {host}:{port}")
    app.run(host=host, port=port, debug=debug)
