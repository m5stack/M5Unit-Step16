/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @Hardwares: Basic/Fire/Gray/Core2(PortA) + Unit Step16
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5Unit-Step16:https://github.com/m5stack/M5Unit-Step16
 */

#include <M5Unified.h>
#include <M5UnitStep16.h>

#define I2C_SDA_PIN (21)  // I2C SDA pin (Port A)
#define I2C_SCL_PIN (22)  // I2C SCL pin (Port A)

#define CHECK_INTERVAL         (1000)  // Check connection state every second (ms)
#define LOOP_DELAY             (20)    // Main loop delay (ms)
#define CONNECTION_CHECK_DELAY (100)   // Connection check delay (ms)
#define BREATH_STEP            (3)     // Breath effect step size
#define BREATH_MIN             (0)     // Minimum brightness for breath effect
#define BREATH_MAX             (100)   // Maximum brightness for breath effect
#define NUM_RGB_COLORS         (6)     // Number of RGB test colors
#define TEXT_SIZE_SMALL        (1)     // Small text size
#define TEXT_SIZE_LARGE        (2)     // Large text size
#define TITLE_BAR_HEIGHT       (30)    // Height of title bar
#define ENCODER_BAR_HEIGHT     (30)    // Height of encoder value bar
#define MODE_TEXT_Y            (70)    // Y position for mode text
#define MODE_VALUE_Y           (90)    // Y position for mode value
#define STATUS_TEXT_Y          (120)   // Y position for status text
#define CONTENT_RECT_Y         (145)   // Y position for content rectangle
#define CONTENT_RECT_HEIGHT    (65)    // Height of content rectangle
#define BUTTON_BAR_Y           (220)   // Y position for button bar
#define BUTTON_BAR_HEIGHT      (20)    // Height of button bar

// UnitStep16 object
UnitStep16 step16;

// Test mode enumeration
enum TestMode {
    MODE_LED_TEST = 0,   // LED breath test
    MODE_RGB_TEST,       // RGB breath test
    MODE_ROTATION_TEST,  // Rotation direction test
    MODE_DATA_READ,      // Read all data
    MODE_FLASH_SAVE,     // Save flash test
    MODE_COUNT           // Total number of modes
};

// Current state
TestMode currentMode        = MODE_LED_TEST;
uint8_t encoderValue        = 0;
bool isTestRunning          = false;
unsigned long testStartTime = 0;

// LED breath test state
uint8_t ledBreathValue    = 0;
int8_t ledBreathDirection = 1;

// RGB breath test state
uint8_t rgbBreathValue    = 0;
int8_t rgbBreathDirection = 1;
uint8_t rgbColorIndex     = 0;

// Rotation direction test state
uint8_t lastEncoderValue   = 0;
uint8_t currentRotationDir = 0;  // 0=clockwise, 1=counterclockwise
bool rotationChanged       = false;

// Device connection state
bool deviceConnected        = false;
unsigned long lastCheckTime = 0;

// Test mode name
const char* modeNames[] = {"LED TEST", "RGB TEST", "ROTATION TEST", "READ FLASH", "SAVE FLASH"};

void setup()
{
    M5.begin();
    M5.Power.begin();
    M5.Lcd.setTextSize(TEXT_SIZE_SMALL);
    M5.Lcd.setTextColor(WHITE, BLACK);

    // Initialize I2C (Port A: SDA=21, SCL=22)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // Initialize UnitStep16
    deviceConnected = initializeDevice();

    // Display initial interface
    updateDisplay();
}

