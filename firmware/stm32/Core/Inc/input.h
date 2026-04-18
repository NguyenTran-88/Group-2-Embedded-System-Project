/*
 * input.h
 */

#ifndef INC_INPUT_H_
#define INC_INPUT_H_

#include "main.h"
#include <stdint.h>

typedef enum
{
    EVT_PLAY_TOGGLE = 0,
    EVT_PREV_TRACK,
    EVT_NEXT_TRACK,
    EVT_MODE_TOGGLE,
    EVT_VOL_DOWN,
    EVT_VOL_UP,
    EVT_COUNT
} AppEvent_t;

void Input_Init(void);
uint8_t Input_PopEvent(AppEvent_t *evt_out);

#endif /* INC_INPUT_H_ */
