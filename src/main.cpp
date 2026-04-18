#include <Arduino.h>
#include <bluefruit.h>

#define ENCODER_CLK 11 //006
#define ENCODER_DT 12 //008
#define ENCODER_SW 1 //0.24
#define LED_PIN 15


#define SW_DEBOUNCE_MS 350

BLEDis bledis;
BLEHidAdafruit blehid;
BLEBas blebas;

volatile int encoderDelta = 0;
volatile int lastClkState;
bool isMuted = false;

volatile uint32_t lastSwPress = 0;

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
    delay(2000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(ENCODER_CLK, INPUT);
    pinMode(ENCODER_DT, INPUT);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    lastClkState = digitalRead(ENCODER_CLK);
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), readEncoder, CHANGE);

    Bluefruit.begin();
    Bluefruit.setTxPower(6); //-40, -20, -16, -12, -8, -4, 0, +2, +3, +4, +5, +6, +7, +8 
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

void loop()
{
    if (!Bluefruit.connected())
    {
        sd_app_evt_wait();
        return;
    }

    // if (digitalRead(ENCODER_SW) == LOW) {
    //     uint32_t now = millis();
    //     if ((now - lastSwPress) > SW_DEBOUNCE_MS) {
    //         lastSwPress = now;
    //         blehid.consumerKeyPress(HID_USAGE_CONSUMER_MUTE);
    //         delay(10);
    //         blehid.consumerKeyRelease();
    //     }
    // }
    if (digitalRead(ENCODER_SW) == LOW)
    {
        uint32_t now = millis();
        if ((now - lastSwPress) > SW_DEBOUNCE_MS)
        {
            lastSwPress = now;

            if (!isMuted)
            {
                // Mute + pause
                blehid.consumerKeyPress(HID_USAGE_CONSUMER_MUTE);
                delay(10);
                blehid.consumerKeyRelease();
                delay(10);
                blehid.consumerKeyPress(HID_USAGE_CONSUMER_PLAY_PAUSE);
                delay(10);
                blehid.consumerKeyRelease();
                isMuted = true;
            }
            else
            {
                // Unmute + Play
                blehid.consumerKeyPress(HID_USAGE_CONSUMER_MUTE);
                delay(10);
                blehid.consumerKeyRelease();
                delay(10);
                blehid.consumerKeyPress(HID_USAGE_CONSUMER_PLAY_PAUSE);
                delay(10);
                blehid.consumerKeyRelease();
                isMuted = false;
            }
        }
    }

    if (abs(encoderDelta) >= 1)
    {
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