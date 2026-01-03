"""
LILYGO T5 Weather Display - Middleware API
Aggregates data from multiple upstream APIs to optimize battery life on display board
"""

from flask import Flask, request, jsonify
import requests
import os
import json
from datetime import datetime

app = Flask(__name__)

# Load configuration from environment variables or config file
def load_config():
    """Load API keys and configuration"""
    config_file = os.getenv('CONFIG_FILE', 'config.json')
    
    if os.path.exists(config_file):
        with open(config_file, 'r') as f:
            return json.load(f)
    
    # Fallback to environment variables
    return {
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

config = load_config()

def fetch_weather_data():
    """Fetch weather data from OpenWeatherMap API"""
    try:
        weather_config = config.get('weather', {})
        api_key = weather_config.get('api_key')
        city = weather_config.get('city', 'Sydney')
        country = weather_config.get('country', 'AU')
        units = weather_config.get('units', 'metric')
        
        if not api_key:
            return {'error': 'Weather API key not configured'}
        
        url = f"https://api.openweathermap.org/data/2.5/weather?q={city},{country}&appid={api_key}&units={units}"
        response = requests.get(url, timeout=10)
        response.raise_for_status()
        
        data = response.json()
        
        return {
            'temp': round(data['main']['temp'], 1),
            'feels_like': round(data['main']['feels_like'], 1),
            'temp_max': round(data['main']['temp_max'], 1),
            'temp_min': round(data['main']['temp_min'], 1),
            'humidity': data['main']['humidity'],
            'wind_speed': round(data['wind']['speed'], 1),
            'condition': data['weather'][0]['main'],
            'icon_code': data['weather'][0]['icon'],
            'city': city,
            'units': units
        }
    except Exception as e:
        print(f"Error fetching weather data: {e}")
        return {'error': str(e)}

def fetch_crypto_data():
    """Fetch cryptocurrency data from CoinGecko API"""
    try:
        crypto_config = config.get('crypto', {})
        api_key = crypto_config.get('api_key')
        symbol = crypto_config.get('symbol', 'btc').lower()
        
        if not api_key:
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
        
        url = f"https://api.coingecko.com/api/v3/simple/price?ids={coin_id}&vs_currencies=usd&include_24hr_change=true"
        headers = {'x-cg-demo-api-key': api_key}
        response = requests.get(url, headers=headers, timeout=10)
        response.raise_for_status()
        
        data = response.json()
        
        if coin_id in data:
            return {
                'symbol': symbol.upper(),
                'price': int(data[coin_id].get('usd', 0)),
                'change_24h': round(data[coin_id].get('usd_24h_change', 0), 2)
            }
        else:
            return {'error': f'Crypto symbol/ID not found: {symbol}'}
    except Exception as e:
        print(f"Error fetching crypto data: {e}")
        return {'error': str(e)}

def fetch_stock_data():
    """Fetch stock market data from MarketStack API"""
    try:
        stock_config = config.get('stock', {})
        api_key = stock_config.get('api_key')
        symbol = stock_config.get('symbol', 'AAPL').upper()
        
        if not api_key:
            return {'error': 'Stock API key not configured'}
        
        url = f"http://api.marketstack.com/v1/eod/latest?access_key={api_key}&symbols={symbol}"
        response = requests.get(url, timeout=10)
        response.raise_for_status()
        
        data = response.json()
        
        if 'data' in data and len(data['data']) > 0:
            stock_data = data['data'][0]
            open_price = stock_data.get('open', 0)
            close_price = stock_data.get('close', 0)
            change = close_price - open_price
            
            return {
                'symbol': symbol,
                'price': round(close_price, 2),
                'currency': stock_data.get('exchange', 'USD'),
                'change': round(change, 2)
            }
        else:
            return {'error': 'Stock symbol not found'}
    except Exception as e:
        print(f"Error fetching stock data: {e}")
        return {'error': str(e)}

def fetch_train_data():
    """Fetch train schedule data from Transport NSW API"""
    try:
        train_config = config.get('train', {})
        api_key = train_config.get('api_key')
        origin = train_config.get('origin')
        destination = train_config.get('destination')
        
        if not api_key or not origin or not destination:
            return {'error': 'Train API not fully configured'}
        
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
                
                return {
                    'departure_time': departure_time_utc,
                    'via': via_destination
                }
        
        return {'error': 'No train data available'}
    except Exception as e:
        print(f"Error fetching train data: {e}")
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
    Main endpoint that aggregates all data sources
    Accepts POST or GET requests
    """
    try:
        # Fetch all data in parallel (could be optimized with threading)
        weather = fetch_weather_data()
        crypto = fetch_crypto_data()
        stock = fetch_stock_data()
        train = fetch_train_data()
        
        response = {
            'timestamp': datetime.utcnow().isoformat(),
            'weather': weather,
            'crypto': crypto,
            'stock': stock,
            'train': train
        }
        
        return jsonify(response)
    except Exception as e:
        return jsonify({
            'error': str(e),
            'timestamp': datetime.utcnow().isoformat()
        }), 500

@app.route('/api/weather', methods=['GET'])
def get_weather():
    """Endpoint for weather data only"""
    return jsonify(fetch_weather_data())

@app.route('/api/crypto', methods=['GET'])
def get_crypto():
    """Endpoint for crypto data only"""
    return jsonify(fetch_crypto_data())

@app.route('/api/stock', methods=['GET'])
def get_stock():
    """Endpoint for stock data only"""
    return jsonify(fetch_stock_data())

@app.route('/api/train', methods=['GET'])
def get_train():
    """Endpoint for train data only"""
    return jsonify(fetch_train_data())

if __name__ == '__main__':
    # Run the Flask app
    port = int(os.getenv('PORT', 5000))
    host = os.getenv('HOST', '0.0.0.0')
    debug = os.getenv('DEBUG', 'False').lower() == 'true'
    
    print(f"Starting middleware server on {host}:{port}")
    app.run(host=host, port=port, debug=debug)
