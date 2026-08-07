#include <Arduino.h>
#include <bluefruit.h>

#define ENCODER_CLK 11 //006
#define ENCODER_DT  12 //008
#define ENCODER_SW  1  //0.24
#define LED_PIN     15
#define SW_DEBOUNCE_MS 350

// --- Power-saving connection interval settings ---
// Fast interval: used while the user is actively using the button/encoder
#define CONN_INTERVAL_FAST_MIN 6     // 6 * 1.25ms = 7.5ms
#define CONN_INTERVAL_FAST_MAX 12    // 12 * 1.25ms = 15ms
// Slow interval: used during idle time to save battery
#define CONN_INTERVAL_SLOW_MIN 1600  // 1600 * 1.25ms = 2000ms (2s)
#define CONN_INTERVAL_SLOW_MAX 3200  // 3200 * 1.25ms = 4000ms (4s)
// Idle time before switching to the slow interval
#define IDLE_TIMEOUT_MS (3UL * 60UL * 1000UL) // 3 minutes

BLEDis bledis;
BLEHidAdafruit blehid;
BLEBas blebas;

volatile int encoderDelta = 0;
volatile int lastClkState;
volatile bool activityFlag = false; // set by ISR, handled in loop()

bool isMuted = false;
volatile uint32_t lastSwPress = 0;

uint32_t lastActivity = 0;
bool isSlowMode = false;

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
        activityFlag = true; // signal activity, but don't touch BLE here
    }
}

// Switches back to the fast connection interval on activity.
// Called only from loop(), never from an ISR.
void goFast()
{
    if (isSlowMode)
    {
        Bluefruit.Periph.setConnInterval(CONN_INTERVAL_FAST_MIN, CONN_INTERVAL_FAST_MAX);
        isSlowMode = false;
    }
    lastActivity = millis();
}

// Switches to the slow connection interval if there's been no activity for IDLE_TIMEOUT_MS
void goSlowIfIdle()
{
    if (!isSlowMode && (millis() - lastActivity > IDLE_TIMEOUT_MS))
    {
        Bluefruit.Periph.setConnInterval(CONN_INTERVAL_SLOW_MIN, CONN_INTERVAL_SLOW_MAX);
        isSlowMode = true;
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

    // Start directly in "fast" mode (nicer usability on first connection)
    Bluefruit.Periph.setConnInterval(CONN_INTERVAL_FAST_MIN, CONN_INTERVAL_FAST_MAX);

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

    lastActivity = millis();
}

void loop()
{
    if (!Bluefruit.connected())
    {
        sd_app_evt_wait();
        return;
    }

    // If there was ISR activity since the last loop, handle it here (BLE-safe context)
    if (activityFlag)
    {
        activityFlag = false;
        goFast();
    }

    if (digitalRead(ENCODER_SW) == LOW)
    {
        uint32_t now = millis();
        if ((now - lastSwPress) > SW_DEBOUNCE_MS)
        {
            lastSwPress = now;
            goFast(); // button press also counts as activity

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

        goFast(); // encoder movement also counts as activity

        uint16_t key = (delta > 0)
                           ? HID_USAGE_CONSUMER_VOLUME_INCREMENT
                           : HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        blehid.consumerKeyPress(key);
        delay(5);
        blehid.consumerKeyRelease();
    }

    // Check whether it's time to switch to the slow (power-saving) mode
    goSlowIfIdle();

    sd_app_evt_wait();
}
