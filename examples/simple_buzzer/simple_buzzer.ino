/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @Hardwares: M5Stack PortA device with buzzer + Unit Step16
 * @Dependent Library:
 * M5Unit-Step16:https://github.com/m5stack/M5Unit-Step16
 * @description: When the knob is rotated, play different tones of sound,
 *               play different frequencies of sound clockwise and counterclockwise
 */

#include <Wire.h>
#include <M5UnitStep16.h>

// Buzzer pin definition
#define BUZZER_PIN (25)

#define NUM_TONES             (16)
#define LED_BRIGHTNESS        (60)
#define RGB_BRIGHTNESS        (40)
#define TONE_DURATION         (150)
#define STARTUP_TONE_DURATION (200)
#define STARTUP_TONE_DELAY    (250)
#define ERROR_TONE_DURATION   (300)
#define ERROR_TONE_DELAY      (400)
#define ERROR_TONE_FREQUENCY  (200)
#define PITCH_RATIO           (0.8)

// UnitStep16 object Parameters: (I2C_address, I2C_wire_pointer)
// - I2C_address: default is 0x48, you can change it if needed
// - I2C_wire_pointer: default is &Wire, you can use &Wire1, &Wire2, etc. for different I2C buses
UnitStep16 step16(0x48, &Wire);

// Variables
uint8_t lastValue = 0;
bool isFirstRead  = true;

// Tone frequency definition (Hz)
const int toneFreq[NUM_TONES] = {
    262,   // C4  - 0
    294,   // D4  - 1
    330,   // E4  - 2
    349,   // F4  - 3
    392,   // G4  - 4
    440,   // A4  - 5
    494,   // B4  - 6
    523,   // C5  - 7
    587,   // D5  - 8
    659,   // E5  - 9
    698,   // F5  - A
    784,   // G5  - B
    880,   // A5  - C
    988,   // B5  - D
    1047,  // C6  - E
    1175   // D6  - F
};

void setup()
{
    // Initialize serial port
    Serial.begin(115200);

    Serial.println("\nUnitStep16 Rotation Sound Example");
    Serial.println("==================================");

    // Initialize buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);

    // Initialize I2C
    Wire.begin();

    // Initialize UnitStep16
    if (!step16.begin()) {
        Serial.println("UnitStep16 initialization failed!");
        Serial.println("Please check the connection");
        // Play error tone
        playErrorTone();
        while (1) {
            delay(1000);
        }
    }

    Serial.println("UnitStep16 initialized successfully!");

    // Set initial configuration
    step16.setLedConfig(0xFF);                // LED always on
    step16.setLedBrightness(LED_BRIGHTNESS);  // LED brightness 60%
    step16.setRgbConfig(1);                   // Enable RGB
    step16.setRgbBrightness(RGB_BRIGHTNESS);  // RGB brightness 40%
    step16.setSwitchState(0);                 // Clockwise direction

    Serial.println("Instructions:");
    Serial.println("- Rotating the knob will play corresponding tones");
    Serial.println("- Different positions play different frequency sounds");
    Serial.println("- Clockwise and counterclockwise have different timbres");
    Serial.println();

    // Play startup sound
    playStartupTone();

    // Read initial value
    lastValue = step16.getValue();
}

void loop()
{
    // Read current knob value
    uint8_t currentValue = step16.getValue();

    // Detect if value has changed
    if (currentValue != lastValue && !isFirstRead) {
        // Determine rotation direction
        bool isClockwise = isRotationClockwise(lastValue, currentValue);

        // Play corresponding tone
        playRotationTone(currentValue, isClockwise);

        // Set RGB color (based on value change)
        setRgbByValue(currentValue);

        // Print information
        Serial.print("Value change: ");
        Serial.print(lastValue);
        Serial.print(" -> ");
        Serial.print(currentValue);
        Serial.print(" (");
        Serial.print(isClockwise ? "Clockwise" : "Counterclockwise");
        Serial.print(") Frequency: ");
        Serial.print(toneFreq[currentValue]);
        Serial.println("Hz");

        lastValue = currentValue;
    }

    if (isFirstRead) {
        isFirstRead = false;
    }

    delay(50);
}

/**
 * @brief Determine rotation direction
 * @param oldVal Old value
 * @param newVal New value
 * @return true for clockwise, false for counterclockwise
 */
bool isRotationClockwise(uint8_t oldVal, uint8_t newVal)
{
    // Handle boundary cases (0->15 or 15->0)
    if (oldVal == 0 && newVal == 15) {
        return false;  // Counterclockwise
    } else if (oldVal == 15 && newVal == 0) {
        return true;  // Clockwise
    } else {
        return newVal > oldVal;  // Normal case
    }
}

/**
 * @brief Play rotation tone
 * @param value Current value
 * @param clockwise Is clockwise
 */
void playRotationTone(uint8_t value, bool clockwise)
{
    int frequency = toneFreq[value];
    int duration  = TONE_DURATION;

    if (clockwise) {
        // Clockwise: play standard tone
        tone(BUZZER_PIN, frequency, duration);
    } else {
        // Counterclockwise: play slightly lower tone
        tone(BUZZER_PIN, frequency * PITCH_RATIO, duration);
    }
}

/**
 * @brief Set RGB color based on value
 * @param value Knob value
 */
void setRgbByValue(uint8_t value)
{
    // Map 0-15 to color wheel (0-360 degrees)
    float hue = (value * 360.0) / NUM_TONES;

    // HSV to RGB
    float r, g, b;
    hsvToRgb(hue, 1.0, 1.0, &r, &g, &b);

    step16.setRgb((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

/**
 * @brief HSV to RGB color space conversion
 * @param h Hue (0-360)
 * @param s Saturation (0-1)
 * @param v Value (0-1)
 * @param r Red output
 * @param g Green output
 * @param b Blue output
 */
void hsvToRgb(float h, float s, float v, float* r, float* g, float* b)
{
    int i   = (int)(h / 60.0) % 6;
    float f = (h / 60.0) - i;
    float p = v * (1.0 - s);
    float q = v * (1.0 - s * f);
    float t = v * (1.0 - s * (1.0 - f));

    switch (i) {
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;
        case 5:
            *r = v;
            *g = p;
            *b = q;
            break;
    }
}

/**
 * @brief Play startup tone
 */
void playStartupTone()
{
    // Play ascending scale
    int startupNotes[] = {262, 330, 392, 523};  // C-E-G-C
    for (int i = 0; i < 4; i++) {
        tone(BUZZER_PIN, startupNotes[i], STARTUP_TONE_DURATION);
        delay(STARTUP_TONE_DELAY);
    }
    delay(500);
}

/**
 * @brief Play error tone
 */
void playErrorTone()
{
    // Play three low tones
    for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, ERROR_TONE_FREQUENCY, ERROR_TONE_DURATION);
        delay(ERROR_TONE_DELAY);
    }
}