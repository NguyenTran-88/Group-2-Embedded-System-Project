/*
 * ui.h
 * UI module for the SSD1306 player screen.
 *
 * This layer owns screen drawing and lightweight redraw policy.
 * It stays separate from player logic so UI behavior can evolve
 * without leaking display details into the service layer.
 */

#ifndef INC_UI_H_
#define INC_UI_H_

#include <stdint.h>

void UI_Init(void);
void UI_FirstDraw(void);
void UI_RefreshAll(void);
void UI_ResetWaveAnim(void);
void UI_Service(void);
void UI_DrawPauseFull(void);
void UI_DrawPauseIconOnly(void);
void UI_DrawPlayIconOnly(void);
void UI_DrawBluetoothFull(void);
void UI_DrawWaveFrame(uint8_t frame);

#endif /* INC_UI_H_ */
