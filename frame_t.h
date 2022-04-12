//
// Created by Roman Oberenkowski on 21.11.2021.
//
#ifndef FRAME_T_H
#define FRAME_T_H

#include "led_constants.h"

struct frame_t {
    char data[COLUMNS_COUNT][PAYLOAD_LENGTH];
    int offset;
};


#endif //FRAME_T_H
