/*
 * player_service.c
 *
 * Playback control service for the STM32 audio player.
 *
 * This module sits between the UI/input layer and the low-level DFPlayer and
 * GPIO control paths. It exists to keep policy decisions in one place:
 * which source is active, when Bluetooth power should be enabled, when the
 * audio path should be muted to avoid switching artifacts, and how the system
 * should interpret playback state for the UI.
 */

#include "player_service.h"
#include <stdint.h>

/*
 * Local operating limits.
 *
 * These values are intentionally kept in the service layer instead of being
 * scattered across UI code, because playback policy belongs here rather than
 * in button handlers.
 */

#define DFPLAYER_VOL_MIN               0u
#define DFPLAYER_VOL_MAX               30u
#define DFPLAYER_VOL_INIT              10u

/*
 * Hardware audio source selection.
 *
 * The service can optionally drive an external analog switch / mux. Keeping
 * the feature under compile-time control makes the module easier to reuse if
 * the hardware changes later.
 */

#define AUDIO_SOURCE_HW_SWITCH_EN      1u

#define AUDIO_SEL_GPIO_Port            GPIOB
#define AUDIO_SEL_Pin                  GPIO_PIN_3

#define AUDIO_INH_GPIO_Port            GPIOB
#define AUDIO_INH_Pin                  GPIO_PIN_4

#define AUDIO_SEL_SD_LEVEL             GPIO_PIN_RESET
#define AUDIO_SEL_BT_LEVEL             GPIO_PIN_SET

#define AUDIO_INH_ENABLE_LEVEL         GPIO_PIN_RESET
#define AUDIO_INH_DISABLE_LEVEL        GPIO_PIN_SET

/*
 * Bluetooth power gating.
 *
 * The Bluetooth module is powered only when needed so the system avoids idle
 * noise injection and unnecessary current draw while operating in SD mode.
 */

#define BT_EN_CONTROL_EN               1u
#define BT_EN_GPIO_Port                GPIOB
#define BT_EN_Pin                      GPIO_PIN_1
#define BT_EN_ACTIVE_LEVEL             GPIO_PIN_SET
#define BT_EN_INACTIVE_LEVEL           GPIO_PIN_RESET

#define BT_BOOT_WAIT_MS                300u

/*
 * DFPlayer BUSY input handling.
 */

#define DFP_BUSY_GPIO_Port             GPIOB
#define DFP_BUSY_Pin                   GPIO_PIN_5

#define DFP_BUSY_ACTIVE_LOW            1u
#define DFP_BUSY_STABLE_MS             20u
#define DFP_BUSY_IGNORE_AFTER_CMD_MS   500u

/*
 * Startup and query timings.
 */

#define DFP_BOOT_INIT_WAIT_MS         2000u
#define DFP_SELECT_TF_WAIT_MS          300u
#define DFP_QUERY_TIMEOUT_MS           300u
#define DFP_NEXT_SETTLE_MS             250u
#define DFP_FALLBACK_MAX_TRACKS        255u

/*
 * Private service state.
 *
 * This block represents the application's working view of playback. The UI is
 * expected to read from this cached state instead of constantly probing the
 * hardware, which keeps rendering logic simpler and avoids unnecessary traffic
 * to the DFPlayer.
 */

static DFPLAYER_Name  g_dfp;

static uint8_t        g_is_playing      = 0u;
static uint8_t        g_df_started_once = 0u;
static uint8_t        g_volume          = DFPLAYER_VOL_INIT;
static AudioSource_t  g_source          = AUDIO_SRC_DFPLAYER_SD;

static uint8_t        g_busy_idle_stable = 1u;
static uint8_t        g_busy_idle_sample = 1u;
static uint32_t       g_busy_tick        = 0u;

static uint32_t       g_dfp_cmd_tick     = 0u;

/*
 * Track count starts with a safe default and is refined during initialization.
 * A non-zero value avoids edge cases in wrap logic if counting fails.
 */

static uint8_t        g_track            = 1u;
static uint8_t        g_track_total      = 5u;


