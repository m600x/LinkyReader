#include "EDF_Teleinfo.h"

/*
  Generate the bargraph
*/
void displayGraph(EDFTempo &data, int x, int y) {
    int graph_height = 70;
    int graph_wide = 120;
    data.img.fillSmoothRoundRect(x, y, graph_wide + 20, graph_height + 20, 4, TFT_DARKCYAN);
    data.img.fillRect(x + 6, y + 6, graph_wide + 8, graph_height + 8, TFT_BLACK);
    for (int i = 0 ; i < 120 ; i++) {
        if (data.graph[i] != 0) {
            int bar_length =  1;
            if (data.graph_min < data.graph_max)
                bar_length = map(data.graph[i], data.graph_min, data.graph_max, 0, graph_height);
            data.img.drawLine(x + 10 + i,
                         y + 10 + graph_height - bar_length,
                         x + 10 + i,
                         y + 10 + graph_height,
                         TFT_RED);
        }
    }
}

/*
  Generate a small container for a data with a front header
*/
void displayElement(EDFTempo &data, String id, String value, int x, int y, int length, uint32_t color) {
    data.img.loadFont(AA_FONT_SMALL);
    
    int header_space = 20;
    data.img.fillSmoothRoundRect(x, y, length + header_space, 20, 4, color);
    data.img.fillSmoothRoundRect(x + 2, y + 2, 28, 16, 4, TFT_BLACK);

    data.img.setTextDatum(TC_DATUM);
    data.img.setTextColor(TFT_LIGHTGREY);
    data.img.drawString(id, x + 16, y + 3);

    data.img.setTextDatum(TL_DATUM);
    data.img.setTextColor(TFT_BLACK);
    data.img.drawString(value, x + 32, y + 3);
}

/*
  Main screen generator
*/
void displayMain(EDFTempo &data, String timestamp) {
    data.img.fillRect(0, 0, 320, 170, TFT_BLACK);
    displayElement(data, "T", timestamp, -30, 0, 170, TFT_DARKGREEN);
    displayElement(data, "IP", WiFi.localIP().toString(), 170, 0, 120, TFT_DARKCYAN);
    displayElement(data, "HC", String(data.blue_hc.value / 1000) + "Kw",  0,   25, 85, TFT_BLUE);
    displayElement(data, "HC", String(data.white_hc.value / 1000) + "Kw", 108, 25, 85, TFT_LIGHTGREY);
    displayElement(data, "HC", String(data.red_hc.value / 1000) + "Kw",   216, 25, 85, TFT_RED);
    displayElement(data, "HP", String(data.blue_hp.value / 1000) + "Kw",  0,   50, 85, TFT_BLUE);
    displayElement(data, "HP", String(data.white_hp.value / 1000) + "Kw", 108, 50, 85, TFT_LIGHTGREY);
    displayElement(data, "HP", String(data.red_hp.value / 1000) + "Kw",   216, 50, 85, TFT_RED);
    data.img.setTextColor(TFT_DARKGREY);
    data.img.setTextDatum(TC_DATUM);
    data.img.drawString("CONSO INSTANTANEE", 85, 75);
    data.img.drawString(data.cost_name[data.cost_index.value] + ": " + String(data.instant_cost.value) + "e/Kw", 85, 125);
    data.img.setTextColor(TFT_RED);
    data.img.loadFont(AA_FONT_LARGE);
    data.img.drawString(String(data.power.value) + "W", 85, 90);
    displayElement(data, "-", String(data.graph_min), 0, 145, 60, TFT_DARKGREY);
    displayElement(data, "+", String(data.graph_max), 90, 145, 60, TFT_DARKGREY);
    displayGraph(data, 180, 75);
}

/*
  Wait screen for a Wifi connection
*/
void displayWifi(EDFTempo &data) {
    data.img.fillRect(0, 0, 320, 170, TFT_BLACK);

    data.img.setTextDatum(TL_DATUM);
    data.img.setTextColor(TFT_DARKGREY);
    data.img.loadFont(AA_FONT_LARGE);
    data.img.drawString("WIFI", 140, 20);

    int status = data.wifi_connection % 4;
    if (status >= 2 || status == 0)
        data.img.drawSmoothArc(95, 52, 20, 15, 135, 225, TFT_DARKGREY, TFT_BLACK, true);
    if (status == 3 || status == 0)
        data.img.drawSmoothArc(95, 52, 30, 25, 135, 225, TFT_DARKGREY, TFT_BLACK, true);
    if (status == 0)
        data.img.drawSmoothArc(95, 52, 40, 35, 135, 225, TFT_DARKGREY, TFT_BLACK, true);
    data.img.fillSmoothCircle(95, 52, 7, TFT_DARKGREY, TFT_BLACK);

    data.img.setTextDatum(TC_DATUM);
    data.img.setTextColor(TFT_DARKGREY);
    data.img.loadFont(AA_FONT_LARGE);
    data.img.drawString(SSID, 160, 70);
    data.img.drawString(PASS, 160, 110);

    displayElement(data, "UP", millisToTime(), 0, 150, 90, TFT_PURPLE);
    displayElement(data, "T", String(data.wifi_connection / 2) + " s.", 210, 150, 90, TFT_DARKCYAN);
}

/*
  Wait screen for an MQTT connection
*/
void displayMQTT(EDFTempo &data) {
    data.img.fillRect(0, 0, 320, 170, TFT_BLACK);

    data.img.setTextDatum(TL_DATUM);
    data.img.setTextColor(TFT_DARKGREY);
    data.img.loadFont(AA_FONT_LARGE);
    data.img.drawString("MQTT", 140, 20);

    data.img.fillSmoothRoundRect(70, 10, 50, 50, 2, TFT_PURPLE, TFT_BLACK);
    data.img.drawSmoothArc(70, 60, 23, 15, 180, 270, TFT_BLACK, TFT_BLACK, false);
    data.img.drawSmoothArc(70, 60, 43, 35, 180, 270, TFT_BLACK, TFT_BLACK, false);
    data.img.drawSmoothArc(70, 60, 63, 55, 180, 270, TFT_BLACK, TFT_BLACK, false);

    data.img.setTextDatum(TC_DATUM);
    data.img.setTextColor(TFT_DARKGREY);
    data.img.loadFont(AA_FONT_LARGE);
    data.img.drawString(MQTT_SERVER, 160, 70);
    data.img.drawString(String(MQTT_PORT), 160, 110);

    displayElement(data, "UP", millisToTime(), 0, 150, 90, TFT_PURPLE);
    displayElement(data, "OK", "WIFI", 123, 150, 55, TFT_GREEN);
    displayElement(data, "T", String(data.mqtt_connection) + " s.", 210, 150, 90, TFT_DARKCYAN);
}