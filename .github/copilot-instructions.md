# AI Coding Agent Instructions for LILYGO T5 Project Template

## Project Overview

This is a project template for the **LILYGO T5 4.7" e-paper display** on **Raspberry Pi 3**.

### Key Facts
- **Hardware**: LILYGO T5 4.7" with ESP32-S3 and PSRAM
- **Connection**: USB to Raspberry Pi at `/dev/ttyACM0`, 115200 baud serial
- **Build System**: PlatformIO (Arduino framework)
- **Display**: 540×960 pixels, 1-bit (black & white only)
- **Refresh Rate**: ~1-3 seconds for full update (slow, don't animate)

## Development Setup

### Virtual Environment (REQUIRED)
Always use a Python virtual environment - never system-wide pip:

```bash
# Create
python3 -m venv venv

# Activate (before any Python work)
source venv/bin/activate

# Install from requirements.txt
pip install -r requirements.txt
```

### Build & Upload Firmware
```bash
# Build and upload to device (one command)
pio run -t upload

# Monitor serial output
pio device monitor -b 115200
```

### Temporary Files
All temporary, test, and debug files go in `temp/` directory:
- Test scripts
- Log files
- Binary buffers
- Serialized data

## Code Structure

```
project/
├── src/
│   └── main.ino              # Arduino firmware (edit this!)
├── python/
│   ├── main.py               # Python entry point
│   ├── display_driver.py      # Serial communication module
│   └── config.py              # Configuration constants
├── platformio.ini            # Build configuration
└── requirements.txt          # Python dependencies
```

## Common Tasks

### Modify Firmware
1. Edit `src/main.ino`
2. Run: `pio run -t upload`
3. View output: `pio device monitor -b 115200`

### Display Text (Firmware)
```cpp
epd_poweron();
epd_clear();  // White background

int cursor_x = 10;
int cursor_y = 10;
writeln((GFXfont *)&FiraSans, "Hello World!", &cursor_x, &cursor_y, NULL);

epd_poweroff_all();  // Save power (image persists)
```

### Display Image (Python)
```python
from PIL import Image
from python.display_driver import DisplayDriver

# Create image
img = Image.new('1', (540, 960), color=255)  # B&W, 1-bit
# ... draw on image ...

# Send to display
display = DisplayDriver()
display.display_image(img)
```

## Key Libraries & APIs

### Arduino (Firmware)
- **epd_driver.h**: Initialize and control display
- **firasans.h**: Built-in font
- **Button2.h**: Button handling (if using touch)

### Python
- **PySerial**: USB serial communication
- **Pillow (PIL)**: Image creation and manipulation
- **requests**: HTTP for APIs

## Important Constraints

1. **E-Paper is Slow**: Don't refresh more than 1-2× per minute
2. **Display is 1-bit B&W**: No colors, grayscale, or anti-aliasing
3. **Baud Rate 115200**: Use consistently for serial communication
4. **Power Management**: Use `epd_poweroff_all()` to save battery

## File Organization Rules

- **Source Code**: `src/` (firmware) and `python/` (Python)
- **Temporary Files**: `temp/` directory (logs, test data, buffers)
- **Configuration**: `config.py` and `platformio.ini`
- **Dependencies**: `requirements.txt` and `platformio.ini`

## Debugging Workflow

### Device Not Found
```bash
ls /dev/ttyACM0           # Check port exists
python3 diagnose.py       # From parent lilygo-t5 directory
dmesg | tail -10          # Check kernel logs
```

### Upload Fails
```bash
pio run -t erase          # Erase flash first
pio run -t upload         # Try again
```

### Display Blank
- Check `pio device monitor -b 115200` for error messages
- Verify `epd_init()` is called in setup()
- Verify PSRAM is enabled in platformio.ini

### Python Errors
- Activate virtualenv: `source venv/bin/activate`
- Check import paths: `import sys; print(sys.path)`
- Read serial output for device-side errors

## Project Template References

- **Parent Template**: `/home/chan/Repo/lilygo-t5-template/`
- **Working Example**: `/home/chan/Repo/lilygo-t5/examples/hello_world/`
- **Library**: `/home/chan/Repo/lilygo-t5/lib/LilyGo/` (symlink to LilyGo-EPD47/src)

## Common Pitfalls

| Issue | Cause | Fix |
|-------|-------|-----|
| `No module named 'serial'` | Not in virtualenv | Run: `source venv/bin/activate` |
| `Device not found` | Wrong port or disconnected | Check `/dev/ttyACM*` exists |
| `Upload fails with timeout` | Firmware upload in progress | Wait and retry |
| `Display stays blank` | epd_init() not called | Add to setup() |
| `Image doesn't appear` | Wrong format/size | Use 1-bit, 540×960 |
| `Can't import epd_driver` | PlatformIO lib path wrong | Check lib_extra_dirs in platformio.ini |

## Next Steps for New Projects

1. Copy this template to a new directory
2. Edit `src/main.ino` with your firmware logic
3. Edit `python/main.py` with your application logic
4. Run: `pio run -t upload`
5. Test: `pio device monitor -b 115200`
6. Customize: Iterate until satisfied
7. Commit: Push to your version control system

---

**Template Created From**: LILYGO T5 working example on Raspberry Pi 3  
**Last Updated**: 2025-12-26