void loop()
{
    M5.update();

    // Check device connection state periodically
    if (millis() - lastCheckTime > CHECK_INTERVAL) {
        lastCheckTime           = millis();
        bool currentlyConnected = checkDeviceConnection();

        if (currentlyConnected != deviceConnected) {
            deviceConnected = currentlyConnected;

            if (deviceConnected) {
                // Device reconnected, reinitialize
                if (initializeDevice()) {
                    isTestRunning = false;
                    updateDisplay();
                }
            } else {
                // Device disconnected
                isTestRunning = false;
                showDisconnectedScreen();
            }
        }
    }

    // If the device is not connected, do not execute other operations
    if (!deviceConnected) {
        delay(CONNECTION_CHECK_DELAY);
        return;
    }

    // Read encoder value
    uint8_t newValue = step16.getValue();
    if (newValue != encoderValue) {
        encoderValue = newValue;
        updateEncoderDisplay();
    }

    // A button: previous mode
    if (M5.BtnA.wasPressed()) {
        if (currentMode == 0) {
            currentMode = (TestMode)(MODE_COUNT - 1);
        } else {
            currentMode = (TestMode)(currentMode - 1);
        }
        isTestRunning = false;
        updateDisplay();
    }

    // B button: start/stop test
    if (M5.BtnB.wasPressed()) {
        // Special handling: in rotation direction test mode, B button is used to switch direction
        if (currentMode == MODE_ROTATION_TEST && isTestRunning) {
            // Switch rotation direction
            currentRotationDir = (currentRotationDir == 0) ? 1 : 0;
            step16.setSwitchState(currentRotationDir);
            rotationChanged  = true;
            lastEncoderValue = encoderValue;
            updateDisplay();
            return;
        }

        isTestRunning = !isTestRunning;
        testStartTime = millis();

        if (isTestRunning) {
            // Initialize when starting test
            switch (currentMode) {
                case MODE_LED_TEST:
                    step16.setLedConfig(UNIT_STEP16_LED_CONFIG_ON);
                    ledBreathValue     = 0;
                    ledBreathDirection = 1;
                    break;

                case MODE_RGB_TEST:
                    step16.setRgbConfig(UNIT_STEP16_RGB_CONFIG_ON);
                    rgbBreathValue     = 0;
                    rgbBreathDirection = 1;
                    rgbColorIndex      = 0;
                    break;

                case MODE_ROTATION_TEST:
                    lastEncoderValue   = encoderValue;
                    currentRotationDir = step16.getSwitchState();
                    rotationChanged    = false;
                    break;

                case MODE_DATA_READ:
                    // Data read mode does not need to be initialized
                    break;

                case MODE_FLASH_SAVE:
                    // Flash save mode does not need to be initialized
                    break;
            }
        } else {
            // Restore default state when stopping test
            step16.setLedConfig(UNIT_STEP16_LED_CONFIG_DEFAULT);
            step16.setLedBrightness(UNIT_STEP16_LED_BRIGHTNESS_DEFAULT);
            step16.setRgbConfig(UNIT_STEP16_RGB_CONFIG_DEFAULT);
            step16.setRgbBrightness(UNIT_STEP16_RGB_BRIGHTNESS_DEFAULT);
            step16.setRgb(UNIT_STEP16_R_VALUE_DEFAULT, UNIT_STEP16_G_VALUE_DEFAULT, UNIT_STEP16_B_VALUE_DEFAULT);
        }

        updateDisplay();
    }

    // C button: next mode
    if (M5.BtnC.wasPressed()) {
        currentMode   = (TestMode)((currentMode + 1) % MODE_COUNT);
        isTestRunning = false;
        updateDisplay();
    }

    // Execute current test
    if (isTestRunning) {
        runCurrentTest();
    }

    delay(LOOP_DELAY);
}

// Update the entire display interface
void updateDisplay()
{
    M5.Lcd.clear();

    // Title bar
    M5.Lcd.fillRect(0, 0, 320, 30, DARKGREY);
    M5.Lcd.setTextColor(WHITE, DARKGREY);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 7);
    M5.Lcd.print("Unit Step16 Test Demo");

    // Encoder value display area
    M5.Lcd.fillRect(0, 30, 320, 30, NAVY);
    M5.Lcd.setTextColor(WHITE, NAVY);
    M5.Lcd.setCursor(10, 37);
    M5.Lcd.printf("Encoder Value: 0x%X (%d)", encoderValue, encoderValue);

    // Current mode display
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 70);
    M5.Lcd.print("Current Mode:");
    M5.Lcd.setTextColor(CYAN, BLACK);
    M5.Lcd.setCursor(10, 90);
    M5.Lcd.print(modeNames[currentMode]);

    // Test status
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(10, 120);
    M5.Lcd.print("Status: ");
    if (isTestRunning) {
        M5.Lcd.setTextColor(GREEN, BLACK);
        M5.Lcd.print("Testing...");
    } else {
        M5.Lcd.setTextColor(YELLOW, BLACK);
        M5.Lcd.print("Standby");
    }

    // Test content display area
    M5.Lcd.drawRect(5, 145, 310, 65, WHITE);
    displayTestContent();

    // Button hint
    drawButtonLabels();
}

