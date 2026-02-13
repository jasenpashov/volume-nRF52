#include <Arduino.h>
#include <Adafruit_TinyUSB.h> // Добави това за поддръжка на USB Serial
#include <bluefruit.h>

// Твоите пинове (Nice!Nano GPIO)
#define ENCODER_CLK 6
#define ENCODER_DT  8
#define ENCODER_SW  17
#define LED_PIN     15

void setup() {
    Serial.begin(115200);
    
    // Важно: При nRF52840 трябва да изчакаме USB порта
    while (!Serial) delay(10); 

    Serial.println("--- ТЕСТ СТАРТИРА ---");
    
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    static int lastClk = HIGH;
    int currentClk = digitalRead(ENCODER_CLK);
    int currentDt = digitalRead(ENCODER_DT);
    int currentSw = digitalRead(ENCODER_SW);

    // 1. Проверка на въртенето
    if (currentClk != lastClk) {
        Serial.print("Въртене! CLK: ");
        Serial.print(currentClk);
        Serial.print(" | DT: ");
        Serial.println(currentDt);
        
        // Премигваме с диода при всяко мърдане
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    lastClk = currentClk;

    // 2. Проверка на бутона
    if (currentSw == LOW) {
        Serial.println(">>> БУТОНЪТ Е НАТИСНАТ! <<<");
        delay(100); // Прост дебаунс
    }

    delay(1); // Малка пауза за стабилност
}

// #include <Arduino.h>
// #include <bluefruit.h>

// // Дефиниране на пиновете
// // Новите пинове за Nice!Nano / Pro Micro формат
// #define ENCODER_CLK 6    // Означен с "6" на платката s1
// #define ENCODER_DT  8    // Означен с "8" на платката s2
// #define ENCODER_SW  17   // Означен с "17" на платката key
// #define LED_PIN     15   // Синият LED на Nice!Nano (P0.15)

// // Функция,Пин на Nice!Nano,GPIO в чипа
// // ENCODER_CLK,P0.06 (написано 6),6
// // ENCODER_DT,P0.08 (написано 8),8
// // ENCODER_SW,P0.17 (написано 17),17
// // LED (Син),P0.15,15

// // Обекти за Bluetooth услугите
// BLEDis bledis;       
// BLEHidAdafruit blehid;    
// BLEBas blebas;       

// int lastClkState;
// unsigned long lastEncoderMove = 0;
// unsigned long pressTime = 0;
// bool buttonHeld = false;
// const unsigned long LONG_PRESS_RESET = 15000;

// // --- ПРОТОТИПИ НА ФУНКЦИИТЕ (Задължително за PlatformIO/C++) ---
// void startAdv(void);
// void connect_callback(uint16_t conn_handle);
// void disconnect_callback(uint16_t conn_handle, uint8_t reason);

// void setup() {
//     Serial.begin(115200);

//     pinMode(LED_PIN, OUTPUT);
//     pinMode(ENCODER_CLK, INPUT_PULLUP);
//     pinMode(ENCODER_DT, INPUT_PULLUP);
//     pinMode(ENCODER_SW, INPUT_PULLUP);

//     lastClkState = digitalRead(ENCODER_CLK);

//     // Инициализация
//     Bluefruit.begin();
//     Bluefruit.setTxPower(4);
//     Bluefruit.setName("nRF52-Volume");
    
//     // Регистрация на калбеците
//     Bluefruit.Periph.setConnectCallback(connect_callback);
//     Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

//     bledis.setManufacturer("Logitech");
//     bledis.setModel("nRF52-HID");
//     bledis.begin();

//     blebas.begin();
//     blebas.write(100);

//     blehid.begin();

//     startAdv();
//     Serial.println("Ready for Windows pairing...");
// }

// void loop() {
//     int swState = digitalRead(ENCODER_SW);

//     // Encoder Logic
//     if (millis() - lastEncoderMove > 10) {
//         int clkState = digitalRead(ENCODER_CLK);
//         if (clkState != lastClkState && clkState == LOW) {
//             if (digitalRead(ENCODER_DT) != clkState) {
//                 blehid.consumerKeyPress(HID_USAGE_CONSUMER_VOLUME_INCREMENT);
//                 blehid.consumerKeyRelease();
//             } else {
//                 blehid.consumerKeyPress(HID_USAGE_CONSUMER_VOLUME_DECREMENT);
//                 blehid.consumerKeyRelease();
//             }
//             lastEncoderMove = millis();
//         }
//         lastClkState = clkState;
//     }

//     // Button Logic
//     if (swState == LOW && !buttonHeld) {
//         pressTime = millis();
//         buttonHeld = true;
//     } 
//     else if (swState == HIGH && buttonHeld) {
//         unsigned long held = millis() - pressTime;
//         if (held >= LONG_PRESS_RESET) {
//             Bluefruit.Periph.clearBonds();
//             NVIC_SystemReset();
//         } else if (held >= 800) {
//             blehid.consumerKeyPress(HID_USAGE_CONSUMER_PLAY_PAUSE);
//             blehid.consumerKeyRelease();
//         } else {
//             blehid.consumerKeyPress(HID_USAGE_CONSUMER_MUTE);
//             blehid.consumerKeyRelease();
//         }
//         buttonHeld = false;
//     }
// }

// // --- ДЕФИНИЦИИ НА ФУНКЦИИТЕ ---
// void startAdv(void) {
//     Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
//     Bluefruit.Advertising.addAppearance(0x03C3);
//     Bluefruit.Advertising.addService(blehid);
//     Bluefruit.Advertising.addService(blebas);
//     Bluefruit.ScanResponse.addName();
//     Bluefruit.Advertising.restartOnDisconnect(true);
//     Bluefruit.Advertising.setInterval(32, 244);
//     Bluefruit.Advertising.start(0);
// }

// void connect_callback(uint16_t conn_handle) {
//     digitalWrite(LED_PIN, HIGH);
//     Serial.println("Connected!");
// }

// void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
//     digitalWrite(LED_PIN, LOW);
//     Serial.print("Disconnected, reason: 0x");
//     Serial.println(reason, HEX);
// }