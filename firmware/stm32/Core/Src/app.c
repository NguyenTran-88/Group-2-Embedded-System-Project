/*
 * app.c
 *
 * main loop handling all the submodules
 *
 */
#include "app.h"
#include "input.h"
#include "player_service.h"
#include "ui.h"

static void App_HandleEvent(AppEvent_t evt);

void App_Init(UART_HandleTypeDef *huart)
{
    Input_Init();
    PlayerService_Init(huart);
    UI_Init();
    UI_FirstDraw();
}

void App_Loop(void)
{
    AppEvent_t evt;

    PlayerService_Service();

    while (Input_PopEvent(&evt))
    {
        App_HandleEvent(evt);
    }

    UI_Service();
}

static void App_HandleEvent(AppEvent_t evt)
{
    switch (evt)
    {
        case EVT_PLAY_TOGGLE:
            PlayerService_PlayToggle();
            UI_ResetWaveAnim();
            UI_RefreshAll();
            break;

        case EVT_PREV_TRACK:
            PlayerService_Prev();
            UI_ResetWaveAnim();
            UI_RefreshAll();
            break;

        case EVT_NEXT_TRACK:
            PlayerService_Next();
            UI_ResetWaveAnim();
            UI_RefreshAll();
            break;

        case EVT_MODE_TOGGLE:
            PlayerService_ModeToggle();
            UI_ResetWaveAnim();
            UI_RefreshAll();
            break;

        case EVT_VOL_DOWN:
            PlayerService_VolDown();
            UI_RefreshAll();
            break;

        case EVT_VOL_UP:
            PlayerService_VolUp();
            UI_RefreshAll();
            break;

        default:
            break;
    }
}