// Only update encoder value display
void updateEncoderDisplay()
{
    M5.Lcd.fillRect(0, 30, 320, 30, NAVY);
    M5.Lcd.setTextColor(WHITE, NAVY);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 37);
    M5.Lcd.printf("Encoder Value: 0x%X (%d)", encoderValue, encoderValue);
}

// Display test content
void displayTestContent()
{
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(WHITE, BLACK);
    int y = 150;

    switch (currentMode) {
        case MODE_LED_TEST:
            if (isTestRunning) {
                M5.Lcd.setCursor(10, y);
                M5.Lcd.printf("LED Brightness: %d%%", ledBreathValue);
                M5.Lcd.setCursor(10, y + 15);
                M5.Lcd.print("Status: ");
                if (ledBreathValue == 0) {
                    M5.Lcd.print("Off");
                } else if (ledBreathValue == 100) {
                    M5.Lcd.print("Max");
                } else {
                    M5.Lcd.print(ledBreathDirection > 0 ? "Bright" : "Dim");
                }
            } else {
                M5.Lcd.setCursor(10, y);
                M5.Lcd.print("Press B to start LED breath test");
                M5.Lcd.setCursor(10, y + 15);
                M5.Lcd.print("LED will breath from 0% to 100%");
            }
            break;

        case MODE_RGB_TEST:
            if (isTestRunning) {
                M5.Lcd.setCursor(10, y);
                M5.Lcd.printf("RGB Brightness: %d%%", rgbBreathValue);
                M5.Lcd.setCursor(10, y + 15);
                const char* colors[] = {"Red", "Green", "Blue", "Yellow", "Purple", "Cyan"};
                M5.Lcd.printf("Current Color: %s", colors[rgbColorIndex]);
                M5.Lcd.setCursor(10, y + 30);
                M5.Lcd.print("Status: ");
                if (rgbBreathValue == 0) {
                    M5.Lcd.print("Off");
                } else if (rgbBreathValue == 100) {
                    M5.Lcd.print("Max");
                } else {
                    M5.Lcd.print(rgbBreathDirection > 0 ? "Bright" : "Dim");
                }
            } else {
                M5.Lcd.setCursor(10, y);
                M5.Lcd.print("Press B to start RGB breath test");
                M5.Lcd.setCursor(10, y + 15);
                M5.Lcd.print("RGB will cycle through multiple colors");
                M5.Lcd.setCursor(10, y + 30);
                M5.Lcd.print("Each color has a breath effect");
            }
            break;

        case MODE_ROTATION_TEST:
            if (isTestRunning) {
                M5.Lcd.setCursor(10, y);
                M5.Lcd.printf("Current Direction: %s", currentRotationDir == 0 ? "Clockwise" : "Counterclockwise");
                M5.Lcd.setCursor(10, y + 15);
                M5.Lcd.printf("Last Value: 0x%X -> Current: 0x%X", lastEncoderValue, encoderValue);
                M5.Lcd.setCursor(10, y + 30);
                if (rotationChanged) {
                    M5.Lcd.setTextColor(GREEN, BLACK);
                    M5.Lcd.print("Direction changed successfully!");
                } else {
                    M5.Lcd.print("Rotate knob to see the effect");
                }
                M5.Lcd.setCursor(10, y + 45);
                M5.Lcd.setTextSize(1);
                M5.Lcd.setTextColor(YELLOW, BLACK);
                M5.Lcd.print("Press B again to switch direction");
            } else {
                M5.Lcd.setCursor(10, y);
                M5.Lcd.print("Press B to test rotation direction");
                M5.Lcd.setCursor(10, y + 15);
                M5.Lcd.print("Can switch between clockwise and");
                M5.Lcd.setCursor(10, y + 30);
                M5.Lcd.print("counterclockwise rotation");
            }
            break;

        case MODE_DATA_READ:
            M5.Lcd.setCursor(10, y);
            M5.Lcd.printf("LED Config: %d", step16.getLedConfig());
            M5.Lcd.setCursor(160, y);
            M5.Lcd.printf("LED Brightness: %d%%", step16.getLedBrightness());

            M5.Lcd.setCursor(10, y + 15);
            M5.Lcd.printf("RGB Status: %s", step16.getRgbConfig() == UNIT_STEP16_RGB_CONFIG_OFF ? "Off" : "On");
            M5.Lcd.setCursor(160, y + 15);
            M5.Lcd.printf("RGB Brightness: %d%%", step16.getRgbBrightness());

            uint8_t r, g, b;
            step16.getRgb(&r, &g, &b);
            M5.Lcd.setCursor(10, y + 30);
            M5.Lcd.printf("RGB Value: R=%d G=%d B=%d", r, g, b);

            M5.Lcd.setCursor(160, y + 30);
            M5.Lcd.printf("Ver:0x%02X", step16.getVersion());

            M5.Lcd.setCursor(10, y + 45);
            M5.Lcd.printf("Rotation Direction: %s",
                          step16.getSwitchState() == UNIT_STEP16_SWITCH_CLOCKWISE ? "CW" : "CCW");

            M5.Lcd.setCursor(160, y + 45);
            M5.Lcd.printf("I2C Address: 0x%02X", step16.getAddress());
            break;

        case MODE_FLASH_SAVE:
            if (isTestRunning) {
                unsigned long elapsed = millis() - testStartTime;
                M5.Lcd.setCursor(10, y);

                if (elapsed < 1000) {
                    M5.Lcd.print("Saving LED configuration...");
                } else if (elapsed < 2000) {
                    M5.Lcd.print("LED configuration saved!");
                    M5.Lcd.setCursor(10, y + 15);
                    M5.Lcd.print("Saving RGB configuration...");
                } else if (elapsed < 3000) {
                    M5.Lcd.print("LED configuration saved!");
                    M5.Lcd.setCursor(10, y + 15);
                    M5.Lcd.print("RGB configuration saved!");
                    M5.Lcd.setCursor(10, y + 30);
                    M5.Lcd.setTextColor(GREEN, BLACK);
                    M5.Lcd.print("All configurations saved!");
                } else {
                    isTestRunning = false;
                    updateDisplay();
                }
            } else {
                M5.Lcd.setCursor(10, y);
                M5.Lcd.print("Press B to save current configuration to Flash");
                M5.Lcd.setCursor(10, y + 15);
                M5.Lcd.print("Will save all LED and RGB configurations");
                M5.Lcd.setCursor(10, y + 30);
                M5.Lcd.print("Configuration will not be lost after power off");
            }
            break;
    }
}

