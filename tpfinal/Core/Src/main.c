/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Cerradura electronica - Bare Metal / Event-Triggered System
  ******************************************************************************
  *
  * Este archivo hace UNA sola cosa: inicializar el hardware y arrancar el
  * ejecutor ciclico. Toda la logica de la aplicacion vive en app/ , repartida
  * en tareas no bloqueantes:
  *
  *   app/src/app.c            ejecutor ciclico (tick 1 ms, WFI, WCET, U)
  *   app/src/task_sensor.c    escrutar entradas digitales (antirrebote)
  *   app/src/task_analog.c    escrutar entradas analogicas (ADC1/ADC2)
  *   app/src/task_system.c    procesar: FSM de la cerradura
  *   app/src/task_actuator.c  actuar: LED, alarma, salida OK, servo
  *   app/src/task_display.c   actuar: LCD 16x2 I2C (no bloqueante)
  *   app/src/task_storage.c   actuar: EEPROM AT24C32 (no bloqueante)
  *
  * El mapa de pines esta en app/inc/board.h. Aca no hay ni un numero de pin.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "board.h"
#include "app.h"
#include "servo.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef  hadc1;	/* potenciometro         (PA0 / ADC1_IN0) */
ADC_HandleTypeDef  hadc2;	/* monitor de tension    (PA7 / ADC2_IN7) */
I2C_HandleTypeDef  hi2c1;	/* LCD + DS3231 + AT24C32                 */
UART_HandleTypeDef huart2;	/* consola de logs                        */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void MX_I2C1_Init(void);

/**
  * @brief  Punto de entrada.
  */
int main(void)
{
  /* Reset de perifericos, flash y SysTick (1 ms) */
  HAL_Init();

  SystemClock_Config();

  /* --- Hardware ----------------------------------------------------------- */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_I2C1_Init();
  Servo_Init();

  /* --- Aplicacion --------------------------------------------------------- */
  app_init();

  /* --- Ejecutor ciclico --------------------------------------------------- */
  while (1)
  {
    app_update();
  }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /* --- SYSCLK = 64 MHz ---------------------------------------------------
     HSI (8 MHz) / 2 = 4 MHz  ->  PLL x16  =  64 MHz

     Antes estaba en PLL x2, o sea 8 MHz: el micro corria a 1/8 de lo que
     puede. Con eso, cualquier vuelta del ejecutor ciclico con algo de trabajo
     se pasaba del deadline de 1 ms (U > 100%).

     64 MHz es el maximo alcanzable con el HSI interno (para 72 MHz hace falta
     el cristal externo HSE, que la Nucleo trae por ST-LINK MCO). */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;	/* HCLK  = 64 MHz */
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;		/* PCLK1 = 32 MHz (max 36) */
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;		/* PCLK2 = 64 MHz */

  /* A 64 MHz la Flash necesita 2 wait states (0 WS solo hasta 24 MHz).
     Con FLASH_LATENCY_0 a 64 MHz el micro lee basura y se cuelga. */
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  /* El ADC no puede pasar de 14 MHz: PCLK2 (64) / 6 = 10,67 MHz.
     Con el DIV2 anterior habrian sido 32 MHz y las conversiones saldrian mal. */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1: potenciometro (PA0 = ADC1_IN0)
  */
static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = POT_ADC_CHANNEL;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC2: monitor de tension 0-3,3V (PA7 = ADC2_IN7)
  */
static void MX_ADC2_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_ADC2_CLK_ENABLE();

  gpio.Pin  = VMON_PIN;
  gpio.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(VMON_PORT, &gpio);

  hadc2.Instance = ADC2;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = VMON_ADC_CHANNEL;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1: LCD (PCF8574) + RTC DS3231 + EEPROM AT24C32
  *
  * A 400 kHz (Fast Mode). A 100 kHz una transaccion del LCD se comia casi
  * todo el presupuesto de 1 ms de la vuelta del ejecutor ciclico; a 400 kHz
  * baja a ~1/4 y el WCET queda con margen de sobra. Los tres esclavos del
  * bus soportan Fast Mode.
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2: consola de logs (115200 8N1)
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO. Todos los pines salen de board.h.
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* --- Salidas: arrancan todas en reposo -------------------------------- */
  HAL_GPIO_WritePin(LED_STATUS_PORT, LED_STATUS_PIN, LED_STATUS_OFF);
  HAL_GPIO_WritePin(ALARM_PORT,      ALARM_PIN,      ALARM_OFF);
  HAL_GPIO_WritePin(OK_PORT,         OK_PIN,         OK_OFF);

  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  GPIO_InitStruct.Pin = LED_STATUS_PIN;
  HAL_GPIO_Init(LED_STATUS_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = ALARM_PIN;
  HAL_GPIO_Init(ALARM_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = OK_PIN;
  HAL_GPIO_Init(OK_PORT, &GPIO_InitStruct);

  /* --- Entradas por POLLING (las lee task_sensor con antirrebote) -------- */
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;

  GPIO_InitStruct.Pin = BTN_CONFIRM_PIN;
  HAL_GPIO_Init(BTN_CONFIRM_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = DOOR_PIN;
  HAL_GPIO_Init(DOOR_PORT, &GPIO_InitStruct);

  /* --- Entrada por INTERRUPCION: boton azul B1 (PC13) --------------------
     Activo en bajo => la pulsacion es un flanco DESCENDENTE. */
  GPIO_InitStruct.Pin  = BTN_MODE_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_MODE_PORT, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* USER CODE BEGIN 4 */
/**
  * @brief Callback de la EXTI (boton azul B1).
  *
  * Lo unico que hace es ENCOLAR el evento y volver: la ISR es cortita y toda
  * la logica queda del lado de las tareas. El antirrebote aca es por tiempo
  * (200 ms) porque un pulsador mecanico genera decenas de flancos por pulsada
  * y no quiero llenar la cola de eventos.
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  static uint32_t last_press_tick = 0;
  uint32_t now = HAL_GetTick();

  if (GPIO_Pin == BTN_MODE_PIN)
  {
    if ((now - last_press_tick) > 200u)
    {
      last_press_tick = now;
      put_event_task_system(EV_SYS_BTN_MODE);
    }
  }
}
/* USER CODE END 4 */

/**
  * @brief  Se ejecuta ante un error irrecuperable.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
