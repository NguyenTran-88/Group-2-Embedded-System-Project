/*
 * ui.c
 *

 */

#include "ui.h"
#include "ui_assets.h"
#include "player_service.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include <stdio.h>
#include <stdint.h>

/*
 * Fixed update regions keep redraws predictable and avoid refreshing
 * more of the OLED than necessary.
 */

#define UI_TRACK_X              48
#define UI_TRACK_Y              6
#define UI_TRACK_W              80
#define UI_TRACK_H              10

#define UI_VOL_X                22
#define UI_VOL_Y                40
#define UI_VOL_W                12
#define UI_VOL_H                8

#define UI_WAVE_ANIM_PERIOD_MS  300u

/*
 * Cached player state lets the UI update only when something visible
 * actually changed.
 */

static uint8_t  g_wave_frame     = 0u;
static uint32_t g_wave_anim_tick = 0u;

static uint8_t      g_cache_valid       = 0u;
static AudioSource_t g_cache_source     = AUDIO_SRC_DFPLAYER_SD;
static uint8_t      g_cache_is_playing  = 0u;
static uint8_t      g_cache_track       = 1u;
static uint8_t      g_cache_track_total = 1u;
static uint8_t      g_cache_volume      = 0u;

/* ===== Private function prototypes ===== */

static void UI_DrawTrack(void);
static void UI_DrawVolume(void);
static void UI_UpdateCacheFromPlayer(void);

/* ===== Public functions ===== */

void UI_Init(void)
{
    ssd1306_Init();
    UI_ResetWaveAnim();

    /* Force the first service pass to draw from a clean state. */
    g_cache_valid = 0u;
}

void UI_FirstDraw(void)
{
	/* Keep the same startup look as the previous implementation. */
	UI_DrawPauseFull();

    UI_DrawTrack();
    UI_DrawVolume();

    UI_UpdateCacheFromPlayer();
    g_cache_valid = 1u;
}

void UI_RefreshAll(void)
{
    AudioSource_t src     = PlayerService_GetSource();
    uint8_t       playing = PlayerService_IsPlaying();

    /* Choose a full-screen base image that matches the current mode. */
    if (src == AUDIO_SRC_BLUETOOTH)
    {
        UI_DrawBluetoothFull();
    }
    else
    {
        if (g_cache_valid == 0u)
        {
            UI_DrawPauseFull();
        }
        else if (g_cache_source != AUDIO_SRC_DFPLAYER_SD)
        {
            /* Returning from Bluetooth should restore the SD layout. */
            UI_DrawPauseFull();
        }
        else if ((g_cache_is_playing == 0u) && (playing != 0u))
        {
            UI_DrawPlayIconOnly();
        }
        else if ((g_cache_is_playing != 0u) && (playing == 0u))
        {
            UI_DrawPauseIconOnly();
        }

        /* Text stays visible whenever SD mode is active. */
        UI_DrawTrack();
        UI_DrawVolume();
    }

    UI_UpdateCacheFromPlayer();
    g_cache_valid = 1u;
}

void UI_ResetWaveAnim(void)
{
    g_wave_frame = 0u;
    g_wave_anim_tick = HAL_GetTick();
}

void UI_Service(void)
{
    AudioSource_t src         = PlayerService_GetSource();
    uint8_t       playing     = PlayerService_IsPlaying();
    uint8_t       track       = PlayerService_GetTrack();
    uint8_t       track_total = PlayerService_GetTrackTotal();
    uint8_t       volume      = PlayerService_GetVolume();
    uint32_t      now;

    /* Source or play-state changes affect the whole layout. */
    if ((g_cache_valid == 0u) ||
        (src != g_cache_source) ||
        (playing != g_cache_is_playing))
    {
        UI_RefreshAll();
        return;
    }

    /* Track and volume can be refreshed locally. */
    if (src == AUDIO_SRC_DFPLAYER_SD)
    {
        if ((track != g_cache_track) || (track_total != g_cache_track_total))
        {
            UI_DrawTrack();
            g_cache_track = track;
            g_cache_track_total = track_total;
        }

        if (volume != g_cache_volume)
        {
            UI_DrawVolume();
            g_cache_volume = volume;
        }
    }

    /* Wave animation only makes sense during active SD playback. */
    if ((src != AUDIO_SRC_DFPLAYER_SD) || (playing == 0u))
    {
        return;
    }

    now = HAL_GetTick();
    if ((now - g_wave_anim_tick) < UI_WAVE_ANIM_PERIOD_MS)
    {
        return;
    }

    g_wave_anim_tick = now;

    /* Redraw the animated region, then restore the text overlay. */
    UI_DrawWaveFrame(g_wave_frame);
    UI_DrawTrack();
    UI_DrawVolume();

    g_wave_frame++;
    if (g_wave_frame >= 5u)
    {
        g_wave_frame = 0u;
    }

    UI_UpdateCacheFromPlayer();
}