// Initialize device
bool initializeDevice()
{
    if (!step16.begin()) {
        return false;
    }

    // Set initial configuration
    step16.setLedConfig(UNIT_STEP16_LED_CONFIG_ON);           // LED always on
    step16.setLedBrightness(UNIT_STEP16_LED_BRIGHTNESS_MAX);  // LED brightness 100%
    step16.setSwitchState(UNIT_STEP16_SWITCH_CLOCKWISE);      // Clockwise direction
    step16.setRgbConfig(UNIT_STEP16_RGB_CONFIG_ON);           // Enable RGB
    step16.setRgbBrightness(UNIT_STEP16_RGB_BRIGHTNESS_MAX);  // RGB brightness 100%
    step16.setRgb(UNIT_STEP16_R_VALUE_DEFAULT, UNIT_STEP16_G_VALUE_DEFAULT, UNIT_STEP16_B_VALUE_DEFAULT);  // RGB close

    return true;
}

// Check device connection state
bool checkDeviceConnection()
{
    Wire.beginTransmission(0x48);  // Default I2C address
    return (Wire.endTransmission() == 0);
}

// Display disconnected screen
void showDisconnectedScreen()
{
    M5.Lcd.clear();

    M5.Lcd.fillRect(0, 0, 320, 30, DARKGREY);
    M5.Lcd.setTextColor(WHITE, DARKGREY);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 7);
    M5.Lcd.print("Unit Step16 Test Demo");

    // Disconnected hint
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(40, 80);
    M5.Lcd.print("DISCONNECTED");

    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(20, 130);
    M5.Lcd.print("Device not detected");

    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(20, 160);
    M5.Lcd.print("Please check:");
    M5.Lcd.setCursor(30, 175);
    M5.Lcd.print("1. Cable connection");
    M5.Lcd.setCursor(30, 190);
    M5.Lcd.print("2. Power supply");
    M5.Lcd.setCursor(30, 205);
    M5.Lcd.print("3. I2C address (0x48)");
}

