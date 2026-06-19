# Rype Plant Monitoring System

## 1. Project Summary

Rype reads soil moisture from the sensor input, converting and filtering the data to show watering needs, moisture %, and raw ADC. Our project also shows the status in terminal and through optional outputs like our personalized Discord server and LED lights. 

## 2. Requirements

Required:
- CMake >= 3.16
- C++17 compiler
- SQLite3 dev libraries
- libcurl dev libraries
- pthreads (Linux/macOS toolchain default)

For mock/demo mode:
- Python 3
- `socat`

## 3. Build Instructions

From repository root:

```bash
mkdir -p build
cd build
cmake ..
make -j
```

Expected result:
- `build/plant_monitor` is produced without errors.

## 4. Run Instructions (Real Hardware)

1. Connect Arduino/sensor and confirm serial device path.
2. Set serial config in `include/config.hpp` (for example `/dev/ttyACM0`).
3. Rebuild if config changed.
4. Run:

```bash
cd build
./plant_monitor
```

## 5. Run Instructions (Mock / TA Friendly)

Terminal A (virtual serial pair):

```bash
socat -d -d pty,raw,echo=0,link=/tmp/plant_serial_write pty,raw,echo=0,link=/tmp/plant_serial_read
```

Terminal B (mock Arduino data):

```bash
python3 mock_arduino.py /tmp/plant_serial_write
```

Terminal C (app):

1. Set `SERIAL_PORT` in `include/config.hpp` to `/tmp/plant_serial_read`
2. Rebuild
3. Run:

```bash
cd build
./plant_monitor
```

## 6. Configuration Flags

Primary runtime/build toggles are in `include/feature_flags.hpp`.

Common flags:
- `FEATURE_LED_ALERTS`: toggle for LED light
- `FEATURE_DISCORD_NOTIFICATIONS`: toggle for Discord notifications

Core sensor/calibration values are in `include/config.hpp`.

For the terminal Vision check:
- `VISION_CAMERA_INDEX` selects which camera to use.
- On most Mac laptops, the built-in camera is `0`.
- Press `[V]` in the terminal UI to run a camera snapshot health check and
    show the result as `HEALTHY` or `UNHEALTHY`.

## 7. Known Environment Notes

- Raspberry Pi LED control is expected to use Linux GPIO support.
- If LED is wired to Arduino pin 7 (not Pi header), LED behavior must be handled by Arduino sketch logic outlined below.
```
const int SENSOR_PIN = A0;
const int LED_PIN = 7;

const int DRY_VALUE = 590;
const int WET_VALUE = 273;
const float MOISTURE_THRESHOLD = 66.2f; // "needs water" threshold

float rawToPct(int raw) {
    float pct = (float)(DRY_VALUE - raw) * 100.0f / (float)(DRY_VALUE - WET_VALUE);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);  // LED off at startup until first reading
}

void loop() {
    int raw = analogRead(SENSOR_PIN);
    float moisture = rawToPct(raw);
    bool needsWater = (moisture < MOISTURE_THRESHOLD);

    // LED on if plant needs water, off if OK
    digitalWrite(LED_PIN, needsWater ? HIGH : LOW);

    Serial.print("RAW:");
    Serial.println(raw);
    delay(1000);
}
```
- If LED is wired to Raspberry Pi physical pin 7, that maps to BCM GPIO 4.