static void UI_DrawTrack(void)
{
    char s[16];

    snprintf(s, sizeof(s), "Track: %u /%u",
             (unsigned int)PlayerService_GetTrack(),
             (unsigned int)PlayerService_GetTrackTotal());

    ssd1306_FillRectangle(UI_TRACK_X, UI_TRACK_Y,
                          UI_TRACK_X + UI_TRACK_W - 1,
                          UI_TRACK_Y + UI_TRACK_H - 1,
                          Black);

    ssd1306_SetCursor(UI_TRACK_X, UI_TRACK_Y);
    ssd1306_WriteString(s, Font_6x8, White);

    ssd1306_UpdateArea(UI_TRACK_X, UI_TRACK_Y, UI_TRACK_W, UI_TRACK_H);
}

static void UI_DrawVolume(void)
{
    char s[5];

    snprintf(s, sizeof(s), "%u",
             (unsigned int)PlayerService_GetVolume());

    ssd1306_FillRectangle(UI_VOL_X, UI_VOL_Y,
                          UI_VOL_X + UI_VOL_W - 1,
                          UI_VOL_Y + UI_VOL_H - 1,
                          Black);

    ssd1306_SetCursor(UI_VOL_X, UI_VOL_Y);
    ssd1306_WriteString(s, Font_6x8, White);

    ssd1306_UpdateArea(UI_VOL_X, UI_VOL_Y, UI_VOL_W, UI_VOL_H);
}

static void UI_UpdateCacheFromPlayer(void)
{
    g_cache_source      = PlayerService_GetSource();
    g_cache_is_playing  = PlayerService_IsPlaying();
    g_cache_track       = PlayerService_GetTrack();
    g_cache_track_total = PlayerService_GetTrackTotal();
    g_cache_volume      = PlayerService_GetVolume();
}

void UI_DrawPauseFull(void)
{
    ssd1306_Fill(Black);
    ssd1306_DrawBitmap(0, 0, ui_pause_bitmap, 128, 64, White);
    ssd1306_UpdateScreen();
}

void UI_DrawPauseIconOnly(void)
{
    ssd1306_Fill(Black);
    ssd1306_DrawBitmap(0, 0, ui_pause_bitmap, 128, 64, White);
    ssd1306_UpdateArea(70, 42, 28, 28);
}

void UI_DrawPlayIconOnly(void)
{
    ssd1306_Fill(Black);
    ssd1306_DrawBitmap(0, 0, ui_play_bitmap, 128, 64, White);
    ssd1306_UpdateArea(70, 42, 28, 28);
}

void UI_DrawBluetoothFull(void)
{
    ssd1306_Fill(Black);
    ssd1306_DrawBitmap(0, 0, ui_bluetooth_bitmap, 128, 64, White);
    ssd1306_UpdateScreen();
}

void UI_DrawWaveFrame(uint8_t frame)
{
    const uint8_t* wave_frames[] =
    {
        ui_wave_1,
        ui_wave_2,
        ui_wave_3,
        ui_wave_4,
        ui_wave_5,
        ui_wave_6,
        ui_wave_7,
        ui_wave_8,
        ui_wave_9,
        ui_wave_10,
        ui_wave_11
    };

    const uint8_t* bmp = wave_frames[frame % 11u];

    ssd1306_Fill(Black);
    ssd1306_DrawBitmap(0, 0, bmp, 128, 64, White);
    ssd1306_UpdateArea(54, 18, 58, 18);
}
