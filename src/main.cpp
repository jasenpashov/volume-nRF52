#include <Arduino.h>
#include <bluefruit.h>

// Дефиниране на пиновете
// #define ENCODER_CLK  47 // P1.15 -> (1 * 32) + 15 = 47
// #define ENCODER_DT   2  // P0.02 -> (0 * 32) + 2  = 2
// #define ENCODER_SW   29 // P0.29 -> (0 * 32) + 29 = 29
// #define LED_PIN      15 // P0.15 -> (0 * 32) + 15 = 15

// Дефиниране чрез директни GPIO портове
#define ENCODER_CLK   NRF_GPIO_PIN_MAP(1, 15)  // P1.15
#define ENCODER_DT    NRF_GPIO_PIN_MAP(0, 2)   // P0.02
#define ENCODER_SW    NRF_GPIO_PIN_MAP(0, 29)  // P0.29
#define LED_PIN       NRF_GPIO_PIN_MAP(0, 15)  // P0.15

// Обекти за Bluetooth услугите
BLEDis bledis;       
BLEHidAdafruit blehid;    
BLEBas blebas;       

int lastClkState;
unsigned long lastEncoderMove = 0;
unsigned long pressTime = 0;
bool buttonHeld = false;
const unsigned long LONG_PRESS_RESET = 15000;

// --- ПРОТОТИПИ НА ФУНКЦИИТЕ (Задължително за PlatformIO/C++) ---
void startAdv(void);
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);

void setup() {
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    lastClkState = digitalRead(ENCODER_CLK);

    // Инициализация
    Bluefruit.begin();
    Bluefruit.setTxPower(4);
    Bluefruit.setName("nRF52-Volume");
    
    // Регистрация на калбеците
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

void loop() {
    int swState = digitalRead(ENCODER_SW);

    // Encoder Logic
    if (millis() - lastEncoderMove > 10) {
        int clkState = digitalRead(ENCODER_CLK);
        if (clkState != lastClkState && clkState == LOW) {
            if (digitalRead(ENCODER_DT) != clkState) {
                blehid.consumerKeyPress(HID_USAGE_CONSUMER_VOLUME_INCREMENT);
                blehid.consumerKeyRelease();
            } else {
                blehid.consumerKeyPress(HID_USAGE_CONSUMER_VOLUME_DECREMENT);
                blehid.consumerKeyRelease();
            }
            lastEncoderMove = millis();
        }
        lastClkState = clkState;
    }

    // Button Logic
    if (swState == LOW && !buttonHeld) {
        pressTime = millis();
        buttonHeld = true;
    } 
    else if (swState == HIGH && buttonHeld) {
        unsigned long held = millis() - pressTime;
        if (held >= LONG_PRESS_RESET) {
            Bluefruit.Periph.clearBonds();
            NVIC_SystemReset();
        } else if (held >= 800) {
            blehid.consumerKeyPress(HID_USAGE_CONSUMER_PLAY_PAUSE);
            blehid.consumerKeyRelease();
        } else {
            blehid.consumerKeyPress(HID_USAGE_CONSUMER_MUTE);
            blehid.consumerKeyRelease();
        }
        buttonHeld = false;
    }
}

// --- ДЕФИНИЦИИ НА ФУНКЦИИТЕ ---
void startAdv(void) {
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addAppearance(0x03C3);
    Bluefruit.Advertising.addService(blehid);
    Bluefruit.Advertising.addService(blebas);
    Bluefruit.ScanResponse.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);
    Bluefruit.Advertising.start(0);
}

void connect_callback(uint16_t conn_handle) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Connected!");
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    digitalWrite(LED_PIN, LOW);
    Serial.print("Disconnected, reason: 0x");
    Serial.println(reason, HEX);
}