// Draw button labels
void drawButtonLabels()
{
    // Button area background
    M5.Lcd.fillRect(0, 210, 106, 30, DARKGREEN);
    M5.Lcd.fillRect(107, 210, 106, 30, DARKGREEN);
    M5.Lcd.fillRect(214, 210, 106, 30, DARKGREEN);

    M5.Lcd.setTextColor(WHITE, DARKGREEN);
    M5.Lcd.setTextSize(1);

    // A button - previous
    M5.Lcd.setCursor(25, 220);
    M5.Lcd.print("< Previous");

    // B button - test
    M5.Lcd.setCursor(130, 220);
    M5.Lcd.print(isTestRunning ? "Stop" : "Test");

    // C button - next
    M5.Lcd.setCursor(235, 220);
    M5.Lcd.print("Next >");
}

// Execute current test
void runCurrentTest()
{
    static unsigned long lastUpdateTime = 0;

    switch (currentMode) {
        case MODE_LED_TEST:
            // LED breath effect
            if (millis() - lastUpdateTime > 1) {
                ledBreathValue += ledBreathDirection * 4;

                if (ledBreathValue >= 100) {
                    ledBreathValue     = 100;
                    ledBreathDirection = -1;
                } else if (ledBreathValue <= 0) {
                    ledBreathValue     = 0;
                    ledBreathDirection = 1;
                }

                step16.setLedBrightness(ledBreathValue);

                // Update display
                M5.Lcd.fillRect(10, 150, 300, 55, BLACK);
                displayTestContent();

                lastUpdateTime = millis();
            }
            break;

        case MODE_RGB_TEST:
            // RGB breath effect
            if (millis() - lastUpdateTime > 1) {
                rgbBreathValue += rgbBreathDirection * 4;

                if (rgbBreathValue >= 100) {
                    rgbBreathValue     = 100;
                    rgbBreathDirection = -1;
                } else if (rgbBreathValue <= 0) {
                    rgbBreathValue     = 0;
                    rgbBreathDirection = 1;
                    // Switch to next color
                    rgbColorIndex = (rgbColorIndex + 1) % 6;
                }

                // Set RGB color and brightness
                uint8_t colors[][3] = {
                    {255, 0, 0},    // Red
                    {0, 255, 0},    // Green
                    {0, 0, 255},    // Blue
                    {255, 255, 0},  // Yellow
                    {255, 0, 255},  // Purple
                    {0, 255, 255}   // Cyan
                };

                step16.setRgb(colors[rgbColorIndex][0], colors[rgbColorIndex][1], colors[rgbColorIndex][2]);
                step16.setRgbBrightness(rgbBreathValue);

                // Update display
                M5.Lcd.fillRect(10, 150, 300, 55, BLACK);
                displayTestContent();

                lastUpdateTime = millis();
            }
            break;

        case MODE_ROTATION_TEST:
            // Rotation direction test
            if (encoderValue != lastEncoderValue) {
                lastEncoderValue = encoderValue;
                M5.Lcd.fillRect(10, 150, 300, 55, BLACK);
                displayTestContent();
            }
            break;

        case MODE_DATA_READ:
            // Update display data periodically
            if (millis() - lastUpdateTime > 500) {
                M5.Lcd.fillRect(10, 150, 300, 55, BLACK);
                displayTestContent();
                lastUpdateTime = millis();
            }
            break;

        case MODE_FLASH_SAVE:
            // Flash save operation
            {
                unsigned long elapsed = millis() - testStartTime;

                if (elapsed >= 1000 && elapsed < 1100) {
                    step16.saveToFlash(UNIT_STEP16_SAVE_LED_CONFIG);  // Save LED configuration
                } else if (elapsed >= 2000 && elapsed < 2100) {
                    step16.saveToFlash(UNIT_STEP16_SAVE_RGB_CONFIG);  // Save RGB configuration
                }

                // Update display
                M5.Lcd.fillRect(10, 150, 300, 55, BLACK);
                displayTestContent();
            }
            break;
    }
}