/* Private helpers. */
static void    PlayerService_SetAudioSource(AudioSource_t src);
static uint8_t PlayerService_DfpBusyIdleRaw(void);
static void    PlayerService_AutoNextByBusyService(void);
static void    PlayerService_AudioSwitchMute(uint8_t mute);
static uint8_t PlayerService_DetectTrackTotalByWrap(uint8_t *count_out);
static void    PlayerService_SetBtPower(uint8_t on);


void PlayerService_Init(UART_HandleTypeDef *huart)
{
    DFPLAYER_Init(&g_dfp, huart);

#if BT_EN_CONTROL_EN
    /*
     * Boot in SD mode with Bluetooth powered down.
     *
     * This keeps startup behavior deterministic and avoids bringing up an audio
     * source that is not yet selected.
     */
    PlayerService_SetBtPower(0u);
#endif

#if AUDIO_SOURCE_HW_SWITCH_EN
    /*
    * Mute before touching the source path so initialization side effects do
    * not leak to the speaker, especially during track counting.
    */
    PlayerService_AudioSwitchMute(1u);
    HAL_GPIO_WritePin(AUDIO_SEL_GPIO_Port, AUDIO_SEL_Pin, AUDIO_SEL_SD_LEVEL);
#endif

    /*
     * Give the DFPlayer enough time to finish its own internal power-up and
     * media scan before sending configuration commands.
     */
    HAL_Delay(DFP_BOOT_INIT_WAIT_MS);

    DFPLAYER_SetVolume(&g_dfp, g_volume);

    /*
     * Determine track count by observing wrap-around behavior.
     *
     * The module does not always provide a reliable total-track query in a way
     * that fits this design, so the service derives it using a controlled walk
     * through the playlist.
     */

    if (PlayerService_DetectTrackTotalByWrap(&g_track_total) == 0u)
    {
        g_track_total = 1u;
    }

    g_track = 1u;
    g_is_playing = 0u;
    g_df_started_once = 0u;

#if AUDIO_SOURCE_HW_SWITCH_EN
    HAL_Delay(1);
    PlayerService_AudioSwitchMute(0u);
#endif

    PlayerService_SetAudioSource(AUDIO_SRC_DFPLAYER_SD);

    g_busy_idle_sample = PlayerService_DfpBusyIdleRaw();
    g_busy_idle_stable = g_busy_idle_sample;
    g_busy_tick        = HAL_GetTick();
    g_dfp_cmd_tick     = HAL_GetTick();
}

void PlayerService_Service(void)
{
    PlayerService_AutoNextByBusyService();
}

void PlayerService_PlayToggle(void)
{
    if (g_source != AUDIO_SRC_DFPLAYER_SD)
    {
        return;
    }

    if (g_is_playing == 0u)
    {
        /*
         * Preserve the established project behavior: the first transition into
         * playback uses the standard play command rather than trying to infer a
         * resume point through other means.
         */
        DFPLAYER_Play(&g_dfp);
        g_df_started_once = 1u;
        g_dfp_cmd_tick = HAL_GetTick();
        g_is_playing = 1u;
    }
    else
    {
        DFPLAYER_Pause(&g_dfp);
        g_dfp_cmd_tick = HAL_GetTick();
        g_is_playing = 0u;
    }
}

void PlayerService_Prev(void)
{
    if (g_source != AUDIO_SRC_DFPLAYER_SD)
    {
        return;
    }

    if (g_track_total == 0u)
    {
        g_track_total = 1u;
    }

    g_track = (g_track > 1u) ? (g_track - 1u) : g_track_total;

    if (g_is_playing == 0u)
    {
        g_is_playing = 1u;
        g_df_started_once = 1u;
    }

    DFPLAYER_Prev(&g_dfp);
    g_dfp_cmd_tick = HAL_GetTick();
}

void PlayerService_Next(void)
{
    if (g_source != AUDIO_SRC_DFPLAYER_SD)
    {
        return;
    }

    g_track = (g_track < g_track_total) ? (g_track + 1u) : 1u;

    if (g_is_playing == 0u)
    {
        g_is_playing = 1u;
        g_df_started_once = 1u;
    }

    DFPLAYER_Next(&g_dfp);
    g_dfp_cmd_tick = HAL_GetTick();
}

void PlayerService_ModeToggle(void)
{
    if (g_source == AUDIO_SRC_DFPLAYER_SD)
    {
        PlayerService_SetAudioSource(AUDIO_SRC_BLUETOOTH);
    }
    else
    {
        PlayerService_SetAudioSource(AUDIO_SRC_DFPLAYER_SD);
    }
}

