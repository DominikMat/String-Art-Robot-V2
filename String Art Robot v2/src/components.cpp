#include "components.h"

// wlasne znaki do lcd
byte invLess[8] = { 0b11101, 0b11011, 0b10111, 0b01111, 0b10111, 0b11011, 0b11101, 0b11111 };
byte invDash[8] = { 0b11111, 0b11111, 0b11111, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111 };
byte normalDot[8] = { 0b00000, 0b00000, 0b01110, 0b01110, 0b01110, 0b00000, 0b00000, 0b00000 };
byte invDot[8] = { 0b11111, 0b11111, 0b10001, 0b10001, 0b10001, 0b11111, 0b11111, 0b11111 };
byte pauseChar[8] = { 0b00000, 0b01010, 0b01010, 0b01010, 0b01010, 0b01010, 0b00000, 0b00000 };
byte invPauseChar[8] = { 0b11111, 0b10101, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111, 0b11111 };

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

        lcd.createChar(CHAR_INVERTED_LESS, invLess);
        lcd.createChar(CHAR_INVERTED_DASH, invDash);
        lcd.createChar(CHAR_DOT, normalDot);
        lcd.createChar(CHAR_INVERTED_DOT, invDot);
        lcd.createChar(CHAR_PAUSE, pauseChar);
        lcd.createChar(CHAR_INVERTED_PAUSE, invPauseChar);
        
        lcd.setCursor(0, 0);
        lcd.print("String art robot");
        lcd.setCursor(0, 1);
        lcd.print("V2 - Dominik Mat");
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
    #define POTENTIONMETER_MIN_VALUE 270
    #define POTENTIONMETER_MAX_VALUE 4000

    float potentiometerValue = 0; 

    void initPotentiometer() {
        pinMode(POTENTIONMETER_PIN, INPUT);
    }
    float readPotentiometer() {
        int currentAnaglogValue = analogRead(POTENTIONMETER_PIN);
        potentiometerValue = (float)(currentAnaglogValue-POTENTIONMETER_MIN_VALUE) / (float)(POTENTIONMETER_MAX_VALUE-POTENTIONMETER_MIN_VALUE);
        potentiometerValue  = max(0.0f, min(1.0f, 1.0f - potentiometerValue)); 
        return potentiometerValue;
    }   

/* Led */
    #define LED_R 13
    #define LED_G 14
    #define LED_B 32
    int colorState = 0; 
    
    void initLed() {
        pinMode(LED_R, OUTPUT);
        pinMode(LED_G, OUTPUT);
        pinMode(LED_B, OUTPUT);
        setRGBColor(0, 0, 0);
    }
    void setRGBColor(int r, int g, int b) {
        Serial.printf("Setting rgb diode colour: RGB(%d,%d,%d)",r,g,b);
        analogWrite(LED_R, r);
        analogWrite(LED_G, g);
        analogWrite(LED_B, b);
    }
    void set_rgb_specified_colour(RGBColour colour) {
        switch (colour) {
            case COLOUR_PRINTING:       setRGBColor(0, 0, 255);   break; // Niebieski - pracuje
            case COLOUR_PRINT_COMPLETE: setRGBColor(0, 150, 0);   break; // Ciemniejszy zielony
            case COLOUR_PRINT_PAUSED:   setRGBColor(255, 100, 0); break; // Jasny pomarańczowy
            case COLOUR_PRINT_STOPPED:  setRGBColor(255, 20, 0);  break; // Czerwono-pomarańczowy
            case COLOUR_READY:          setRGBColor(0, 255, 255); break; // Cyjan - gotowy do wyboru
            case COLOUR_ERROR:          setRGBColor(255, 0, 0);   break; // Czysty czerwony
            case COLOUR_CLEAR:          setRGBColor(0, 0, 0);     break; // Wyłączony
            case COLOUR_ALERT:          setRGBColor(255, 0, 255);     break; // Wyłączony
            default:                    setRGBColor(0, 0, 0);     break;
        }
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
    bool checkButtonPressed() {
        if (isButtonPressed) { isButtonPressed = false; return true; }
        else return false;
    }

/* SD Card Reader */
    #define SD_CS_PIN 5
    #define SD_SCK_PIN 18
    #define SD_MISO_PIN 19
    #define SD_MOSI_PIN 23

    void initSDReader() {
        Serial.print("Init SD Card ... ");
        if (!SD.begin(SD_CS_PIN)) {
            Serial.println("> SD card NOT detected.");
        } else {
            Serial.println("> SD card detected!");
        }
    }
    std::vector<std::string> readPrintableFileNames() {
        std::vector<std::string> fileNames;
        File root = SD.open("/");
        if (!root) {
            Serial.println("Nie mozna otworzyc folderu glownego SD");
            return fileNames;
        }

        File file = root.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                String name = file.name();
                // Szukamy plików z rozszerzeniem .gcode
                if (name.endsWith(".gcode") || name.endsWith(".GCODE")) {
                    int dotIndex = name.lastIndexOf('.'); // usun rozszerzenie
                    String nameWithoutExt = name.substring(0, dotIndex);
                    if (nameWithoutExt.startsWith("/")) { // unsun wiodacy slash
                        nameWithoutExt = nameWithoutExt.substring(1);
                    }
                    fileNames.push_back(nameWithoutExt.c_str());
                }
            }
            file = root.openNextFile();
        }
        root.close();
        return fileNames;
    }
    PrintSequence generatePrintSequenceFromFile(std::string filename) {
        PrintSequence seq;
        seq.print_name = filename;
        
        String fullPath = "/" + String(filename.c_str()) + ".gcode";
        File file = SD.open(fullPath);
        if (!file) return seq;

        bool inSequence = false;
        
        while (file.available()) {
            // Czytamy całą linię do znaku nowej linii, funkcja trim() usuwa białe znaki (np. \r)
            String line = file.readStringUntil('\n');
            line.trim(); 
            
            if (line.length() == 0) continue;

            if (line.startsWith("NAIL_TOTAL_NUM=")) {
                seq.nail_number = line.substring(15).toInt();
            } 
            else if (line.startsWith("SEQUENCE_LENGTH=")) {
                seq.sequence_length = line.substring(16).toInt();
            } 
            else if (line == "SEQUENCE_START") {
                inSequence = true;
            } 
            else if (line == "SEQUENCE_END") {
                inSequence = false;
            } 
            else if (inSequence) {
                seq.nail_sequence.push_back(line.toInt());
            }
        }
        file.close();
        return seq;
    }


/* Hall Sensor */
    #define HALL_INPUT_PIN 35
    int hallValue = 0;

    void initHallSensor() {
        pinMode(HALL_INPUT_PIN, INPUT);
    }
    int readHallSensor() {
        hallValue = analogRead(HALL_INPUT_PIN);
        return hallValue;
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
