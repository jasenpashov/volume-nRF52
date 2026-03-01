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

#define ENCODER_CLK 11 //  11 D1 P0.06,
#define ENCODER_DT 12  // 12 D2 P0.08
#define ENCODER_SW 1   // 1 D5 P0.24
#define LED_PIN 15

BLEDis bledis;
BLEHidAdafruit blehid;
BLEBas blebas;

int lastClkState;
unsigned long lastEncoderMove = 0;
unsigned long pressTime = 0;
bool buttonHeld = false;
const unsigned long LONG_PRESS_RESET = 15000;

void startAdv(void);
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);

int encoderDelta = 0;
unsigned long lastSend = 0;

void setup()
{
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    pinMode(ENCODER_CLK, INPUT);
    pinMode(ENCODER_DT, INPUT);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    lastClkState = digitalRead(ENCODER_CLK);

    Bluefruit.begin();
    Bluefruit.setTxPower(4); // 4 maximums, -16 Enough for 2-3 meters. Saves a lot of energy
    Bluefruit.setName("nRF52-Volume");

    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    bledis.setManufacturer("Logitech");
    bledis.setModel("nRF52-HID");
    bledis.begin();

    blebas.begin();
    blebas.write(100);

    blehid.begin();

    startAdv();
    Serial.println("Ready for Windows pairing...");
}

void loop()
{
    if (!Bluefruit.connected()) return;

    /* =========================
       BUTTON (Mute / Play / Reset)
       ========================= */
    int swState = digitalRead(ENCODER_SW);

    if (swState == LOW && !buttonHeld)
    {
        pressTime = millis();
        buttonHeld = true;
    }
    else if (swState == HIGH && buttonHeld)
    {
        unsigned long held = millis() - pressTime;

        if (held >= LONG_PRESS_RESET)
        {
            Bluefruit.Periph.clearBonds();
            NVIC_SystemReset();
        }
        else if (held >= 800)
        {
            blehid.consumerKeyPress(HID_USAGE_CONSUMER_PLAY_PAUSE);
            delay(10);
            blehid.consumerKeyRelease();
        }
        else
        {
            blehid.consumerKeyPress(HID_USAGE_CONSUMER_MUTE);
            delay(10);
            blehid.consumerKeyRelease();
        }

        buttonHeld = false;
    }

    /* =========================
       ENCODER READ (raw)
       ========================= */
    static int encoderDelta = 0;
    static unsigned long lastSend = 0;

    if (millis() - lastEncoderMove > 5)
    {
        int clkState = digitalRead(ENCODER_CLK);

        if (clkState != lastClkState)
        {
            if (digitalRead(ENCODER_DT) != clkState)
                encoderDelta++;
            else
                encoderDelta--;

            lastEncoderMove = millis();
        }

        lastClkState = clkState;
    }

    /* =========================
       ENCODER → HID (filtered)
       ========================= */
    if (millis() - lastSend > 40 && encoderDelta != 0)
    {
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

        lastSend = millis();
    }
    delay(5); // A little delay to reduce cycles and save power consumption
}

void startAdv(void)
{
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    // Bluefruit.Advertising.addAppearance(0x03C3);
    Bluefruit.Advertising.addAppearance(0x03C1);
    
    Bluefruit.Advertising.addService(blehid);
    Bluefruit.Advertising.addService(blebas);
    Bluefruit.ScanResponse.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);
    Bluefruit.Advertising.start(0);
}

void connect_callback(uint16_t conn_handle)
{
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Connected!");
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    digitalWrite(LED_PIN, LOW);
    Serial.print("Disconnected, reason: 0x");
    Serial.println(reason, HEX);
}