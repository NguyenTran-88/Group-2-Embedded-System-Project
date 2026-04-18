/*
 * player_service.h
 *
 * Service layer for user-facing playback control.
 *
 * This module keeps the rest of the application insulated from DFPlayer command
 * details and audio source switching logic. The goal is to expose a small,
 * predictable API for UI code while centralizing state such as playback status,
 * active source, volume, and track indexing in one place.
 */

#ifndef INC_PLAYER_SERVICE_H_
#define INC_PLAYER_SERVICE_H_

#include "main.h"
#include "DFPLAYER.h"
#include <stdint.h>

typedef enum
{
    AUDIO_SRC_DFPLAYER_SD = 0,
    AUDIO_SRC_BLUETOOTH
} AudioSource_t;

void PlayerService_Init(UART_HandleTypeDef *huart);
void PlayerService_Service(void);

void PlayerService_PlayToggle(void);
void PlayerService_Prev(void);
void PlayerService_Next(void);
void PlayerService_ModeToggle(void);
void PlayerService_VolUp(void);
void PlayerService_VolDown(void);

uint8_t PlayerService_IsPlaying(void);
uint8_t PlayerService_GetVolume(void);
uint8_t PlayerService_GetTrack(void);
uint8_t PlayerService_GetTrackTotal(void);
AudioSource_t PlayerService_GetSource(void);

#endif /* INC_PLAYER_SERVICE_H_ */
