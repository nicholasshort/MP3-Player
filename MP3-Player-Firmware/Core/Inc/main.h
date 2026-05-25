/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DAC_GPO_Pin GPIO_PIN_14
#define DAC_GPO_GPIO_Port GPIOC
#define LEDS_PWM_Pin GPIO_PIN_0
#define LEDS_PWM_GPIO_Port GPIOA
#define LEDS_PWR_EN_Pin GPIO_PIN_1
#define LEDS_PWR_EN_GPIO_Port GPIOA
#define BUTTON_PLAY_Pin GPIO_PIN_2
#define BUTTON_PLAY_GPIO_Port GPIOA
#define BUTTON_NEXT_Pin GPIO_PIN_3
#define BUTTON_NEXT_GPIO_Port GPIOA
#define BAT_CHARGE_Pin GPIO_PIN_0
#define BAT_CHARGE_GPIO_Port GPIOB
#define LED_GREEN_Pin GPIO_PIN_1
#define LED_GREEN_GPIO_Port GPIOB
#define LED_BLUE_Pin GPIO_PIN_2
#define LED_BLUE_GPIO_Port GPIOB
#define SD_CARD_NCS_Pin GPIO_PIN_12
#define SD_CARD_NCS_GPIO_Port GPIOB
#define SD_CARD_SCK_Pin GPIO_PIN_13
#define SD_CARD_SCK_GPIO_Port GPIOB
#define SD_CARD_MISO_Pin GPIO_PIN_14
#define SD_CARD_MISO_GPIO_Port GPIOB
#define SD_CARD_MOSI_Pin GPIO_PIN_15
#define SD_CARD_MOSI_GPIO_Port GPIOB
#define SD_CARD_NCD_Pin GPIO_PIN_8
#define SD_CARD_NCD_GPIO_Port GPIOA
#define BUTTON_PREV_PWR_Pin GPIO_PIN_9
#define BUTTON_PREV_PWR_GPIO_Port GPIOA
#define BUTTON_MENU_Pin GPIO_PIN_10
#define BUTTON_MENU_GPIO_Port GPIOA
#define BAT_NPG_Pin GPIO_PIN_4
#define BAT_NPG_GPIO_Port GPIOB
#define BAT_STAT_Pin GPIO_PIN_5
#define BAT_STAT_GPIO_Port GPIOB
#define BAT_SCL_Pin GPIO_PIN_6
#define BAT_SCL_GPIO_Port GPIOB
#define BAT_SDA_Pin GPIO_PIN_7
#define BAT_SDA_GPIO_Port GPIOB
#define BAT_NCE_Pin GPIO_PIN_8
#define BAT_NCE_GPIO_Port GPIOB
#define BAT_INT_Pin GPIO_PIN_9
#define BAT_INT_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
