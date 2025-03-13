#include <WiFi.h>
#include <ArduinoWebsockets.h>

#define INPUTPIN  (15)  
#define OUTPIN    (16)  

const char* ssid = "VIP";                   
const char* password = "Bulka@!1234#2023";  
const char* websocket_server = "ws://172.25.24.220:8080/car_washing";

using namespace websockets;
WebsocketsClient client;

// Asosiy parametrlar
int min_low_pulse = 45;
int max_low_pulse = 55;
unsigned long timeout = 500;
int low_duration = 50;
int high_duration = 100;
int washId = 9;
int terminalId = 9;

// Qo'shimcha parametrlar
unsigned long pulse_begin = 0;
unsigned long last_pulse_time = 0;
int pulse_state = 0, last_state = 1;
int pulse_count = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastWebSocketCheck = 0;

void connectToWiFi() {
    Serial.print("Wi-Fi ulanmoqda...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" ✅ Ulandi!");
}

void connectToWebSocket() {
    Serial.print("WebSocketga ulanmoqda...");
    if (client.connect(websocket_server)) {
        Serial.println(" ✅ Ulandi!");
    } else {
        Serial.println(" ❌ Muvaffaqiyatsiz!");
    }
}

void checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠️ Wi-Fi uzildi, qayta ulanmoqda...");
        WiFi.disconnect();
        WiFi.reconnect();
        connectToWiFi();
        connectToWebSocket();
    }
}

void checkWebSocketConnection() {
    if (!client.available()) {
        Serial.println("⚠️ WebSocket uzildi, qayta ulanmoqda...");
        connectToWebSocket();
    }
}

void handleWebSocketMessage(WebsocketsMessage message) {
    String msg = message.data();
    msg.trim();  // Qator boshidagi va oxiridagi bo'sh joylarni olib tashlash
    Serial.print("🔹 WebSocket xabari: ");
    Serial.println(msg);
    int washIdmssg;
    int terminalIdmssg;

    // washId ni olish
    int washPos = msg.indexOf("\"washId\":");
    if (washPos != -1) {
        washPos += 9; // "washId": so‘zidan keyingi joyni olamiz
        int washEnd = msg.indexOf(",", washPos);
        washIdmssg = msg.substring(washPos, washEnd).toInt();
    }

    // terminalId ni olish
    int terminalPos = msg.indexOf("\"terminalId\":");
    if (terminalPos != -1) {
        terminalPos += 13; // "terminalId": so‘zidan keyingi joyni olamiz
        int terminalEnd = msg.indexOf(",", terminalPos);
        terminalIdmssg = msg.substring(terminalPos, terminalEnd).toInt();
    }

    if (washId == washIdmssg  && terminalId == terminalIdmssg) {
        int amountStart = msg.indexOf("\"amount\":") + 9; // 9 ta belgi "amount": ni o'tkazib yuborish uchun
        int amountEnd = msg.indexOf("}", amountStart);

        if (amountStart != -1 && amountEnd != -1) {  
            String amountStr = msg.substring(amountStart, amountEnd);
            amountStr.trim();  // Qo'shimcha bo'sh joylarni olib tashlash
            int amount = amountStr.toInt();  // Stringdan integerga o'tkazish
            int impulses = amount / 1000;

            Serial.print("💰 Qabul qilingan amount: ");
            Serial.println(amount);
            for (int i = 0; i < impulses; i++) {
                digitalWrite(OUTPIN, HIGH);
                delay(50);
                digitalWrite(OUTPIN, LOW);
                delay(100);
            }
        } else {
            Serial.println("⚠️ amount maydoni topilmadi!");
        }
    }
}



void setup() {
    Serial.begin(115200);
    delay(2000);
    
    pinMode(INPUTPIN, INPUT_PULLUP);
    pinMode(OUTPIN, OUTPUT);
    digitalWrite(OUTPIN, LOW);

    connectToWiFi();
    connectToWebSocket();
    client.onMessage(handleWebSocketMessage);
}

void loop() {
    unsigned long curtime = millis();
    if (curtime - lastWiFiCheck > 10000) {
        lastWiFiCheck = curtime;
        checkWiFiConnection();
    }
    if (curtime - lastWebSocketCheck > 5000) {
        lastWebSocketCheck = curtime;
        checkWebSocketConnection();
    }

    pulse_state = digitalRead(INPUTPIN);
    
    if (pulse_state == LOW && last_state == HIGH) {
        pulse_begin = curtime;
        last_state = LOW;
    } else if (pulse_state == HIGH && last_state == LOW) {
        unsigned long duration = curtime - pulse_begin;
        if (duration >= min_low_pulse && duration <= max_low_pulse) {
            pulse_count++; 
            last_pulse_time = curtime;
        }
        last_state = HIGH;
    }

    if (pulse_count > 0 && (curtime - last_pulse_time > timeout)) {
        int money = pulse_count * 1000;
        String jsonData = "{\"action\": \"cash\",\"washId\": 15, \"terminalId\": 150, \"amount\": " + String(money) + "}";

        Serial.print("JSON yuborilmoqda: ");
        Serial.println(jsonData);
        
        for (int i = 0; i < pulse_count; i++) {
            digitalWrite(OUTPIN, HIGH);
            delay(low_duration);
            digitalWrite(OUTPIN, LOW);
            delay(high_duration);
        }
        
        if (client.available()) {
            client.send(jsonData);
            Serial.println("✅ JSON WebSocketga yuborildi!");
        } else {
            Serial.println("❌ WebSocket uzildi!");
        }
        pulse_count = 0;
    }

    if (client.available()) {
        client.poll();
    } 
}