void PlayerService_VolUp(void)
{
    if (g_source != AUDIO_SRC_DFPLAYER_SD)
    {
        return;
    }

    if (g_volume < DFPLAYER_VOL_MAX)
    {
        g_volume++;
        DFPLAYER_SetVolume(&g_dfp, g_volume);
    }
}

void PlayerService_VolDown(void)
{
    if (g_source != AUDIO_SRC_DFPLAYER_SD)
    {
        return;
    }

    if (g_volume > DFPLAYER_VOL_MIN)
    {
        g_volume--;
        DFPLAYER_SetVolume(&g_dfp, g_volume);
    }
}

uint8_t PlayerService_IsPlaying(void)
{
    return g_is_playing;
}

uint8_t PlayerService_GetVolume(void)
{
    return g_volume;
}

uint8_t PlayerService_GetTrack(void)
{
    return g_track;
}

uint8_t PlayerService_GetTrackTotal(void)
{
    return g_track_total;
}

AudioSource_t PlayerService_GetSource(void)
{
    return g_source;
}

static void PlayerService_SetAudioSource(AudioSource_t src)
{
    if (src == g_source)
    {
        return;
    }
    /*
     * Pause the DFPlayer before leaving SD mode so playback state stays aligned
     * with the audible path. Otherwise the file could keep advancing silently in
     * the background while the user is listening to Bluetooth.
     */
    if ((src == AUDIO_SRC_BLUETOOTH) && (g_is_playing != 0u))
    {
        DFPLAYER_Pause(&g_dfp);
        g_is_playing = 0u;
        g_dfp_cmd_tick = HAL_GetTick();
    }

#if AUDIO_SOURCE_HW_SWITCH_EN
    /*
     * Mute around source switching to suppress pop and click artifacts caused by
     * analog path reconfiguration.
     */
    PlayerService_AudioSwitchMute(1u);
#endif

#if BT_EN_CONTROL_EN
    if (src == AUDIO_SRC_BLUETOOTH)
    {
    	/*
    	* Power Bluetooth before routing audio to it. The small guard delay is
    	* there to avoid exposing boot transients or a half-initialized source to
    	 * the amplifier.
    	*/
        PlayerService_SetBtPower(1u);
        HAL_Delay(BT_BOOT_WAIT_MS);
    }
#endif

#if AUDIO_SOURCE_HW_SWITCH_EN
    if (src == AUDIO_SRC_DFPLAYER_SD)
    {
        HAL_GPIO_WritePin(AUDIO_SEL_GPIO_Port,
                          AUDIO_SEL_Pin,
                          AUDIO_SEL_SD_LEVEL);
    }
    else
    {
        HAL_GPIO_WritePin(AUDIO_SEL_GPIO_Port,
                          AUDIO_SEL_Pin,
                          AUDIO_SEL_BT_LEVEL);
    }

    HAL_Delay(1u);
#endif

#if BT_EN_CONTROL_EN
    if (src == AUDIO_SRC_DFPLAYER_SD)
    {
        /*
         * Turn Bluetooth off only after the mux has already been moved away from
         * that path so power-down noise does not reach the output.
         */
        PlayerService_SetBtPower(0u);
    }
#endif

#if AUDIO_SOURCE_HW_SWITCH_EN
    PlayerService_AudioSwitchMute(0u);
#endif

    g_source = src;
}

static uint8_t PlayerService_DfpBusyIdleRaw(void)
{
#if (DFP_BUSY_ACTIVE_LOW != 0u)
    /* active-low: LOW = playing, HIGH = idle */
    return (HAL_GPIO_ReadPin(DFP_BUSY_GPIO_Port, DFP_BUSY_Pin) == GPIO_PIN_SET) ? 1u : 0u;
#else
    /* active-high: HIGH = playing, LOW = idle */
    return (HAL_GPIO_ReadPin(DFP_BUSY_GPIO_Port, DFP_BUSY_Pin) == GPIO_PIN_RESET) ? 1u : 0u;
#endif
}

