#include "EDF_Teleinfo.h"

char DataRawLinky[1000];
char BufferLinky[30];
int IdxDataRawLinky = 0;
int IdxBufferLinky = 0;
bool LFon = false;
int power_tmp = 0;

/*
  Check if item.value_tmp is the same as new_value to save in value.
  Two consecutive value must be read from Linky to be saved, avoiding wrong reading.
  Optional bypass to disable this behavior. Used in watt counter since they change rapidly.

  Return: true if changed
*/
bool linkySaveCheck(TempoInt &item, int new_value, bool bypass=false) {
    if (bypass)
        item.value_tmp = new_value;
    if (item.value_tmp == 0) {
        item.value_tmp = new_value;
    }
    else {
        if (item.value_tmp == new_value && item.value != new_value) {
            logger("LNKY | Value of [" + item.name + "] has changed from [" + String(item.value) + "] to [" + String(new_value) + "]");
            item.value = new_value;
        }
        item.value_tmp = 0;
        return true;
    }
    return false;
}

/*
  Process data incoming from the reader. Check for the code to match a data that is wanted.
  If the code match it, check or directly save the value to the data structure.
*/
void linkySaver(EDFTempo &data, String code, String value) {
    if (code == data.blue_hc.index && value.length() == 9)
        linkySaveCheck(data.blue_hc, value.toInt(), true);
    if (code == data.blue_hp.index && value.length() == 9)
        linkySaveCheck(data.blue_hp, value.toInt(), true);
    if (code == data.white_hc.index && value.length() == 9)
        linkySaveCheck(data.white_hc, value.toInt(), true);
    if (code == data.white_hp.index && value.length() == 9)
        linkySaveCheck(data.white_hp, value.toInt(), true);
    if (code == data.red_hc.index && value.length() == 9)
        linkySaveCheck(data.red_hc, value.toInt(), true);
    if (code == data.red_hp.index && value.length() == 9)
        linkySaveCheck(data.red_hp, value.toInt(), true);
    if (code == data.power_index.index && value.length() == 4) {
        if (value[3] == 'B' && value[1] == 'C')
            data.power_index.value = 0;
        if (value[3] == 'W' && value[1] == 'C')
            data.power_index.value = 1;
        if (value[3] == 'R' && value[1] == 'C')
            data.power_index.value = 2;
        if (value[3] == 'B' && value[1] == 'P')
            data.power_index.value = 3;
        if (value[3] == 'W' && value[1] == 'P')
            data.power_index.value = 4;
        if (value[3] == 'R' && value[1] == 'P')
            data.power_index.value = 5;
        data.current_cost.value = data.cost[data.power_index.value];
    }
    if (code == data.power.index && (value.length() == 4 || value.length() == 5)) {
        if (linkySaveCheck(data.power, value.toInt())) {
            float new_value = (float(data.power.value) / 1000) * data.current_cost.value;
            logger("LNKY | Value of [" + data.instant_cost.name + "] has changed from [" + String(data.instant_cost.value) + "] to [" + String(new_value) + "]");
            data.instant_cost.value = new_value;
        }
    }
    if (code == data.intensity.index && value.length() == 3) {
        if (linkySaveCheck(data.intensity, value.toInt())) {
            int new_value = value.toInt() * 239;
            logger("LNKY | Value of [" + data.power_watts.name + "] has changed from [" + String(data.power_watts.value) + "] to [" + String(new_value) + "]");
            data.power_watts.value = new_value;
        }
    }
    if (code == data.intensity_max.index && value.length() == 3) {
            int new_value = value.toInt();
            logger("LNKY | Value of [" + data.intensity_max.name + "] has changed from [" + String(data.intensity_max.value) + "] to [" + String(new_value) + "]");
        linkySaveCheck(data.intensity_max, value.toInt());
    }
}

/*
  Parse chunk of data to check for potential code/value from the serial
*/
void linkyParser(EDFTempo &data) {
    int nb_blanc = 0;
    String code = "";
    String val = "";
    for (int i = 0; i < IdxBufferLinky; i++) {
        if (BufferLinky[i] == ' ') {
            nb_blanc++;
        }
        else {
            if (nb_blanc == 0) {
                code += BufferLinky[i];
            }
            if (nb_blanc == 1) {
                val += BufferLinky[i];
            }
            if (nb_blanc < 2) {
                DataRawLinky[IdxDataRawLinky] = BufferLinky[i];
                IdxDataRawLinky = (IdxDataRawLinky + 1) % 1000;
            }
        }
    }
    DataRawLinky[IdxDataRawLinky] = char(13);
    IdxDataRawLinky = (IdxDataRawLinky + 1) % 1000;
    logger("LNKY | Extracted: code [" + code + "] value [" + val + "] length [" + val.length() + "]");
    linkySaver(data, code, val);
}

/*
  Serial reader of the Linky
*/
void linkyReader(EDFTempo &data) {
    if (Serial2.available() > 0) {
        int V = Serial2.read();
        if (V == 2) {
            for (int i = 0; i < 5; i++) {
                DataRawLinky[IdxDataRawLinky] = '-';
                IdxDataRawLinky = (IdxDataRawLinky + 1) % 1000;
            }
            logger("LNKY | End of transmission detected");
        }
        if (V > 9) {
            switch (V) {
                case 10:
                    LFon = true;
                    IdxBufferLinky = 0;
                    break;
                case 13:
                    if (LFon) {
                        LFon = false;
                        linkyParser(data);
                    }
                    break;
                default:
                    BufferLinky[IdxBufferLinky] = char(V);
                    IdxBufferLinky = (IdxBufferLinky + 1) % 30;
                    break;
            }
        }
    }
}