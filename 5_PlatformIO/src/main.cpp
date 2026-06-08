// Add library
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h> // https://github.com/me-no-dev/AsyncTCP.git
#include <ESPAsyncWebServer.h> //  https://github.com/me-no-dev/ESPAsyncWebServer.git
#include <LittleFS.h> // file management library
#include <myADS1115.h> // file my custom
//---------------------------

// Wi-Fi configuration
const char* ssid = "tiendai@@";
const char* password = "dai123";
//------------------------------

// WheatStone Bridge param
const float P = 62000;
const float R = 10000;
const float Q = 30;
//------------------------------

// Initial
MyADS1115 ads; 
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
//------------------------------

// Websocket processing
void onWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
void *arg, uint8_t *data, size_t len){
    if(type == WS_EVT_CONNECT){
        Serial.printf("Web connected (ID: %u)\n", client->id());
    }
}
//------------------------------

void setup() {
    Serial.begin(115200);
    // Init system file LittleFS
    if(!LittleFS.begin(true)){
        Serial.println("Error LittleFS");
        return;
    }

    // Init ADC
    if(!ads.begin(21, 22)){
        Serial.println("Error ADS1115");
        while(1);
    }

    // Connect WiFi
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nIP: " + WiFi.localIP().toString());

    // Init WebSocket
    ws.onEvent(onWSEvent);
    server.addHandler(&ws);

    // Give file web from LittleFS
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        // open file ResMeas_InterfaceWeb and give
        request->send(LittleFS, "/ResMeas_InterfaceWeb.html", "text/html");
    });
    server.begin();
}

void loop() {
    static unsigned long prevTime = 0;
    if (millis() - prevTime > 350){
        float Eb = ads.readVoltage_Source();
        float Vdiff = ads.getStableVoltage(10);

         // Calculate Rx function
        float K = R/(R+P);
        float M = K - (Vdiff/Eb);
        float Rx = 0;
        // Check M
        if (M > 0 && M < 1) {
            Rx = (M*Q)/(1-M);
        }
        if (Rx < 0) Rx=0;
        // push value on web
        ws.textAll(String(Rx,3));
        prevTime = millis();
    }
    ws.cleanupClients();
}