static void PlayerService_AutoNextByBusyService(void)
{
    uint32_t now;
    uint8_t idle_now;
    uint8_t prev_idle;

    if (g_source != AUDIO_SRC_DFPLAYER_SD)
    {
        return;
    }

    now = HAL_GetTick();
    /*
     * Ignore BUSY changes immediately after transport commands. During this
     * window the pin may reflect internal DFPlayer transitions rather than a
     * genuine end-of-track event.
     */

    if ((now - g_dfp_cmd_tick) < DFP_BUSY_IGNORE_AFTER_CMD_MS)
    {
        return;
    }

    idle_now = PlayerService_DfpBusyIdleRaw();

    if (idle_now != g_busy_idle_sample)
    {
        g_busy_idle_sample = idle_now;
        g_busy_tick = now;
        return;
    }

    if ((now - g_busy_tick) < DFP_BUSY_STABLE_MS)
    {
        return;
    }

    if (g_busy_idle_stable != g_busy_idle_sample)
    {
        prev_idle = g_busy_idle_stable;
        g_busy_idle_stable = g_busy_idle_sample;

        if ((prev_idle == 0u) && (g_busy_idle_stable == 1u))
        {
            if (g_is_playing != 0u)
            {
                DFPLAYER_Next(&g_dfp);
                g_is_playing = 1u;
                g_df_started_once = 1u;
                g_track = (g_track < g_track_total) ? (g_track + 1u) : 1u;
                g_dfp_cmd_tick = now;
            }
        }
    }
}

static uint8_t PlayerService_DetectTrackTotalByWrap(uint8_t *count_out)
{
    uint16_t start_track = 0u;
    uint16_t cur_track   = 0u;
    uint16_t step        = 0u;

    if (count_out == NULL)
    {
        return 0u;
    }

    *count_out = 1u;

    /*
     * Re-select TF storage so the DFPlayer rebuilds a known file pointer before
     * counting begins. This reduces dependence on whatever playback state the
     * module happened to retain before startup.
     */

    DFPLAYER_SelectTF(&g_dfp);
    HAL_Delay(DFP_SELECT_TF_WAIT_MS);

    if (DFPLAYER_QueryCurrentTfTrack(&g_dfp, &start_track, DFP_QUERY_TIMEOUT_MS) == 0u)
    {
        return 0u;
    }

    /*
     * Walk forward until the playlist wraps back to the starting track.
     *
     * This approach is slower than a direct total-file query, but it is robust
     * when the module's response behavior is limited or inconsistent across
     * clones.
     */
    if ((start_track == 0u) || (start_track > DFP_FALLBACK_MAX_TRACKS))
    {
        return 0u;
    }

    for (step = 1u; step <= DFP_FALLBACK_MAX_TRACKS; step++)
    {
        DFPLAYER_Next(&g_dfp);
        HAL_Delay(DFP_NEXT_SETTLE_MS);

        if (DFPLAYER_QueryCurrentTfTrack(&g_dfp, &cur_track, DFP_QUERY_TIMEOUT_MS) == 0u)
        {
            return 0u;
        }

        if (cur_track == start_track)
        {
            *count_out = (uint8_t)step;
            /*
            * Restore the module to a predictable post-init state: first track,
            * paused, and ready for UI control.
            */

            DFPLAYER_SelectTF(&g_dfp);
            HAL_Delay(DFP_SELECT_TF_WAIT_MS);

            return 1u;
        }
    }

    DFPLAYER_SelectTF(&g_dfp);
    HAL_Delay(DFP_SELECT_TF_WAIT_MS);

    return 0u;
}




static void PlayerService_AudioSwitchMute(uint8_t mute)
{
    HAL_GPIO_WritePin(AUDIO_INH_GPIO_Port,
                      AUDIO_INH_Pin,
                      (mute != 0u) ? AUDIO_INH_DISABLE_LEVEL
                                   : AUDIO_INH_ENABLE_LEVEL);
}

static void PlayerService_SetBtPower(uint8_t on)
{
#if BT_EN_CONTROL_EN
    HAL_GPIO_WritePin(BT_EN_GPIO_Port,
                      BT_EN_Pin,
                      (on != 0u) ? BT_EN_ACTIVE_LEVEL
                                 : BT_EN_INACTIVE_LEVEL);
#else
    (void)on;
#endif
}
