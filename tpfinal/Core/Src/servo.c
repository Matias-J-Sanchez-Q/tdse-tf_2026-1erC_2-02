/**
  ******************************************************************************
  * @file    servo.c
  * @brief   Driver del servo SG90 del cerrojo (TIM4_CH1 / PB6, PWM 50 Hz)
  ******************************************************************************
  */
#include "servo.h"
#include "board.h"

TIM_HandleTypeDef htim4;

void Servo_Init(void)
{
  GPIO_InitTypeDef gpio = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  uint32_t timer_clk_hz;
  uint32_t prescaler;

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_TIM4_CLK_ENABLE();

  /* PB6 = TIM4_CH1, salida alternativa push-pull */
  gpio.Pin   = SERVO_PIN;
  gpio.Mode  = GPIO_MODE_AF_PP;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SERVO_PORT, &gpio);

  /* El prescaler se calcula a partir del reloj real de APB1 en vez de
     asumir una frecuencia fija: asi el PWM sigue siendo de 50 Hz aunque
     se cambie la configuracion del reloj del sistema.
     TIM4 cuelga de APB1; si el divisor de APB1 != 1, el timer corre al doble. */
  timer_clk_hz = HAL_RCC_GetPCLK1Freq();
  if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
  {
    timer_clk_hz *= 2u;
  }

  /* Busco una base de tiempo de 1 MHz => cada tick del timer es 1 us */
  prescaler = (timer_clk_hz / 1000000u) - 1u;

  htim4.Instance               = TIM4;
  htim4.Init.Prescaler         = prescaler;
  htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim4.Init.Period            = 19999;   /* 20000 us = 20 ms = 50 Hz */
  htim4.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode     = TIM_OCMODE_PWM1;
  sConfigOC.Pulse      = 500;             /* ~0 grados */
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
}

void Servo_SetAngle(uint8_t deg)
{
  uint16_t pulse_us;

  if (deg > 180u)
  {
    deg = 180u;
  }

  pulse_us = (uint16_t)(500u + (((uint32_t)deg * 2000u) / 180u));

  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, pulse_us);
}
