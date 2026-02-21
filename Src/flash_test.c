/**
 * @file    flash_example.c
 * @brief   W25Q64FV driver test with STM32CubeIDE Live Expressions support
 * @author Saikishen
 * call Flash_Demo(); from main.c in setup part of int main()
 */

#include "main.h"
#include "w25q64fv.h"
#include <string.h>
#include <stdio.h>

extern SPI_HandleTypeDef hspi2;


volatile uint8_t  g_flash_result        = 0xFF;
volatile uint8_t  g_flash_step          = 0;
volatile uint8_t  g_flash_mfr_id        = 0x00;
volatile uint16_t g_flash_jedec_id      = 0x0000;
volatile uint8_t  g_flash_uid[8]        = {0};
volatile uint8_t  g_flash_sr1           = 0x00;
volatile uint8_t  g_flash_sr2           = 0x00;
volatile uint8_t  g_flash_verify_ok     = 0;
volatile uint32_t g_flash_mismatch_byte = 0xFFFFFFFF;
volatile uint32_t g_flash_erase_ms      = 0;
volatile uint32_t g_flash_write_ms      = 0;
volatile uint32_t g_flash_read_ms       = 0;
static uint8_t s_tx_buf[512];
static uint8_t s_rx_buf[512];


#define RUN_STEP(step_num, expr) do {g_flash_step = (step_num); status = (expr);g_flash_result = (uint8_t)status; if (status != W25Q64FV_OK) return;    } while (0)


void Flash_Demo(void)
{
    W25Q64FV_Handle flash;
    W25Q64FV_Status status;
    uint32_t        t_start;

    g_flash_result        = 0xFF;
    g_flash_step          = 0;
    g_flash_mfr_id        = 0x00;
    g_flash_jedec_id      = 0x0000;
    g_flash_sr1           = 0x00;
    g_flash_sr2           = 0x00;
    g_flash_verify_ok     = 0;
    g_flash_mismatch_byte = 0xFFFFFFFF;
    g_flash_erase_ms      = 0;
    g_flash_write_ms      = 0;
    g_flash_read_ms       = 0;
    memset((void *)g_flash_uid, 0, sizeof(g_flash_uid));

    RUN_STEP(1, W25Q64FV_Init(&flash, &hspi2,FLASH_CS_GPIO_Port, FLASH_CS_Pin));

    g_flash_step = 2;
    status = W25Q64FV_ReadJEDECID(&flash,
                                    (uint8_t *)&g_flash_mfr_id,
                                    (uint16_t *)&g_flash_jedec_id);
    g_flash_result = (uint8_t)status;
    if (status != W25Q64FV_OK) return;
    g_flash_step = 3;
    status = W25Q64FV_ReadUniqueID(&flash, (uint8_t *)g_flash_uid);
    g_flash_result = (uint8_t)status;
    if (status != W25Q64FV_OK) return;
    g_flash_step = 4;
    W25Q64FV_ReadSR1(&flash, (uint8_t *)&g_flash_sr1);
    W25Q64FV_ReadSR2(&flash, (uint8_t *)&g_flash_sr2);
    g_flash_step = 5;
    t_start = HAL_GetTick();
    status  = W25Q64FV_SectorErase(&flash, 0x000000UL);
    g_flash_erase_ms = HAL_GetTick() - t_start;
    g_flash_result   = (uint8_t)status;
    if (status != W25Q64FV_OK) return;
    for (uint16_t i = 0; i < sizeof(s_tx_buf); i++)
        s_tx_buf[i] = (uint8_t)i;

    g_flash_step = 6;
    t_start = HAL_GetTick();
    status  = W25Q64FV_Write(&flash, 0x000000UL, s_tx_buf, sizeof(s_tx_buf));
    g_flash_write_ms = HAL_GetTick() - t_start;
    g_flash_result   = (uint8_t)status;
    if (status != W25Q64FV_OK) return;
    g_flash_step = 7;
    memset(s_rx_buf, 0, sizeof(s_rx_buf));

    t_start = HAL_GetTick();
    status  = W25Q64FV_Read(&flash, 0x000000UL, s_rx_buf, sizeof(s_rx_buf));
    g_flash_read_ms = HAL_GetTick() - t_start;
    g_flash_result  = (uint8_t)status;
    if (status != W25Q64FV_OK) return;

    g_flash_verify_ok     = 1;
    g_flash_mismatch_byte = 0xFFFFFFFF;
    for (uint32_t i = 0; i < sizeof(s_tx_buf); i++) {
        if (s_tx_buf[i] != s_rx_buf[i]) {
            g_flash_verify_ok     = 0;
            g_flash_mismatch_byte = i;
            break;
        }
    }
    g_flash_step = 8;
    W25Q64FV_PowerDown(&flash);

}
