/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
 * @Hardwares: M5Stack PortA device + Unit Step16
 * @Dependent Library:
 * M5Unit-Step16:https://github.com/m5stack/M5Unit-Step16
 * @description: When the knob is rotated, only 0-8 is displayed,
 *              when raw value reaches 9-F, rotation direction is automatically reversed,
 *              LED brightness follows display value, never zero (minimum 10%),
 *              RGB color follows the 0-8 pattern
 */

#include <Wire.h>
#include <M5UnitStep16.h>

#define NUM_DIGITS      (9)
#define MAX_DIGIT       (8)
#define MIN_DIGIT       (0)
#define MIN_BRIGHTNESS  (10)
#define MAX_BRIGHTNESS  (100)
#define BRIGHTNESS_STEP (10)
#define RGB_BRIGHTNESS  (70)
#define LED_ALWAYS_ON   (0xFF)

// Create UnitStep16 object
UnitStep16 step16;

// Variables
uint8_t currentDigit = 0;
uint8_t lastRawValue = 0;
bool isFirstRead     = true;

// LED brightness for each digit (10-100, never zero)
const uint8_t digitBrightness[NUM_DIGITS] = {
    MIN_BRIGHTNESS,                        // 0 - 10%
    MIN_BRIGHTNESS + BRIGHTNESS_STEP,      // 1 - 20%
    MIN_BRIGHTNESS + BRIGHTNESS_STEP * 2,  // 2 - 30%
    MIN_BRIGHTNESS + BRIGHTNESS_STEP * 3,  // 3 - 40%
    MIN_BRIGHTNESS + BRIGHTNESS_STEP * 4,  // 4 - 50%
    MIN_BRIGHTNESS + BRIGHTNESS_STEP * 5,  // 5 - 60%
    MIN_BRIGHTNESS + BRIGHTNESS_STEP * 6,  // 6 - 70%
    MIN_BRIGHTNESS + BRIGHTNESS_STEP * 7,  // 7 - 80%
    MIN_BRIGHTNESS + BRIGHTNESS_STEP * 8   // 8 - 90%
};

// RGB colors for each digit (0-8, from cold to warm)
struct RGBColor {
    uint8_t r, g, b;
    const char* name;
};

const RGBColor digitColors[NUM_DIGITS] = {
    {0, 0, 128, "Deep Blue"},       // 0 - cold color
    {0, 64, 192, "Blue"},           // 1
    {0, 128, 255, "Light Blue"},    // 2
    {0, 192, 192, "Cyan"},          // 3
    {0, 255, 128, "Cyan Green"},    // 4
    {128, 255, 0, "Yellow Green"},  // 5
    {192, 192, 0, "Yellow"},        // 6
    {255, 128, 0, "Orange"},        // 7
    {255, 64, 0, "Orange Red"}      // 8 - warm color
};

void setup()
{
    // Initialize I2C
    Wire.begin();

    // Initialize UnitStep16
    if (!step16.begin()) {
        while (1) {
            delay(1000);
        }
    }

    // Set initial configuration
    step16.setRgbConfig(1);                   // Enable RGB
    step16.setRgbBrightness(RGB_BRIGHTNESS);  // RGB brightness 70%

    // Read initial value
    lastRawValue = step16.getValue();
    currentDigit = mapRawValueToDigit(lastRawValue);
    updateDisplay(currentDigit);
}

void loop()
{
    // Read current raw value
    uint8_t rawValue = step16.getValue();

    // Detect if raw value has changed
    if (rawValue != lastRawValue && !isFirstRead) {
        handleValueChange(lastRawValue, rawValue);
        lastRawValue = rawValue;
    }

    if (isFirstRead) {
        isFirstRead = false;
    }

    delay(50);
}

/**
 * @brief Handle value change
 * @param oldVal Old raw value
 * @param newVal New raw value
 */
void handleValueChange(uint8_t oldVal, uint8_t newVal)
{
    // Check if new value is in 9-F range (need to reverse direction)
    if (newVal >= 9 && newVal <= 15) {
        // Reverse rotation direction
        uint8_t currentDirection = step16.getSwitchState();
        step16.setSwitchState(currentDirection == 0 ? 1 : 0);
    }

    // Always map raw value to 0-8 for display
    uint8_t newDigit = mapRawValueToDigit(newVal);

    // Update display if digit changed
    if (newDigit != currentDigit) {
        currentDigit = newDigit;
        updateDisplay(currentDigit);
    }
}

/**
 * @brief Map raw value to digit 0-8
 * @param rawValue Raw value (0-15)
 * @return Mapped digit (0-8)
 */
uint8_t mapRawValueToDigit(uint8_t rawValue)
{
    // Map 0-F to 0-8 cycling pattern
    // 0-8: direct mapping
    // 9-F: reverse mapping 8-0
    if (rawValue <= 8) {
        return rawValue;  // 0->0, 1->1, ..., 8->8
    } else {
        // 9->8, A->7, B->6, C->5, D->4, E->3, F->2
        return 17 - rawValue;  // This creates the reverse cycle
    }
}

/**
 * @brief Update display
 * @param digit Digit to display (0-8)
 */
void updateDisplay(uint8_t digit)
{
    if (digit > MAX_DIGIT) return;

    // Set LED brightness (never zero)
    step16.setLedBrightness(digitBrightness[digit]);

    // Set RGB color
    step16.setRgb(digitColors[digit].r, digitColors[digit].g, digitColors[digit].b);

    // LED always on (never turn off)
    step16.setLedConfig(LED_ALWAYS_ON);
}
