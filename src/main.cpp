//tenstar robot nrf52840 pro micro

// #include <Arduino.h>
// #include <bluefruit.h>

// const int testPins[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
// const int numPins = sizeof(testPins) / sizeof(testPins[0]);

// void setup() {
//   Serial.begin(115200);
//   Serial.println("LED scan start");

//   for (int i = 0; i < numPins; i++) {
//     int pin = testPins[i];
//     pinMode(pin, OUTPUT);
//     digitalWrite(pin, HIGH);
//     Serial.print("Testing pin HIGH: ");
//     Serial.println(pin);
//     delay(1000);

//     digitalWrite(pin, LOW);
//     Serial.print("Testing pin LOW: ");
//     Serial.println(pin);
//     delay(1000);
//   }

//   Serial.println("LED scan finished");
// }

// void loop()
// {
//     // нищо не правим в loop
// }

// #include <Arduino.h>
// #include <bluefruit.h>

// #define TEST_PIN 12 //12 0.08 ,11 0.06, 7 102 ,3 115 D18,  2=0.10 D16,  1=0.24 D5

// BLEHidAdafruit blehid;

// unsigned long lastTrigger = 0;
// const unsigned long debounceMs = 500;

// void setup() {
//   Serial.begin(115200);
//   delay(2000);

//   Serial.println("\n=== nRF52840 SINGLE PIN TEST ===");
//   Serial.print("Testing Arduino pin: ");
//   Serial.println(TEST_PIN);
//   Serial.println("Touch GND -> MUTE\n");

//   pinMode(TEST_PIN, INPUT_PULLUP);

//   Bluefruit.begin();
//   Bluefruit.setTxPower(4);
//   Bluefruit.setName("nRF52-PinTest");

//   blehid.begin();

//   Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
//   Bluefruit.Advertising.addService(blehid);
//   Bluefruit.ScanResponse.addName();
//   Bluefruit.Advertising.start(0);
// }

// void loop() {
//   if (!Bluefruit.connected()) return;

//   if (digitalRead(TEST_PIN) == LOW) {
//     if (millis() - lastTrigger > debounceMs) {
//       lastTrigger = millis();

//       Serial.print("GND detected on pin ");
//       Serial.println(TEST_PIN);

//       blehid.consumerKeyPress(HID_USAGE_CONSUMER_MUTE);
//       blehid.consumerKeyRelease();
//     }
//   }
// }

#include <Arduino.h>
#include <bluefruit.h>

#define ENCODER_CLK 11
#define ENCODER_DT  12
#define ENCODER_SW  1
#define LED_PIN     15

// Debounce за бутона
#define SW_DEBOUNCE_MS 350

BLEDis bledis;
BLEHidAdafruit blehid;
BLEBas blebas;

volatile int encoderDelta = 0;
volatile int lastClkState;

// За debounce на бутона
volatile uint32_t lastSwPress = 0;

void readEncoder() {
    int clkState = digitalRead(ENCODER_CLK);
    if (clkState != lastClkState) {
        if (digitalRead(ENCODER_DT) != clkState) {
            encoderDelta++;
        } else {
            encoderDelta--;
        }
        lastClkState = clkState;
    }
}

void setup() {
    delay(2000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    
    pinMode(ENCODER_CLK, INPUT);
    pinMode(ENCODER_DT,  INPUT);
    pinMode(ENCODER_SW,  INPUT_PULLUP);

    lastClkState = digitalRead(ENCODER_CLK);
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), readEncoder, CHANGE);

    Bluefruit.begin();
    Bluefruit.setTxPower(-16);
    Bluefruit.setName("nRF52-Volume-LowPower");


    Bluefruit.Periph.setConnInterval(80, 160); // 100ms–200ms

    bledis.setManufacturer("Logitech");
    bledis.setModel("nRF52-HID");
    bledis.begin();

    blebas.begin();
    blebas.write(100);
    blehid.begin();

    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addAppearance(0x03C1);
    Bluefruit.Advertising.addService(blehid);
    Bluefruit.Advertising.addService(blebas);
    Bluefruit.ScanResponse.addName();

    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(2048, 2048);
    Bluefruit.Advertising.start(0);
}

void loop() {
    if (!Bluefruit.connected()) {
        sd_app_evt_wait();
        return;
    }

    
    if (digitalRead(ENCODER_SW) == LOW) {
        uint32_t now = millis();
        if ((now - lastSwPress) > SW_DEBOUNCE_MS) {
            lastSwPress = now;
            blehid.consumerKeyPress(HID_USAGE_CONSUMER_MUTE);
            delay(10);
            blehid.consumerKeyRelease();
        }
    }

    if (abs(encoderDelta) >= 1) {
        noInterrupts();
        int delta = encoderDelta;
        encoderDelta = 0;
        interrupts();

        uint16_t key = (delta > 0)
            ? HID_USAGE_CONSUMER_VOLUME_INCREMENT
            : HID_USAGE_CONSUMER_VOLUME_DECREMENT;

        blehid.consumerKeyPress(key);
        delay(5);
        blehid.consumerKeyRelease();
    }

    sd_app_evt_wait();
}