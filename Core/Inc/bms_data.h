/*
 * bms_data.h
 *
 * BMS data structures for the charge shutdown display.
 * Holds cell voltage / temperature data received from the BMS over CAN,
 * plus display-update logic for the 16x2 LCD.
 *
 * BMS CAN TX IDs (from mk11-bms-mcu fdcan.h):
 *   0x6B0  CURR_VOLTAGE_SOC    — Pack current, pack voltage, SOC
 *   0x6B1  DCL_CCL_TEMP        — DCL, CCL, temperature
 *   0x6B2  HIGH_LOW_CELL_VOLTAGE — Highest / lowest cell voltages
 */

#ifndef INC_BMS_DATA_H_
#define INC_BMS_DATA_H_

#include "stm32l4xx_hal.h"
#include <stdint.h>

/* ---------- Display refresh interval (ms) ---------- */
#define DISPLAY_CYCLE_MS    2000

/* ---------- Number of display screens ---------- */
#define DISPLAY_NUM_SCREENS 3

/* ---------- BMS Data Structure ---------- */
typedef struct {
    /* Cell voltages (stored as V * 10000, e.g. 3.800V = 38000) */
    uint16_t highest_cell_voltage;  /* From CAN 0x6B2 */
    uint16_t lowest_cell_voltage;   /* From CAN 0x6B2 */
    uint16_t avg_cell_voltage;      /* Computed or from CAN */

    /* Temperatures (stored as °C * 10, e.g. 45.0°C = 450) */
    int16_t highest_temp;           /* From CAN 0x6B1 */
    int16_t lowest_temp;            /* From CAN 0x6B1 */
    int16_t avg_temp;               /* Computed or from CAN */

    /* Pack-level (stored as V * 10, e.g. 350.5V = 3505) */
    uint16_t pack_voltage;          /* From CAN 0x6B0 */

    /* Pack current (stored as A * 10, e.g. 12.5A = 125) */
    int16_t pack_current;           /* From CAN 0x6B0 */

    /* State of Charge (stored as % * 10, e.g. 85.0% = 850) */
    uint16_t soc;                   /* From CAN 0x6B0 */

    /* CAN debug */
    uint32_t can_rx_count;
} BMS_Data_t;

typedef union VOLTAGE_DF {
    struct __attribute__((packed)) {
        uint16_t lowest_cell_voltage;  /* Bytes 0-1: Low Cell Voltage (Big-Endian) */
        uint16_t highest_cell_voltage; /* Bytes 2-3: High Cell Voltage (Big-Endian) */
        uint8_t reserved4;             /* Byte 4: Blank */
        uint8_t reserved5;             /* Byte 5: Blank */
        uint8_t reserved6;             /* Byte 6: Blank */
        uint8_t crc;                   /* Byte 7: CRC Checksum */
    } data;
    uint8_t array[8];
} VOLTAGE_DF;

typedef union TEMP_DF {
    struct __attribute__((packed)) {
        uint16_t dcl;           /* Bytes 0-1: Pack DCL (Big-Endian) */
        uint8_t ccl;            /* Byte 2: Pack CCL */
        uint8_t reserved3;      /* Byte 3: Blank */
        int8_t highest_temp;    /* Byte 4: High Temperature */
        int8_t lowest_temp;     /* Byte 5: Low Temperature */
        uint8_t reserved6;      /* Byte 6: Blank */
        uint8_t crc;            /* Byte 7: CRC Checksum */
    } data;
    uint8_t array[8];
} TEMP_DF;

typedef union SOC_CURR_PACK_DF {
    struct __attribute__((packed)) {
        int16_t curr;           /* Bytes 0-1: Pack Current (Big-Endian) */
        uint16_t pack_voltage;  /* Bytes 2-3: Pack Summed Voltage (Big-Endian) */
        uint8_t soc;            /* Byte 4: Pack SOC */
        uint8_t relays_1;       /* Byte 5: Relays and bits */
        uint8_t relays_2;       /* Byte 6: Relays and bits */
        uint8_t crc;            /* Byte 7: CRC Checksum */
    } data;
    uint8_t array[8];
} SOC_CURR_PACK_DF;

/* Global BMS data instance */
extern volatile BMS_Data_t bms_data;

/* ---------- API ---------- */

/**
 * @brief  Initialise bms_data with placeholder values for display testing.
 */
void BMS_Data_Init(void);

/**
 * @brief  Update the LCD with BMS data.  Internally manages screen cycling
 *         using HAL_GetTick() so it can be called every main-loop iteration.
 * @param  hi2c  Pointer to the I2C handle for the LCD.
 */
void BMS_UpdateDisplay(I2C_HandleTypeDef *hi2c);

#endif /* INC_BMS_DATA_H_ */
