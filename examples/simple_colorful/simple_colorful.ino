/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @Hardwares: M5Stack PortA device + Unit Step16
 * @Dependent Library:
 * M5Unit-Step16:https://github.com/m5stack/M5Unit-Step16
 * @description: When the knob is rotated, display the corresponding color,
 *              16 positions correspond to 16 different colors,
 *              including smooth color transition effect
 */

#include <Wire.h>
#include <M5UnitStep16.h>

#define NUM_COLORS       (16)
#define LED_BRIGHTNESS   (80)
#define RGB_BRIGHTNESS   (60)
#define TRANSITION_SPEED (8)
#define TRANSITION_DELAY (20)
#define DEMO_DELAY       (200)
#define RAINBOW_STEP     (5)
#define RAINBOW_DELAY    (20)

// UnitStep16 object Parameters: (I2C_address, I2C_wire_pointer)
// - I2C_address: default is 0x48, you can change it if needed
// - I2C_wire_pointer: default is &Wire, you can use &Wire1, &Wire2, etc. for different I2C buses
UnitStep16 step16(0x48, &Wire);

// Variables
uint8_t lastValue = 0;
uint8_t targetR = 0, targetG = 0, targetB = 0;
uint8_t currentR = 0, currentG = 0, currentB = 0;
bool smoothTransition    = true;
unsigned long lastUpdate = 0;

// Predefined 16 colors (RGB)
struct Color {
    uint8_t r, g, b;
    const char* name;
};

const Color colorPalette[NUM_COLORS] = {
    {255, 0, 0, "Red"},               // 0
    {255, 128, 0, "Orange"},          // 1
    {255, 255, 0, "Yellow"},          // 2
    {128, 255, 0, "Yellow Green"},    // 3
    {0, 255, 0, "Green"},             // 4
    {0, 255, 128, "Cyan Green"},      // 5
    {0, 255, 255, "Cyan"},            // 6
    {0, 128, 255, "Sky Blue"},        // 7
    {0, 0, 255, "Blue"},              // 8
    {128, 0, 255, "Purple Blue"},     // 9
    {255, 0, 255, "Purple"},          // A
    {255, 0, 128, "Pink"},            // B
    {255, 128, 128, "Light Red"},     // C
    {255, 255, 128, "Light Yellow"},  // D
    {128, 255, 255, "Light Cyan"},    // E
    {255, 255, 255, "White"}          // F
};

void setup()
{
    // Initialize serial port
    Serial.begin(115200);
    Serial.println("\nUnitStep16 Rotary Colorful Display Example");
    Serial.println("==========================================");

    // Initialize I2C
    Wire.begin();

    // Initialize UnitStep16
    if (!step16.begin()) {
        Serial.println("UnitStep16 initialization failed!");
        Serial.println("Please check the connection");
        while (1) {
            delay(1000);
        }
    }

    Serial.println("UnitStep16 initialized successfully!");

    // Set initial configuration
    step16.setLedConfig(0xFF);                // LED always on
    step16.setLedBrightness(LED_BRIGHTNESS);  // LED brightness 80%
    step16.setRgbConfig(1);                   // Enable RGB
    step16.setRgbBrightness(RGB_BRIGHTNESS);  // RGB brightness 60%
    step16.setSwitchState(0);                 // Clockwise direction

    Serial.println("Features:");
    Serial.println("- 16 positions correspond to 16 different colors");
    Serial.println("- Support smooth color transition effect");
    Serial.println("- Rotate to view all colors");
    Serial.println();

    // Show color palette
    showColorPalette();

    // Demo color cycle
    demoColorCycle();

    // Read initial value and set color
    lastValue = step16.getValue();
    setTargetColor(lastValue);
    setCurrentColor();

    Serial.println("Start monitoring knob rotation...\n");
}

