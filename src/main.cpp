#include "EDF_Teleinfo.h"

RemoteDebug Debug;
WiFiClient espClient;
PubSubClient mqtt(espClient);
ESP32Time rtc(0);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

EDFTempo data;

unsigned long display_timer = millis();
unsigned long mqtt_timer = millis();

/*
  Generate a HH:MM:SS string time from millis since ESP32 boot to get the uptime.
*/
String millisToTime() {
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    return (hours % 24 < 10 ? "0" : "") + String(hours % 24, DEC) + ":" +
           (minutes % 60 < 10 ? "0" : "") + String(minutes % 60, DEC) + ":" +
           (seconds % 60 < 10 ? "0" : "") + String(seconds % 60, DEC);
}

/*
  Helper to log in the Serial and Remote debugger at the same time.
*/
void logger(String msg) {
    String payload = "T " + rtc.getTime("%F %T") + " | U " + millisToTime() + " | C" + String(xPortGetCoreID()) + " | " + msg;
    Serial.println(payload);
    Debug.println(payload);
}

/*
  Set the internal RTC with NTP to keep accurate current time.
*/
void updateRTC() {
    logger("RTC  | Start update");
    timeClient.update();
    time_t rawtime = timeClient.getEpochTime();
    logger("RTC  | Received: " + timeClient.getFormattedTime() + " " + timeClient.getDay());
    rtc.setTime(timeClient.getEpochTime());
    rtc.getTime("%F %T");
    logger("RTC  | End update");
}

/*
  Keep the int array of the bargraph up to date.
*/
void updateGraph() {
    if (data.power.value != 0) {
        int new_min = 0;
        int new_max = 0;
        for (int i = 119 ; i > 0 ; i--) {
            data.graph[i] = data.graph[i - 1];
            if (data.graph[i] >= new_max)
                new_max = data.graph[i];
            if (data.graph[i] != 0 && data.graph[i] <= new_min)
                new_min = data.graph[i];
            if (data.graph[i] != 0 && new_min == 0)
                new_min = data.graph[i];
        }
        data.graph[0] = data.power.value;
        data.graph_min = new_min;
        data.graph_max = new_max;
        display_timer = millis();
    }
}

/*
  Helper to connect to the MQTT server, looping until connected.
*/
void mqttConnection() {
    if (WiFi.status() == WL_CONNECTED) {
        logger("MQTT | Wifi connection found, attempting MQTT connection...");
        mqtt.setServer(MQTT_SERVER, MQTT_PORT);
        if (mqtt.connect(IDENTIFIER)) {
            logger("MQTT | Connected!");
            data.mqtt_connection = 0;
        } else {
            logger("MQTT | Failed, rc=" + mqtt.state());
            data.mqtt_connection++;
            mqttConnection();
        }
    }
    else {
        logger("MQTT | Wifi connection not found, aborting MQTT connection.");
    }
}

/*
  MQTT sender for float. Does not send if the value is zero.
*/
void mqttPublisherSingle(TempoFloat &item) {
    if (item.value != 0) {
        logger("MQTT | Publishing for [" + item.name + "] on topic [" + item.topic + "] value [" + String(item.value, 4) + "]");
        mqtt.publish(item.topic.c_str(), String(item.value, 4).c_str(), true);
    }
    else {
        logger("MQTT | Aborting publishing for [" + item.name + "] on topic [" + item.topic + "] value [" + String(item.value, 4) + "]");
    }
}

/*
  MQTT sender for integer. Does not send if the value is zero.
*/
void mqttPublisherSingle(TempoInt &item) {
    if (item.value != 0) {
        logger("MQTT | Publishing for [" + item.name + "] on topic [" + item.topic + "] value [" + String(item.value) + "]");
        mqtt.publish(item.topic.c_str(), String(item.value).c_str(), true);
    }
    else {
        logger("MQTT | Aborting publishing for [" + item.name + "] on topic [" + item.topic + "] value [" + String(item.value) + "]");
    }
}

/*
  MQTT main process loop that will iterate over each item and send it
  to the designated topic.
*/
void mqttPublisher() {
    logger("MQTT | Start mqttPublisher");
    if (!mqtt.connected())
        mqttConnection();
    mqttPublisherSingle(data.blue_hc);
    mqttPublisherSingle(data.blue_hp);
    mqttPublisherSingle(data.white_hc);
    mqttPublisherSingle(data.white_hp);
    mqttPublisherSingle(data.red_hc);
    mqttPublisherSingle(data.red_hp);
    mqttPublisherSingle(data.power);
    mqttPublisherSingle(data.power_watts);
    mqttPublisherSingle(data.intensity);
    mqttPublisherSingle(data.intensity_max);
    mqttPublisherSingle(data.instant_cost);
    mqttPublisherSingle(data.current_cost);
    mqtt_timer = millis();
    logger("MQTT | End mqttPublisher");
}

/*
  Helper to connect to the wifi network, looping until connected.
*/
void wifi_connection(boolean first=false) {
    logger("WIFI | Start wifi_connection");
    if (first) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(SSID, PASS);
    }
    else {
        WiFi.reconnect();
    }
    for (int cycle = 1; cycle <= 100; cycle++) {
        if (WiFi.status() == WL_CONNECTED)
            break;
        if (cycle % 5 == 0) {
            data.wifi_connection++;
        }
        delay(100);
    }
    if (WiFi.status() != WL_CONNECTED) {
        logger("WIFI | Wifi is still not connected, looping back");
        wifi_connection(false);
    }
    else {
        logger("WIFI | Connected!");
    }
}

/*
  Required for the over the air update functionality.
*/
void initOTA() {
    ArduinoOTA.setHostname(IDENTIFIER);
    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else
                type = "filesystem";
            logger("OTA  | Start Updating " + type);
        })
        .onEnd([]() {
            logger("OTA  | End");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Debug.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            Debug.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
                logger("OTA  | Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                logger("OTA  | Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                logger("OTA  | Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                logger("OTA  | Receive Failed");
            else if (error == OTA_END_ERROR)
                logger("OTA  | End Failed");
        });
    ArduinoOTA.begin();
}

/*
  Loop on separated core the the reader function.
*/
void loop_linky(void *pvParameters) {
    while (true) {
        linkyReader(data);
        delay(1);
    }
}

/*
  Loop on separated core for the display processing.
*/
void loop_display(void *pvParameters) {
    for (int x = 0 ; x < 120 ; x++) {
        data.graph[x] = 0;
    }
    data.tft.begin();
    data.tft.setRotation(1);
    data.tft.fillScreen(TFT_BLACK);
    data.img.createSprite(320, 170);
    while(true) {
        if (WiFi.status() != WL_CONNECTED) {
            displayWifi(data);
        }
        else if (!mqtt.connected() && data.mqtt_connection > 0) {
            displayMQTT(data);
        }
        else {
            displayMain(data, rtc.getTime("%F %T"));
        }
        data.img.pushSprite(0, 0);
        delay(200);
    }
}

/*
  Init of the ESP32
*/
void setup() {
    Serial.begin(115200);
    Serial.println("Booting");
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);
    pinMode(PIN_BTN_TOP, INPUT);
    pinMode(PIN_BTN_BOTTOM, INPUT);
    ledcSetup(0, 5000, 8);
    ledcAttachPin(PIN_LCD_BL, 0);
    ledcWrite(0, LCD_BRIGHTNESS);
    xTaskCreatePinnedToCore(loop_linky, "loop_linky", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(loop_display, "loop_display", 4096, NULL, 1, NULL, 1);
    wifi_connection(true);
    initOTA();
    Debug.begin(IDENTIFIER);
    Debug.setResetCmdEnabled(true);
    updateRTC();
    Serial2.begin(1200, SERIAL_7E1, PIN_RXD2, PIN_TXD2);
}

/*
  Main loop that keep:
  - the wifi connected
  - Schedule the bargraph data update
  - Schedule MQTT publishing
*/
void loop() {
    ArduinoOTA.handle();
    Debug.handle();
    if (WiFi.status() != WL_CONNECTED)
        wifi_connection();
    if (millis() > display_timer + (TIMER_GRAPH_UPDATE * 1000))
        updateGraph();
    if (MQTT_ENABLED && millis() > mqtt_timer + (TIMER_MQTT_UPDATE * 1000))
        mqttPublisher();
    delay(1);
}
