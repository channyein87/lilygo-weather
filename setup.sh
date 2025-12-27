#!/bin/bash
# Setup script for new LILYGO T5 projects
# 
# Usage: 
#   bash setup.sh
#
# This script:
#   1. Creates virtual environment
#   2. Installs Python dependencies
#   3. Verifies PlatformIO installation
#   4. Checks device connection
#   5. Ready to build!

set -e  # Exit on error

echo "================================"
echo "LILYGO T5 Project Setup"
echo "================================"
echo ""

# Color codes
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'  # No Color

# Step 1: Check Python
echo -e "${BLUE}[1/5]${NC} Checking Python installation..."
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 not found!"
    exit 1
fi
PYTHON_VERSION=$(python3 --version 2>&1)
echo -e "${GREEN}✓${NC} Found: $PYTHON_VERSION"
echo ""

# Step 2: Create virtual environment
echo -e "${BLUE}[2/5]${NC} Creating virtual environment..."
if [ -d "venv" ]; then
    echo "⚠️  Virtual environment already exists, skipping..."
else
    python3 -m venv venv
    echo -e "${GREEN}✓${NC} Virtual environment created"
fi
echo ""

# Step 3: Activate and install dependencies
echo -e "${BLUE}[3/5]${NC} Installing Python dependencies..."
source venv/bin/activate
pip install --quiet --upgrade pip setuptools wheel
pip install --quiet -r requirements.txt
echo -e "${GREEN}✓${NC} Dependencies installed:"
pip list | grep -E "pillow|pyserial|requests" | sed 's/^/    /'
echo ""

# Step 4: Check PlatformIO
echo -e "${BLUE}[4/5]${NC} Checking PlatformIO..."
if ! command -v pio &> /dev/null; then
    echo "⚠️  PlatformIO not found in PATH"
    echo "   Install with: pip install platformio"
else
    PIO_VERSION=$(pio --version 2>&1 | head -1)
    echo -e "${GREEN}✓${NC} $PIO_VERSION"
fi
echo ""

# Step 5: Check device connection
echo -e "${BLUE}[5/5]${NC} Checking device connection..."
if [ -e /dev/ttyACM0 ]; then
    echo -e "${GREEN}✓${NC} Device found at /dev/ttyACM0"
elif [ -e /dev/ttyACM1 ]; then
    echo -e "${YELLOW}⚠${NC}  Device found at /dev/ttyACM1 (not /dev/ttyACM0)"
    echo "   Update platformio.ini upload_port accordingly"
else
    echo -e "${YELLOW}⚠${NC}  No device detected"
    echo "   Make sure LILYGO T5 is connected via USB"
fi
echo ""

echo "================================"
echo -e "${GREEN}✅ Setup Complete!${NC}"
echo "================================"
echo ""
echo "Next steps:"
echo "  1. Activate virtualenv: source venv/bin/activate"
echo "  2. Edit src/main.ino with your code"
echo "  3. Build & upload: pio run -t upload"
echo "  4. Monitor output: pio device monitor -b 115200"
echo ""
echo "For more info, see README.md"
