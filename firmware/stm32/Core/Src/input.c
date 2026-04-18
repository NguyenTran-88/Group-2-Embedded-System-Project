/*
 * input.c
 *
 * Button input handling for EXTI-based user controls.
 *
 * Responsibilities:
 * - map GPIO interrupts to application events
 * - debounce button presses in the EXTI callback
 * - push events into a lightweight ISR-safe queue
 */

#include "input.h"
#include <stdint.h>

/*
 * Minimum time between two accepted interrupts from the same button.
 */

#define DEBOUNCE_MS    120u

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
    AppEvent_t    evt;
    uint32_t      last_irq_tick;
} ButtonMap_t;

/*
 * Simple ring buffer used to pass button events from ISR context
 * to the main application loop.
 */

static volatile uint8_t  g_evt_q[16];
static volatile uint8_t  g_evt_q_head = 0u;
static volatile uint8_t  g_evt_q_tail = 0u;

/*
 * Debug helpers for checking which EXTI line fired and how often.
 */

static volatile uint16_t g_dbg_last_exti_pin = 0u;
static volatile uint32_t g_dbg_exti_count[16] = {0u};

/*
 * GPIO-to-event mapping for all user buttons.
 */

static ButtonMap_t g_buttons[] =
{
    { GPIOA, GPIO_PIN_3, EVT_PLAY_TOGGLE, 0u }, 	/* PA3 : Play/Pause */
    { GPIOA, GPIO_PIN_4, EVT_PREV_TRACK,  0u }, 	/* PA4 : Previous Track */
    { GPIOA, GPIO_PIN_5, EVT_NEXT_TRACK,  0u }, 	/* PA5 : Next Track */
    { GPIOA, GPIO_PIN_8, EVT_VOL_DOWN,    0u }, 	/* PA8 : Volume Down */
    { GPIOA, GPIO_PIN_9, EVT_VOL_UP,      0u }, 	/* PA9 : Volume Up */
    { GPIOA, GPIO_PIN_10, EVT_MODE_TOGGLE, 0u }, 	/* PA10 : Mode */
};

/* Internal helpers. */

static int32_t Input_FindButtonIndexByPin(uint16_t pin);
static void    Input_PostEventFromISR(AppEvent_t evt);

void Input_Init(void)
{
    uint32_t i;

    __disable_irq();

    g_evt_q_head = 0u;
    g_evt_q_tail = 0u;
    g_dbg_last_exti_pin = 0u;

    for (i = 0u; i < (uint32_t)(sizeof(g_evt_q) / sizeof(g_evt_q[0])); i++)
    {
        g_evt_q[i] = 0u;
    }

    for (i = 0u; i < 16u; i++)
    {
        g_dbg_exti_count[i] = 0u;
    }

    for (i = 0u; i < (uint32_t)(sizeof(g_buttons) / sizeof(g_buttons[0])); i++)
    {
        g_buttons[i].last_irq_tick = 0u;
    }

    __enable_irq();
}

uint8_t Input_PopEvent(AppEvent_t *evt_out)
{
    uint8_t has_data = 0u;

    if (evt_out == NULL)
    {
        return 0u;
    }

    __disable_irq();
    if (g_evt_q_tail != g_evt_q_head)
    {
        *evt_out = (AppEvent_t)g_evt_q[g_evt_q_tail];
        g_evt_q_tail = (uint8_t)((g_evt_q_tail + 1u) % (uint8_t)sizeof(g_evt_q));
        has_data = 1u;
    }
    __enable_irq();

    return has_data;
}

/*
 * EXTI callback from HAL.
 *
 * With the current CubeMX setup (GPIO_MODE_IT_FALLING + pull-up),
 * a falling edge is treated as a valid button press.
 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    int32_t  idx;
    uint32_t now = HAL_GetTick();

    g_dbg_last_exti_pin = GPIO_Pin;

    idx = Input_FindButtonIndexByPin(GPIO_Pin);
    if (idx < 0)
    {
        return;
    }

    /* Update per-pin debug counters for quick EXTI diagnostics. */
    switch (GPIO_Pin)
    {
        case GPIO_PIN_3:  g_dbg_exti_count[3]++;  break;
        case GPIO_PIN_4:  g_dbg_exti_count[4]++;  break;
        case GPIO_PIN_5:  g_dbg_exti_count[5]++;  break;
        case GPIO_PIN_8:  g_dbg_exti_count[8]++;  break;
        case GPIO_PIN_9:  g_dbg_exti_count[9]++;  break;
        case GPIO_PIN_10:  g_dbg_exti_count[10]++;  break;
        default: break;
    }

    /* Ignore repeated interrupts that arrive within the debounce window. */
    if ((now - g_buttons[(uint32_t)idx].last_irq_tick) < DEBOUNCE_MS)
    {
        return;
    }

    g_buttons[(uint32_t)idx].last_irq_tick = now;

    Input_PostEventFromISR(g_buttons[(uint32_t)idx].evt);
}

static int32_t Input_FindButtonIndexByPin(uint16_t pin)
{
    uint32_t i;

    for (i = 0u; i < (uint32_t)(sizeof(g_buttons) / sizeof(g_buttons[0])); i++)
    {
        if (g_buttons[i].pin == pin)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static void Input_PostEventFromISR(AppEvent_t evt)
{
    uint8_t next = (uint8_t)((g_evt_q_head + 1u) % (uint8_t)sizeof(g_evt_q));

    /*
    * If the queue is full, the new event is dropped intentionally.
    * This keeps the ISR short and avoids blocking behavior.
    */
    if (next != g_evt_q_tail)
    {
        g_evt_q[g_evt_q_head] = (uint8_t)evt;
        g_evt_q_head = next;
    }

}
