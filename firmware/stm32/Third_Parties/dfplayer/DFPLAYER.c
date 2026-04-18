/******************************************************************************************************************
@File:  	DFPlayer Mini Module
@Author:  Khue Nguyen
@Website: khuenguyencreator.com
@Youtube: https://www.youtube.com/channel/UCt8cFnPOaHrQXWmVkk-lfvg
Huong dan su dung:
- Su dung thu vien HAL
- Khoi tao UART Baud 9600
- Khoi tao bien DFPlayer : DFPLAYER_Name MP3;
- Khoi tao DFPlayer do:
	DFPLAYER_Init(&MP3, &huart1);
- Su dung cac ham phai truyen dia chi cua DFPlayer do: 
	DFPLAYER_Play(&MP3);
******************************************************************************************************************/
#include "DFPLAYER.h"

static uint8_t SendFrame[10] = {0x7E, 0xFF, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};

//******************************** LOW Level Functions ****************************//
static void DFPLAYER_SendUART(DFPLAYER_Name* MP3)
{
    HAL_UART_Transmit(MP3->DFP_UART, (uint8_t *)&MP3->SendBuff, sizeof(MP3->SendBuff), 1000);
}

static void DFPLAYER_Delay(uint16_t Time)
{
    HAL_Delay(Time);
}

static uint16_t DFPLAYER_GetChecksum(uint8_t *thebuf)
{
    uint16_t sum = 0;
    for (int i = 1; i < 7; i++)
    {
        sum += thebuf[i];
    }
    return (uint16_t)(0u - sum);
}

static void DFPLAYER_FillBigend(uint8_t *thebuf, uint16_t data)
{
    *thebuf = (uint8_t)(data >> 8);
    *(thebuf + 1) = (uint8_t)data;
}

static void DFPLAYER_FillChecksum(DFPLAYER_Name* MP3)
{
    uint16_t checksum = DFPLAYER_GetChecksum(MP3->SendBuff);
    DFPLAYER_FillBigend(MP3->SendBuff + 7, checksum);
}

static void DFPLAYER_SendCmdEx(DFPLAYER_Name* MP3,
                               uint8_t cmd,
                               uint8_t feedback,
                               uint8_t param_h,
                               uint8_t param_l)
{
    DFPLAYER_Delay(100);
    MP3->SendBuff[3] = cmd;
    MP3->SendBuff[4] = feedback;
    MP3->SendBuff[5] = param_h;
    MP3->SendBuff[6] = param_l;
    DFPLAYER_FillChecksum(MP3);
    DFPLAYER_SendUART(MP3);
}

static void DFPLAYER_SendCmd(DFPLAYER_Name* MP3, uint8_t cmd, uint16_t high_arg, uint16_t low_arg)
{
    DFPLAYER_SendCmdEx(MP3, cmd, 0x00u, (uint8_t)high_arg, (uint8_t)low_arg);
}



static void DFPLAYER_FlushRx(DFPLAYER_Name* MP3)
{
    uint8_t dummy;

    while (HAL_UART_Receive(MP3->DFP_UART, &dummy, 1u, 5u) == HAL_OK)
    {
        /* drain old bytes */
    }
}

static uint8_t DFPLAYER_ReadFrame(DFPLAYER_Name* MP3, uint8_t frame[10], uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t first = 0u;

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        if (HAL_UART_Receive(MP3->DFP_UART, &first, 1u, 10u) != HAL_OK)
        {
            continue;
        }

        if (first != 0x7E)
        {
            continue;
        }

        frame[0] = first;

        if (HAL_UART_Receive(MP3->DFP_UART, &frame[1], 9u, timeout_ms) == HAL_OK)
        {
            if (frame[9] == 0xEF)
            {
                return 1u;
            }
        }
    }

    return 0u;
}

static uint8_t DFPLAYER_WaitResponseCmd(DFPLAYER_Name* MP3,
                                        uint8_t expected_cmd,
                                        uint16_t *param_out,
                                        uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t frame[10];

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        uint32_t remain = timeout_ms - (HAL_GetTick() - start);

        if (DFPLAYER_ReadFrame(MP3, frame, remain) == 0u)
        {
            break;
        }

        /* Ignore unrelated frames like 0x3F init, 0x40 error, 0x41 ack... */
        if (frame[3] != expected_cmd)
        {
            continue;
        }

        if (param_out != NULL)
        {
            *param_out = (uint16_t)(((uint16_t)frame[5] << 8) | frame[6]);
        }

        return 1u;
    }

    return 0u;
}

//******************************** High Level Functions ****************************//
void DFPLAYER_Init(DFPLAYER_Name* MP3, UART_HandleTypeDef* UART)
{
	for(int i = 0; i<10; i++)
	{
		MP3->SendBuff[i] = SendFrame[i];
	}
	MP3->DFP_UART = UART;
}

void DFPLAYER_PlayTrack(DFPLAYER_Name* MP3, uint16_t num)
{
	uint8_t num1 = num>>8;
	uint8_t num2 = num;
	DFPLAYER_SendCmd(MP3, DFP_PLAYTRACK, num1, num2);
}

void DFPLAYER_Next(DFPLAYER_Name* MP3) 
{
	DFPLAYER_SendCmd(MP3, DFP_NEXT,00,00);
}

void DFPLAYER_Prev(DFPLAYER_Name* MP3)
{
	DFPLAYER_SendCmd(MP3, DFP_PREV,00,00);
}

void DFPLAYER_SetVolume(DFPLAYER_Name* MP3, uint16_t volume)
{
	uint8_t volume1 = volume>>8;
	uint8_t volume2 = volume;
	DFPLAYER_SendCmd(MP3, DFP_SETVOLUME, volume1, volume2);
}

void DFPLAYER_Play(DFPLAYER_Name* MP3) 
{
	DFPLAYER_SendCmd(MP3, DFP_PLAY, 00, 00);
} 

void DFPLAYER_Pause(DFPLAYER_Name* MP3)
{
	DFPLAYER_SendCmd(MP3, DFP_PAUSE, 00, 00);
}

void DFPLAYER_Stop(DFPLAYER_Name* MP3)
{
	DFPLAYER_SendCmd (MP3, DFP_STOP, 00, 00);
}

void DFPLAYER_RandomPlay(DFPLAYER_Name* MP3)
{
	DFPLAYER_SendCmd (MP3, DFP_RANDOM,0,0);
}

void DFPLAYER_PlayFileInFolder(DFPLAYER_Name* MP3, uint8_t folder, uint32_t num) 
{
	DFPLAYER_SendCmd (MP3, DFP_PLAYFILEINFOLDER, folder, num);
}

void DFPLAYER_SelectTF(DFPLAYER_Name* MP3)
{
    DFPLAYER_SendCmdEx(MP3, DFP_SELECT_SOURCE, 0x00u, 0x00u, DFP_DEVICE_TF);
}

uint8_t DFPLAYER_QueryCurrentTfTrack(DFPLAYER_Name* MP3,
                                     uint16_t *track_out,
                                     uint32_t timeout_ms)
{
    if (track_out == NULL)
    {
        return 0u;
    }

    DFPLAYER_FlushRx(MP3);
    DFPLAYER_SendCmdEx(MP3, DFP_QUERY_CUR_TF_TRACK, 0x00u, 0x00u, 0x00u);

    return DFPLAYER_WaitResponseCmd(MP3, DFP_QUERY_CUR_TF_TRACK, track_out, timeout_ms);
}
