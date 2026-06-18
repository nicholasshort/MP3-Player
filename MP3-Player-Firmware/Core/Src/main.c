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
#include "adc.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "i2s.h"
#include "spi.h"
#include "tim.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bq24259.h"
#include "buttons.h"
#include "status_leds.h"
#include "ws2812b.h"
#include "sd_card_spi.h"
#include "tad5242.h"

#include <stdint.h>
#include <string.h>
#include <math.h>
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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */



/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2S1_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_ADC1_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  bq24259_init();
  buttons_init();
  status_leds_init();
  ws2812b_init();
  tad5242_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // bq24259_status_t status;
  // bq24259_fault_t fault;
  status_leds_set_mode(STATUS_LED_GREEN, STATUS_LED_MODE_BLINK_SLOW);
  status_leds_set_mode(STATUS_LED_BLUE, STATUS_LED_MODE_BLINK_FAST);

  ws2812b_set_colour(WS2812B_LED_LEFT, 255, 0, 0);
  ws2812b_set_colour(WS2812B_LED_RIGHT, 0, 0, 255);
  ws2812b_set_brightness(WS2812B_LED_LEFT, 30);
  ws2812b_set_brightness(WS2812B_LED_RIGHT, 30);
  ws2812b_update();
  
  // Test FatFs
#define LORN_WAV_DATA_CHUNK_OFFSET 204
#define LORN_WAV_DATA_CHUNK_SIZE   8155940
#define LORN_WAV_DATA_CHUNK_END    8156144
  FATFS fs; // Logical Drive
  FIL file;
  FRESULT fr;
  fr = f_mount(&fs, "", 1);
  if (fr != FR_OK) {
      HAL_Delay(100);
      HAL_NVIC_SystemReset();
  }
  
  fr = f_open(&file, "lorn_drawn_out_like_an_ache.wav", FA_READ);
  if (fr != FR_OK)
    Error_Handler();

  fr = f_lseek(&file, LORN_WAV_DATA_CHUNK_OFFSET); // Move cursor to start of data chunk
  if (fr != FR_OK)
    Error_Handler();

  tad5242_start();

  bool get_new_audio = true;
  static int16_t pcm[2048]; 
  
  while (1)
  {

    if (get_new_audio) {
        UINT bytes_read;
        uint32_t bytes_to_read = sizeof(pcm);
        if (bytes_to_read > LORN_WAV_DATA_CHUNK_END - f_tell(&file))
          bytes_to_read = LORN_WAV_DATA_CHUNK_END - f_tell(&file);
        fr = f_read(&file, pcm, bytes_to_read, &bytes_read);
        if (fr != FR_OK) {
          (void)tad5242_stop();
          Error_Handler();
        }
        if (bytes_read < sizeof(pcm)){
          memset((uint8_t*)pcm + bytes_read, 0, sizeof(pcm) - bytes_read);

          fr = f_lseek(&file, LORN_WAV_DATA_CHUNK_OFFSET); // Return cursor pointer back to start of data chunk
          if (fr != FR_OK)
            Error_Handler();
        }
        get_new_audio = false;
    }


    int32_t* buffer;
    uint32_t frame_count;
    if (tad5242_get_audio_buffer(&buffer, &frame_count) == TAD5242_STREAM_OK) {

        for (uint32_t i = 0; i < frame_count; i++) {
          int32_t left32  = (int32_t)(pcm[2u * i + 0u] / 10);
          int32_t right32 = (int32_t)(pcm[2u * i + 1u] / 10);

          buffer[2u * i + 0u] = (int32_t)(((uint32_t)left32) << 16);
          buffer[2u * i + 1u] = (int32_t)(((uint32_t)right32) << 16);
        }

        tad5242_commit_audio_buffer();

        get_new_audio = true; 

    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 7;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
