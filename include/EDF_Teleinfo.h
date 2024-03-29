#ifndef EDF_TELEINFO
#define EDF_TELEINFO

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "TFT_eSPI.h"
#include "WiFi.h"
#include "RTClib.h"
#include <WiFiUdp.h>
#include <ESP32Time.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include "RemoteDebug.h"
#include "NotoSansBold15.h"
#include "NotoSansBold36.h"

#define SSID                     "REPLACEME"                             // (Mandatory) Wifi SSID
#define PASS                     "REPLACEME"                             // (Mandatory) Wifi password
#define IDENTIFIER               "EDF_Teleinfo"                          // Used for MQTT ID, OTA and remote debugger
#define TIMER_GRAPH_UPDATE       30                                      // Time in seconds before refreshing the LCD bargraph

#define MQTT_ENABLED             false                                   // (Mandatory) MQTT feature flag, set to true to enable
#define MQTT_SERVER              "REPLACEME"                             // (Optional)  IP of the MQTT server
#define MQTT_PORT                1883                                    // (Optional)  Port of the MQTT server
#define TIMER_MQTT_UPDATE        30                                      // Time in seconds between each MQTT message if active (120 bars so 120*30 = 1hour)
#define MQTT_TOPIC_BLUE_HC       "edf/consumption/blue/hc"               // MQTT topic to send the counter of watts of blue HC
#define MQTT_TOPIC_WHITE_HC      "edf/consumption/white/hc"              // MQTT topic to send the counter of watts of white HC
#define MQTT_TOPIC_RED_HC        "edf/consumption/red/hc"                // MQTT topic to send the counter of watts of red HC
#define MQTT_TOPIC_BLUE_HP       "edf/consumption/blue/hp"               // MQTT topic to send the counter of watts of blue HP
#define MQTT_TOPIC_WHITE_HP      "edf/consumption/white/hp"              // MQTT topic to send the counter of watts of white HP
#define MQTT_TOPIC_RED_HP        "edf/consumption/red/hp"                // MQTT topic to send the counter of watts of red HP
#define MQTT_TOPIC_PAPP          "edf/consumption/instant/power"         // MQTT topic to send the Instant VA
#define MQTT_TOPIC_POWER         "edf/consumption/instant/power_watts"   // MQTT topic to send the value from Intensity * voltage (239)
#define MQTT_TOPIC_IINST         "edf/consumption/instant/intensity"     // MQTT topic to send the Instant intensity
#define MQTT_TOPIC_IMAX          "edf/consumption/instant/intensity_max" // MQTT topic to send the Max intensity
#define MQTT_TOPIC_COST_INDEX    "edf/consumption/instant/index_cost"    // MQTT topic to send the Index cost
#define MQTT_TOPIC_COST_CURRENT  "edf/consumption/instant/current_cost"  // MQTT topic to send the value from cost[cost_index] (kWh price)
#define MQTT_TOPIC_COST_POWER    "edf/consumption/instant/power_cost"    // MQTT topic to send the value from power * current_cost

#define PIN_BTN_TOP              14                                      // Leave as it for the T-Display S3
#define PIN_BTN_BOTTOM           0                                       // Leave as it for the T-Display S3
#define PIN_POWER_ON             15                                      // Leave as it for the T-Display S3
#define PIN_LCD_BL               38                                      // Leave as it for the T-Display S3
#define PIN_RXD2                 1                                       // Pin on the T-Display S3 where we connect the RX of the Linky
#define PIN_TXD2                 16                                      // (Unused) Pin on the T-Display S3 where we connect the TX of the Linky
#define LCD_BRIGHTNESS           120                                     // Ranging from 0 to 255, LCD brightness

#define AA_FONT_SMALL NotoSansBold15
#define AA_FONT_LARGE NotoSansBold36

struct TempoInt {
    String name;
    String index;
    String topic;
    int value;
    int value_tmp;
};

struct TempoFloat {
    String name;
    String index;
    String topic;
    float value;
    float value_tmp;
};

struct EDFTempo {
    TFT_eSPI tft = TFT_eSPI();
    TFT_eSprite img = TFT_eSprite(&tft);
    TempoInt blue_hc =        {"Bleu HC",        "BBRHCJB",  MQTT_TOPIC_BLUE_HC,       0, 0}; // READING  | Counter of watts in blue HC
    TempoInt white_hc =       {"Blanc HC",       "BBRHCJW",  MQTT_TOPIC_WHITE_HC,      0, 0}; // READING  | Counter of watts in white HC
    TempoInt red_hc =         {"Rouge HC",       "BBRHCJR",  MQTT_TOPIC_RED_HC,        0, 0}; // READING  | Counter of watts in red HC
    TempoInt blue_hp =        {"Bleu HP",        "BBRHPJB",  MQTT_TOPIC_BLUE_HP,       0, 0}; // READING  | Counter of watts in blue HP
    TempoInt white_hp =       {"Blanc HP",       "BBRHPJW",  MQTT_TOPIC_WHITE_HP,      0, 0}; // READING  | Counter of watts in white HP
    TempoInt red_hp =         {"Rouge HP",       "BBRHPJR",  MQTT_TOPIC_RED_HP,        0, 0}; // READING  | Counter of watts in red HP
    TempoInt power =          {"Power",          "PAPP",     MQTT_TOPIC_PAPP,          0, 0}; // READING  | Instant VA
    TempoInt power_watts =    {"Power Watts",    "",         MQTT_TOPIC_POWER,         0, 0}; // COMPUTED | value from Intensity * voltage (239)
    TempoInt intensity =      {"Intensity",      "IINST",    MQTT_TOPIC_IINST,         0, 0}; // READING  | Instant intensity
    TempoInt intensity_max =  {"Intensity max",  "IMAX",     MQTT_TOPIC_IMAX,          0, 0}; // READING  | Max intensity
    TempoInt cost_index =     {"Cost index",     "PTEC",     MQTT_TOPIC_COST_INDEX,    0, 0}; // READING  | Index for the cost[6] value
    TempoFloat current_cost = {"Current cost",   "",         MQTT_TOPIC_COST_CURRENT,  0, 0}; // COMPUTED | Value from cost[cost_index] (kWh price)
    TempoFloat instant_cost = {"Instant cost",   "",         MQTT_TOPIC_COST_POWER,    0, 0}; // COMPUTED | value from power * current_cost
    int graph[120];
    int graph_max = 0;
    int graph_min = 0;
    String cost_name[6] = {"BLEU HC", "BLANC HC", "ROUGE HC", "BLEU HP", "BLANC HP", "ROUGE HP"};
    float cost[6] = {0.1056, 0.1246, 0.1328, 0.1369, 0.1654, 0.7324};
    int wifi_connection = 0;
    int mqtt_connection = 0;
};

String millisToTime();
void logger(String msg);

void displayMain(EDFTempo &data, String timestamp);
void displayWifi(EDFTempo &data);
void displayMQTT(EDFTempo &data);
void linkyReader(EDFTempo &data);

#endif