/**
  ******************************************************************************
  * @file    servo.h
  * @brief   Driver del servo SG90 del cerrojo (TIM4_CH1 / PB6, PWM 50 Hz).
  *          Es un driver puro: no conoce la logica de la cerradura, solo
  *          sabe poner el eje en un angulo. Todas sus funciones son NO
  *          bloqueantes (escriben un registro de comparacion del timer).
  ******************************************************************************
  */
#ifndef SERVO_H
#define SERVO_H

#include "main.h"

/* Arma TIM4_CH1 y arranca el PWM. Llamar una vez, antes del loop. */
void Servo_Init(void);

/* Manda el eje a un angulo 0..180 grados (500us..2500us de ancho de pulso) */
void Servo_SetAngle(uint8_t deg);

#endif /* SERVO_H */
