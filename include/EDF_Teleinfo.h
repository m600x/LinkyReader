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
#define MQTT_ENABLED             false                                   // (Mandatory) MQTT feature flag, set to true to enable
#define MQTT_SERVER              "REPLACEME"                             // (Optional)  IP of the MQTT server
#define MQTT_PORT                1883                                    // (Optional)  Port of the MQTT server
#define IDENTIFIER               "EDF_Teleinfo"                          // Used for MQTT ID, OTA and remote debugger
#define TIMER_GRAPH_UPDATE       30                                      // Time in seconds before refreshing the LCD bargraph
#define TIMER_MQTT_UPDATE        10                                      // Time in seconds between each MQTT message if active
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
#define MQTT_TOPIC_COST_CURRENT  "edf/consumption/instant/current_cost"  // MQTT topic to send the value from cost[power_index] (kWh price)
#define MQTT_TOPIC_COST_POWER    "edf/consumption/instant/power_cost"    // MQTT topic to send the value from power * current_cost
#define PIN_BTN_TOP         14                                           // Leave as it for the T-Display S3
#define PIN_BTN_BOTTOM      0                                            // Leave as it for the T-Display S3
#define PIN_POWER_ON        15                                           // Leave as it for the T-Display S3
#define PIN_LCD_BL          38                                           // Leave as it for the T-Display S3
#define PIN_RXD2            1                                            // Pin on the T-Display S3 where we connect the RX of the Linky
#define PIN_TXD2            16                                           // (Unused) Pin on the T-Display S3 where we connect the TX of the Linky
#define LCD_BRIGHTNESS      120                                          // Ranging from 0 to 255, LCD brightness

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
    TempoInt blue_hc =        {"Bleu HC",        "BBRHCJB",  "edf/consumption/blue/hc",                0, 0}; // READING  | Counter of watts in blue HC
    TempoInt white_hc =       {"Blanc HC",       "BBRHCJW",  "edf/consumption/white/hc",               0, 0}; // READING  | Counter of watts in white HC
    TempoInt red_hc =         {"Rouge HC",       "BBRHCJR",  "edf/consumption/red/hc",                 0, 0}; // READING  | Counter of watts in red HC
    TempoInt blue_hp =        {"Bleu HP",        "BBRHPJB",  "edf/consumption/blue/hp",                0, 0}; // READING  | Counter of watts in blue HP
    TempoInt white_hp =       {"Blanc HP",       "BBRHPJW",  "edf/consumption/white/hp",               0, 0}; // READING  | Counter of watts in white HP
    TempoInt red_hp =         {"Rouge HP",       "BBRHPJR",  "edf/consumption/red/hp",                 0, 0}; // READING  | Counter of watts in red HP
    TempoInt power_index =    {"Power index",    "PTEC",     "",                                       0, 0}; // COMPUTED | Index for the cost[6] value
    TempoInt power =          {"Power",          "PAPP",     "edf/consumption/instant/power",          0, 0}; // READING  | Instant VA
    TempoInt power_watts =    {"Power Watts",    "",         "edf/consumption/instant/power_watts",    0, 0}; // COMPUTED | value from Intensity * voltage (239)
    TempoInt intensity =      {"Intensity",      "IINST",    "edf/consumption/instant/intensity",      0, 0}; // READING  | Instant intensity
    TempoInt intensity_max =  {"Intensity max",  "IMAX",     "edf/consumption/instant/intensity_max",  0, 0}; // READING  | Max intensity
    TempoFloat current_cost = {"Current cost",   "",         "edf/consumption/instant/current_cost",   0, 0}; // COMPUTED | Value from cost[power_index] (kWh price)
    TempoFloat instant_cost = {"Instant cost",   "",         "edf/consumption/instant/power_cost",     0, 0}; // COMPUTED | value from power * current_cost
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