void loop()
{
    // Read current knob value
    uint8_t currentValue = step16.getValue();

    // Detect if value has changed
    if (currentValue != lastValue) {
        Serial.print("Position change: ");
        Serial.print(lastValue, HEX);
        Serial.print(" -> ");
        Serial.print(currentValue, HEX);
        Serial.print(" | Color: ");
        Serial.print(colorPalette[currentValue].name);
        Serial.print(" (RGB: ");
        Serial.print(colorPalette[currentValue].r);
        Serial.print(", ");
        Serial.print(colorPalette[currentValue].g);
        Serial.print(", ");
        Serial.print(colorPalette[currentValue].b);
        Serial.println(")");

        // Set target color
        setTargetColor(currentValue);

        lastValue = currentValue;
    }

    // Smooth color transition
    if (smoothTransition && millis() - lastUpdate > TRANSITION_DELAY) {
        updateColorTransition();
        lastUpdate = millis();
    }

    delay(10);
}

/**
 * @brief Set target color
 * @param value Knob value (0-15)
 */
void setTargetColor(uint8_t value)
{
    if (value < NUM_COLORS) {
        targetR = colorPalette[value].r;
        targetG = colorPalette[value].g;
        targetB = colorPalette[value].b;
    }
}

/**
 * @brief Set current color directly (no transition)
 */
void setCurrentColor()
{
    currentR = targetR;
    currentG = targetG;
    currentB = targetB;
    step16.setRgb(currentR, currentG, currentB);
}

/**
 * @brief Update color transition animation
 */
void updateColorTransition()
{
    bool needUpdate = false;
    uint8_t speed   = TRANSITION_SPEED;  // Transition speed

    // Smooth transition to target red value
    if (currentR != targetR) {
        if (currentR < targetR) {
            currentR = (currentR + speed < targetR) ? currentR + speed : targetR;
        } else {
            currentR = (currentR - speed > targetR) ? currentR - speed : targetR;
        }
        needUpdate = true;
    }

    // Smooth transition to target green value
    if (currentG != targetG) {
        if (currentG < targetG) {
            currentG = (currentG + speed < targetG) ? currentG + speed : targetG;
        } else {
            currentG = (currentG - speed > targetG) ? currentG - speed : targetG;
        }
        needUpdate = true;
    }

    // Smooth transition to target blue value
    if (currentB != targetB) {
        if (currentB < targetB) {
            currentB = (currentB + speed < targetB) ? currentB + speed : targetB;
        } else {
            currentB = (currentB - speed > targetB) ? currentB - speed : targetB;
        }
        needUpdate = true;
    }

    // If any color value changed, update RGB
    if (needUpdate) {
        step16.setRgb(currentR, currentG, currentB);
    }
}

/**
 * @brief Show color palette
 */
void showColorPalette()
{
    Serial.println("Color Palette:");
    Serial.println("--------------");
    for (int i = 0; i < NUM_COLORS; i++) {
        Serial.print("Position ");
        Serial.print(i, HEX);
        Serial.print(": ");
        Serial.print(colorPalette[i].name);
        Serial.print(" (RGB: ");
        Serial.print(colorPalette[i].r);
        Serial.print(", ");
        Serial.print(colorPalette[i].g);
        Serial.print(", ");
        Serial.print(colorPalette[i].b);
        Serial.println(")");

        if (i == 7) {
            Serial.println("              ---");
        }
    }
    Serial.println("--------------\n");
}

/**
 * @brief Demo color cycle effect
 */
void demoColorCycle()
{
    Serial.println("Color cycle demo started...");

    // Quickly cycle through all colors
    for (int i = 0; i < NUM_COLORS; i++) {
        step16.setRgb(colorPalette[i].r, colorPalette[i].g, colorPalette[i].b);
        Serial.print(".");
        delay(DEMO_DELAY);
    }

    Serial.println(" Completed");

    // Rainbow gradient effect
    Serial.println("Rainbow gradient demo...");
    for (int hue = 0; hue < 360; hue += RAINBOW_STEP) {
        float r, g, b;
        hsvToRgb(hue, 1.0, 1.0, &r, &g, &b);
        step16.setRgb((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
        delay(RAINBOW_DELAY);
    }

    Serial.println("Demo completed\n");

    // Turn off RGB
    step16.setRgb(0, 0, 0);
    delay(500);
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