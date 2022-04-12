//
// Created by Roman Oberenkowski on 21.11.2021.
//

#ifndef LED_CONSTANTS_H
#define LED_CONSTANTS_H


//PARAMETERS
const int LED_COUNT = 36;
const int COLUMNS_COUNT = 96;

//CONSTANTS
#include <cmath>
const int END_PAYLOAD_LENGTH = ceil(LED_COUNT / 2.0 / 8.0);
const int PAYLOAD_LENGTH = 4 + LED_COUNT * 4 + END_PAYLOAD_LENGTH;

const int SHM_KEY = 0x25565; //can be almost anything

#endif //LED_CONSTANTS_H
