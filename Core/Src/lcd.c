/*
 * lcd_i2c.c
 *
 * HD44780 16x2 LCD driver over I2C via PCF8574 backpack (CN0295D)
 *
 * All data is sent in 4-bit mode.  Each byte is transmitted as two nibbles,
 * each latched by toggling the Enable (EN) pin via I2C writes.
 */

#include "lcd.h"

/* ---------- Private State ---------- */
static I2C_HandleTypeDef *lcd_i2c_handle;
static uint8_t lcd_backlight = LCD_PIN_BL;   /* Backlight ON by default */

/* ---------- Low-level helpers ---------- */

/**
 * @brief  Write a single byte to the PCF8574.
 */
static void LCD_I2C_Write(uint8_t data)
{
    HAL_I2C_Master_Transmit(lcd_i2c_handle, LCD_I2C_ADDR, &data, 1, HAL_MAX_DELAY);
}

/**
 * @brief  Pulse the EN pin to latch data already on D4-D7.
 *         The nibble value must already include RS/BL flags.
 */
static void LCD_PulseEnable(uint8_t nibble)
{
    LCD_I2C_Write(nibble | LCD_PIN_EN);      /* EN = 1 */
    HAL_Delay(1);                            /* Hold time */
    LCD_I2C_Write(nibble & ~LCD_PIN_EN);     /* EN = 0 */
    HAL_Delay(1);
}

/**
 * @brief  Send a 4-bit nibble with RS flag.
 * @param  nibble  Upper 4 bits contain the data (D4-D7).
 * @param  rs      0 for command, LCD_PIN_RS for data.
 */
static void LCD_WriteNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble & 0xF0) | rs | lcd_backlight;
    LCD_PulseEnable(data);
}

/**
 * @brief  Send a full byte (two nibbles) with RS flag.
 */
static void LCD_WriteByte(uint8_t byte, uint8_t rs)
{
    LCD_WriteNibble(byte & 0xF0, rs);              /* High nibble */
    LCD_WriteNibble((byte << 4) & 0xF0, rs);       /* Low nibble  */
}

/* ---------- Public API ---------- */

HAL_StatusTypeDef LCD_Init(I2C_HandleTypeDef *hi2c)
{
    lcd_i2c_handle = hi2c;

    /* Wait >40 ms after power-on (HD44780 spec) */
    HAL_Delay(50);

    /* --- Begin 4-bit initialisation sequence (per HD44780 datasheet) --- */

    /* Send 0x30 three times to reliably enter 8-bit mode first */
    LCD_WriteNibble(0x30, 0);
    HAL_Delay(5);                   /* Wait >4.1 ms */
    LCD_WriteNibble(0x30, 0);
    HAL_Delay(5);
    LCD_WriteNibble(0x30, 0);
    HAL_Delay(1);

    /* Now switch to 4-bit mode */
    LCD_WriteNibble(0x20, 0);
    HAL_Delay(1);

    /* From here on we can use full-byte (two nibble) writes */
    LCD_SendCommand(LCD_CMD_FUNCTION_4B);   /* 4-bit, 2 lines, 5x8 */
    LCD_SendCommand(LCD_CMD_DISPLAY_ON);    /* Display ON, cursor OFF */
    LCD_SendCommand(LCD_CMD_CLEAR);         /* Clear display */
    HAL_Delay(2);                           /* Clear needs >1.52 ms */
    LCD_SendCommand(LCD_CMD_ENTRY_MODE);    /* Increment, no shift */

    return HAL_OK;
}

void LCD_SendCommand(uint8_t cmd)
{
    LCD_WriteByte(cmd, 0);          /* RS = 0 → command */
}

void LCD_SendData(uint8_t data)
{
    LCD_WriteByte(data, LCD_PIN_RS); /* RS = 1 → data */
}

void LCD_Clear(void)
{
    LCD_SendCommand(LCD_CMD_CLEAR);
    HAL_Delay(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? col : (0x40 + col);
    LCD_SendCommand(LCD_CMD_SET_DDRAM | addr);
}

void LCD_WriteChar(char c)
{
    LCD_SendData((uint8_t)c);
}

void LCD_WriteString(const char *str)
{
    while (*str)
    {
        LCD_SendData((uint8_t)*str++);
    }
}

void LCD_BacklightOn(void)
{
    lcd_backlight = LCD_PIN_BL;
    LCD_I2C_Write(lcd_backlight);
}

void LCD_BacklightOff(void)
{
    lcd_backlight = 0;
    LCD_I2C_Write(lcd_backlight);
}
