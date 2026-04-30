/*
 * lcd_i2c.h
 *
 * HD44780 16x2 LCD driver over I2C via PCF8574 backpack (CN0295D)
 *
 * PCF8574 pin mapping:
 *   P0 = RS
 *   P1 = RW
 *   P2 = EN
 *   P3 = Backlight
 *   P4 = D4
 *   P5 = D5
 *   P6 = D6
 *   P7 = D7
 */

#ifndef INC_LCD_I2C_H_
#define INC_LCD_I2C_H_

#include "stm32l4xx_hal.h"
#include <stdint.h>

/* ---------- I2C Address ---------- */
#define LCD_I2C_ADDR        (0x27 << 1)   /* 7-bit 0x27, left-shifted for HAL */

/* ---------- HD44780 Commands ---------- */
#define LCD_CMD_CLEAR       0x01
#define LCD_CMD_HOME        0x02
#define LCD_CMD_ENTRY_MODE  0x06          /* Increment cursor, no shift */
#define LCD_CMD_DISPLAY_ON  0x0C          /* Display ON, cursor OFF, blink OFF */
#define LCD_CMD_FUNCTION_4B 0x28          /* 4-bit, 2 lines, 5x8 font */
#define LCD_CMD_SET_DDRAM   0x80          /* OR with address */

/* ---------- PCF8574 Control Bits ---------- */
#define LCD_PIN_RS          (1 << 0)
#define LCD_PIN_RW          (1 << 1)
#define LCD_PIN_EN          (1 << 2)
#define LCD_PIN_BL          (1 << 3)

/* ---------- API ---------- */

/**
 * @brief  Initialise the LCD. Must be called after HAL_I2C_Init().
 * @param  hi2c  Pointer to the I2C handle connected to the LCD.
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef LCD_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief  Clear the display and return cursor to home.
 */
void LCD_Clear(void);

/**
 * @brief  Set cursor position.
 * @param  row  0 or 1
 * @param  col  0..15
 */
void LCD_SetCursor(uint8_t row, uint8_t col);

/**
 * @brief  Write a single character at the current cursor position.
 */
void LCD_WriteChar(char c);

/**
 * @brief  Write a null-terminated string starting at the current cursor position.
 */
void LCD_WriteString(const char *str);

/**
 * @brief  Turn the backlight on.
 */
void LCD_BacklightOn(void);

/**
 * @brief  Turn the backlight off.
 */
void LCD_BacklightOff(void);

/**
 * @brief  Send a command byte to the LCD.
 */
void LCD_SendCommand(uint8_t cmd);

/**
 * @brief  Send a data byte to the LCD.
 */
void LCD_SendData(uint8_t data);

#endif /* INC_LCD_I2C_H_ */
