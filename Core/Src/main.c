/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "gpio.h"
#include "i2c.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bms_data.h"
#include "lcd.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint8_t can_message_count = 0;

uint8_t i2c_found_addr = 0;
uint8_t i2c_device_count = 0;
HAL_StatusTypeDef lcd_init_status;

VOLTAGE_DF latest_voltages;
TEMP_DF latest_temps;
SOC_CURR_PACK_DF latest_pack_data;

/* CAN Rx Buffers */
CAN_RxHeaderTypeDef RxHeader;
uint8_t RxData[8];

CAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8];
uint32_t TxMailbox;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK) {
    return;
  }

  bms_data.can_rx_count++;

  switch (RxHeader.StdId) {
  case 0x6B0: // SOC, Current, Pack Voltage
    /* DBC layout (Motorola-aligned bytes):
     * bytes 0..1: Pack_Current (16 bit, signed, scale 0.1 A)
     * bytes 2..3: Pack_Summed_Voltage (16 bit, unsigned, scale 0.1 V)
     * byte  4:    Pack_SOC (8 bit, unsigned, scale 0.5 %)
     * remaining: various 1-bit flags / CRC we ignore here
     *
     * Convert to in-code units:
     *  - pack_current stored as A * 10  => raw_signed (because raw*0.1*10 ==
     * raw)
     *  - pack_voltage stored as V * 10  => raw_unsigned (raw*0.1*10 == raw)
     *  - soc stored as % * 10           => raw_soc * 5 (raw*0.5*10 == raw*5)
     */
    {
      int16_t raw_curr = (int16_t)((RxData[0] << 8) | RxData[1]);
      uint16_t raw_volt = (uint16_t)((RxData[2] << 8) | RxData[3]);
      uint8_t raw_soc8 = RxData[4];

      bms_data.pack_current =
          raw_curr; /* already in tenths (raw*0.1*10 == raw) */
      bms_data.pack_voltage =
          raw_volt; /* already in tenths (raw*0.1*10 == raw) */
      bms_data.soc =
          (uint16_t)raw_soc8 * 5u; /* raw * 0.5% -> times 10 => raw*5 */
    }
    break;

  case 0x6B1: // Temperatures
    /* DBC layout (Motorola style):
     * bytes 0..1: Pack_DCL (not used here)
     * byte 2:   Pack_CCL (not used)
     * byte 3:   reserved
     * byte 4:   High_Temperature (8-bit, signed, units C)
     * byte 5:   Low_Temperature  (8-bit, signed, units C)
     * bytes 6..7: CRC / reserved
     *
     * The code expects temps in tenths of degrees (°C * 10). The DBC temps are
     * whole °C so multiply by 10. avg_temp is not provided by this DBC message,
     * compute as mean.
     */
    {
      int8_t high_c = (int8_t)RxData[4];
      int8_t low_c = (int8_t)RxData[5];

      bms_data.highest_temp = (int16_t)high_c * 10;
      bms_data.lowest_temp = (int16_t)low_c * 10;
      bms_data.avg_temp = (int16_t)(((int32_t)bms_data.highest_temp +
                                     (int32_t)bms_data.lowest_temp) /
                                    2);
    }
    break;

  case 0x6B2: // Cell Voltages
    /* DBC layout (Motorola style):
     * bytes 0..1: Low_Cell_Voltage (uint16, scale 1e-4 V)
     * bytes 2..3: High_Cell_Voltage (uint16, scale 1e-4 V)
     * remaining bytes: reserved / CRC
     *
     * The code stores cell voltages as V * 10000, which matches raw (raw * 1e-4
     * * 10000 == raw). Compute avg as mean of low/high.
     */
    {
      uint16_t raw_low = (uint16_t)((RxData[0] << 8) | RxData[1]);
      uint16_t raw_high = (uint16_t)((RxData[2] << 8) | RxData[3]);

      bms_data.lowest_cell_voltage = raw_low;
      bms_data.highest_cell_voltage = raw_high;
      bms_data.avg_cell_voltage =
          (uint16_t)(((uint32_t)raw_low + (uint32_t)raw_high) / 2);
    }
    break;
  }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN1_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  // CAN SETUP
  CAN_FilterTypeDef sFilterConfig;
  sFilterConfig.FilterBank = 0;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh = 0x0000;
  sFilterConfig.FilterIdLow = 0x0000;
  sFilterConfig.FilterMaskIdHigh = 0x0000;
  sFilterConfig.FilterMaskIdLow = 0x0000;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
  HAL_CAN_Start(&hcan1);
  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

  // TEST CAN TX - Commented out since this is a listen-only display
  /*
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.StdId = 0xAB;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.DLC = 2;
  TxData[0] = 0xAA;
  TxData[1] = 0xBB;
  */

  // I2C Setup
  i2c_device_count = 0;
  i2c_found_addr = 0;
  for (uint8_t addr = 1; addr < 128; addr++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (addr << 1), 2, 10) == HAL_OK) {
      i2c_found_addr = addr;
      i2c_device_count++;
    }
  }
  lcd_init_status = LCD_Init(&hi2c1);

  // SPLASH SCREEN
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_WriteString("please work");
  LCD_SetCursor(1, 0);
  LCD_WriteString("T-T");
  HAL_Delay(3000);
  LCD_Clear();

  BMS_Data_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {

    /*
    TxData[0] = can_message_count;
    if (HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox) == HAL_OK)
    {
      can_message_count++;
    }
    */

    BMS_UpdateDisplay(&hi2c1);

    // Blink LED to indicate alive
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
    HAL_Delay(200);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
   */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
   */
  HAL_RCCEx_EnableMSIPLLMode();
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */