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
#define ENCODER_DT 12
#define ENCODER_SW 1
#define LED_PIN 15

BLEDis bledis;
BLEHidAdafruit blehid;
BLEBas blebas;

volatile int encoderDelta = 0;
volatile int lastClkState;

void readEncoder()
{
    int clkState = digitalRead(ENCODER_CLK);
    if (clkState != lastClkState)
    {
        if (digitalRead(ENCODER_DT) != clkState)
        {
            encoderDelta++;
        }
        else
        {
            encoderDelta--;
        }
        lastClkState = clkState;
    }
}

void setup()
{

    // Serial.begin(115200); // Serial is stopped for economy.

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(ENCODER_CLK, INPUT);
    pinMode(ENCODER_DT, INPUT);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    lastClkState = digitalRead(ENCODER_CLK);

    // attach Interrupt
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), readEncoder, CHANGE);

    Bluefruit.begin();
    Bluefruit.setTxPower(-12); // Save energy -12 is for about 2-3 meters 4 is the maximum
    Bluefruit.setName("nRF52-Volume-LowPower");

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
    Bluefruit.Advertising.setInterval(619, 619);
    Bluefruit.Advertising.start(0);
}

void loop()
{

    if (!Bluefruit.connected())
    {
        waitForEvent();
        return;
    }

    if (digitalRead(ENCODER_SW) == LOW)
    {
        blehid.consumerKeyPress(HID_USAGE_CONSUMER_MUTE);
        delay(50);
        blehid.consumerKeyRelease();
        delay(300);
    }

    if (encoderDelta >= 2)
    {
        blehid.consumerKeyPress(HID_USAGE_CONSUMER_VOLUME_INCREMENT);
        delay(10);
        blehid.consumerKeyRelease();
        encoderDelta = 0;
    }
    else if (encoderDelta <= -2)
    {
        blehid.consumerKeyPress(HID_USAGE_CONSUMER_VOLUME_DECREMENT);
        delay(10);
        blehid.consumerKeyRelease();
        encoderDelta = 0;
    }

    // 3. PUT THE PROCESSOR TO SLEEP
    // This function waits for the next interrupt or BLE event.
    // It is an internal function specific to the nRF52 architecture.
    waitForEvent(); 
}