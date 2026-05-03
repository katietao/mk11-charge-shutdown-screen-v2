/*
 * bms_data.c
 *
 * BMS data placeholder initialisation and LCD display formatting.
 *
 * The display cycles through 3 screens every DISPLAY_CYCLE_MS:
 *
 *   Screen 0 — Cell Voltages
 *     Row 0: "H:3.800 L:3.200V"
 *     Row 1: "A:3.500 Pk:350.5"
 *
 *   Screen 1 — Temperatures
 *     Row 0: "H:45.0 L:22.0  C"
 *     Row 1: "Avg:33.5C SOC:85"
 *
 *   Screen 2 — CAN Debug
 *     Row 0: "CAN RX Count:   "
 *     Row 1: "       <count>  "
 */

#include "bms_data.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>

/* ---------- Global instance ---------- */
volatile BMS_Data_t bms_data;

/* ---------- Private state ---------- */
static uint8_t  current_screen = 0;
static uint32_t last_switch_tick = 0;

/* ---------- Placeholder init ---------- */
void BMS_Data_Init(void)
{
    /* Initialize to 0 so we don't display fake data if disconnected */
    bms_data.highest_cell_voltage = 0;
    bms_data.lowest_cell_voltage  = 0;
    bms_data.avg_cell_voltage     = 0;

    bms_data.pack_voltage = 0;

    bms_data.highest_temp = 0;
    bms_data.lowest_temp  = 0;
    bms_data.avg_temp     = 0;

    bms_data.pack_current = 0;
    bms_data.soc = 0;

    bms_data.can_rx_count = 0;
}

/* ---------- Display helpers ---------- */

/**
 * @brief  Format and display Screen 0: Cell Voltages
 *
 *   Row 0: "H:3.800 L:3.200V"
 *   Row 1: "A:3.500 Pk:350.5"
 */
static void Display_Voltages(void)
{
    char line[25]; /* 24 chars + null */

    /* --- Row 0 --- */
    uint16_t hv = bms_data.highest_cell_voltage;
    uint16_t lv = bms_data.lowest_cell_voltage;

    snprintf(line, sizeof(line), "H:%u.%03u L:%u.%03u",
             hv / 10000, (hv % 10000) / 10,
             lv / 10000, (lv % 10000) / 10);

    LCD_SetCursor(0, 0);
    LCD_WriteString(line);

    /* --- Row 1 --- */
    uint16_t av = bms_data.avg_cell_voltage;
    uint16_t pv = bms_data.pack_voltage;

    snprintf(line, sizeof(line), "A:%u.%03u Pk:%3u.%1u",
             av / 10000, (av % 10000) / 10,
             pv / 10, pv % 10);

    LCD_SetCursor(1, 0);
    LCD_WriteString(line);
}

/**
 * @brief  Format and display Screen 1: Temperatures
 *
 *   Row 0: "H:45.0 L:22.0  C"
 *   Row 1: "Avg:33.5C SOC:85"
 */
static void Display_Temps(void)
{
    char line[25];

    /* --- Row 0 --- */
    int16_t ht = bms_data.highest_temp;
    int16_t lt = bms_data.lowest_temp;

    snprintf(line, sizeof(line), "H:%d.%d L:%d.%d  C",
             ht / 10, ht % 10,
             lt / 10, lt % 10);

    LCD_SetCursor(0, 0);
    LCD_WriteString(line);

    /* --- Row 1 --- */
    int16_t at = bms_data.avg_temp;
    uint16_t soc_whole = bms_data.soc / 10;

    snprintf(line, sizeof(line), "Avg:%d.%dC SOC:%u%%",
             at / 10, at % 10,
             soc_whole);

    LCD_SetCursor(1, 0);
    LCD_WriteString(line);
}

/**
 * @brief  Format and display Screen 2: CAN Debug
 *
 *   Row 0: "CAN RX Count:   "
 *   Row 1: "         <count> "
 */
static void Display_CAN_Debug(void)
{
    char line[25];

    LCD_SetCursor(0, 0);
    LCD_WriteString("CAN RX Count:   ");

    snprintf(line, sizeof(line), "%-16lu", (unsigned long)bms_data.can_rx_count);
    LCD_SetCursor(1, 0);
    LCD_WriteString(line);
}

/* ---------- Main display update ---------- */
void BMS_UpdateDisplay(I2C_HandleTypeDef *hi2c)
{
    (void)hi2c; /* Reserved for future use if needed */

    uint32_t now = HAL_GetTick();

    if ((now - last_switch_tick) >= DISPLAY_CYCLE_MS)
    {
        last_switch_tick = now;
        current_screen = (current_screen + 1) % DISPLAY_NUM_SCREENS;

        LCD_Clear();

        switch (current_screen)
        {
            case 0:
                Display_Voltages();
                break;
            case 1:
                Display_Temps();
                break;
            case 2:
                Display_CAN_Debug();
                break;
            default:
                break;
        }
    }
}
