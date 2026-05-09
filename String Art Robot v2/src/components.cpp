#include "components.h"

/* ====================================================================================================== */
/*                                              COMPONENTS                                                */
/* ====================================================================================================== */

/* LCD Display */
    #define I2C_SDA 21
    #define I2C_SCL 22
    LiquidCrystal_I2C lcd(0x27, 16, 2); 
    unsigned long lastDisplayUpdate = 0;
    
    void initLcd() {
        Wire.begin(I2C_SDA, I2C_SCL);
        lcd.init();
        lcd.backlight();
        lcd.setCursor(0, 0);
        lcd.print("ESP32 Start!");
    }
    
/* Servo */
    #define SERVO_PIN 25
    bool servoIsLeft = true;
    const int minimumPulseWidthUs = 500; // microseconds
    const int maximumPulseWidthUs = 2400; // microseconds
    
    #define USE_LIBRARY_PWM true

    #if USE_LIBRARY_PWM
        Servo libServo;
    #else
        int servoCurrentPulseWidth = (minimumPulseWidthUs+maximumPulseWidthUs)/2; // start at pos 0 (in middle)
    #endif

    void initServo() {
        #if USE_LIBRARY_PWM
            libServo.setPeriodHertz(50); // Standard 50Hz for SG90
            libServo.attach(SERVO_PIN, minimumPulseWidthUs, maximumPulseWidthUs); 
            libServo.write(0); // middle position
        #else
            pinMode(SERVO_PIN, OUTPUT);
        #endif
    }
    void servoWrite(int angleDeg = 0) {
        #if USE_LIBRARY_PWM
            libServo.write(angleDeg);
        #else
            if (angleDeg < 0) angleDeg = 0; // clamp
            if (angleDeg > 180) angleDeg = 180; // clamp

            int neededPulse = (angleDeg * (maximumPulseWidthUs-minimumPulseWidthUs))/180 + minimumPulseWidthUs;
            servoCurrentPulseWidth = neededPulse;
        #endif
    }
    void loopServo() {
        #if USE_LIBRARY_PWM
            // handled in background
        #else
            digitalWrite(SERVO_PIN, HIGH);
            delayMicroseconds(servoCurrentPulseWidth);
            digitalWrite(SERVO_PIN, LOW);
            delayMicroseconds(20000 - servoCurrentPulseWidth);
        #endif
    }

/* Potentiometer */
    #define POTENTIONMETER_PIN 34
    #define POTENTIONMETER_MIN_VALUE 220
    #define POTENTIONMETER_MAX_VALUE 4000

    float potentiometerValue = 0; 

    void initPotentiometer() {
        pinMode(POTENTIONMETER_PIN, INPUT);
    }
    void readPotentiometer() {
        int currentAnaglogValue = analogRead(POTENTIONMETER_PIN);
        potentiometerValue = (float)(currentAnaglogValue-POTENTIONMETER_MIN_VALUE) / (float)(POTENTIONMETER_MAX_VALUE-POTENTIONMETER_MIN_VALUE);
        potentiometerValue  = max(0.0f, min(1.0f, 1.0f - potentiometerValue)); 
    }   

/* Led */
    #define LED_R 13
    #define LED_G 14
    #define LED_B 32
    int colorState = 0; 
    
    void setRGBColor(int r, int g, int b);
    void initLed() {
        pinMode(LED_R, OUTPUT);
        pinMode(LED_G, OUTPUT);
        pinMode(LED_B, OUTPUT);
        setRGBColor(0, 0, 0);
    }
    void setRGBColor(int r, int g, int b) {
      analogWrite(LED_R, r);
      analogWrite(LED_G, g);
      analogWrite(LED_B, b);
    }

/* Button */
    #define BUTTON_PIN 16
    volatile unsigned long lastButton1Press = 0;
    const int debounceDelay = 250;
    volatile bool isButtonPressed = false;

    void IRAM_ATTR handleButton1() {
        unsigned long interruptTime = millis();
        if (interruptTime - lastButton1Press > debounceDelay) { 
            isButtonPressed = true;
            lastButton1Press = interruptTime;
        }
    }
    void initButton() {
        pinMode(BUTTON_PIN, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButton1, FALLING);
    }   

/* SD Card Reader */
    #define SD_CS_PIN 5
    #define SD_SCK_PIN 18
    #define SD_MISO_PIN 19
    #define SD_MOSI_PIN 23

    void initSDReader() {
        Serial.print("Inicjalizacja karty SD... ");
        if (!SD.begin(SD_CS_PIN)) {
            Serial.println("BLAD! Sprawdz kable lub karte.");
        } else {
            Serial.println("SUKCES! Karta wykryta.");
        }
    }

/* Hall Sensor */
    #define HALL_INPUT_PIN 35
    int hallValue = 0;

    void initHallSensor() {
        pinMode(HALL_INPUT_PIN, INPUT);
    }
    void readHallSensor() {
        hallValue = analogRead(HALL_INPUT_PIN);
    }

/* DRV Stepper Controller (AccelStepper) */
    #define DRV_STEP_PIN 26
    #define DRV_DIR_PIN 27
    
    // Inicjalizacja biblioteki w trybie DRIVER (Step/Dir)
    AccelStepper stepper(AccelStepper::DRIVER, DRV_STEP_PIN, DRV_DIR_PIN);
    
    float currentStepperSpeed = 0.0; //800.0;

    void initStepper() {
        stepper.setMaxSpeed(2000.0);
        stepper.setSpeed(currentStepperSpeed);
    }
