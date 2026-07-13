/*
 * board.h - Mapa de pines del prototipo (Cerradura electronica)
 * Placa: NUCLEO-F103RB / RC
 *
 * Toda la asignacion fisica de pines vive ACA. Ningun task_*.c debe
 * tener numeros de pin hardcodeados.
 */

#ifndef BOARD_H_
#define BOARD_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "main.h"

/********************** macros ***********************************************/
#define NUCLEO_F103RC		(0)
#define BOARD				(NUCLEO_F103RC)

/* ---------------------------------------------------------------------------
 * ENTRADAS DIGITALES
 * ------------------------------------------------------------------------ */

/* BTN_MODE: boton azul de usuario B1 (PC13). Gestionado por INTERRUPCION (EXTI).
   En la Nucleo tiene pull-up externo: suelto = 1, apretado = 0.            */
#define BTN_MODE_PIN		B1_Pin			/* PC13 */
#define BTN_MODE_PORT		B1_GPIO_Port
#define BTN_MODE_PRESSED	GPIO_PIN_RESET

/* BTN_CONFIRM: pulsador entre PB0 y GND. Gestionado por POLLING + antirrebote.
   Pull-up interno: suelto = 1, apretado = 0.                               */
#define BTN_CONFIRM_PIN		GPIO_PIN_0
#define BTN_CONFIRM_PORT	GPIOB
#define BTN_CONFIRM_PRESSED	GPIO_PIN_RESET

/* DOOR: reed switch de puerta en PA1, a GND, con pull-up interno.
   Puerta cerrada (iman presente, contacto cerrado) = 0
   Puerta abierta                                   = 1                     */
#define DOOR_PIN			GPIO_PIN_1
#define DOOR_PORT			GPIOA
#define DOOR_OPEN_LEVEL		GPIO_PIN_SET

/* ---------------------------------------------------------------------------
 * SALIDAS DIGITALES
 * ------------------------------------------------------------------------ */

/* LED de estado: LD2 de la Nucleo (PA5) */
#define LED_STATUS_PIN		LD2_Pin			/* PA5 */
#define LED_STATUS_PORT		LD2_GPIO_Port
#define LED_STATUS_ON		GPIO_PIN_SET
#define LED_STATUS_OFF		GPIO_PIN_RESET

/* Alarma / buzzer (PA4). Activo en alto. */
#define ALARM_PIN			GPIO_PIN_4
#define ALARM_PORT			GPIOA
#define ALARM_ON			GPIO_PIN_SET
#define ALARM_OFF			GPIO_PIN_RESET

/* Salida "clave OK" (PA6). Activo en alto, 3,3V. */
#define OK_PIN				GPIO_PIN_6
#define OK_PORT				GPIOA
#define OK_ON				GPIO_PIN_SET
#define OK_OFF				GPIO_PIN_RESET

/* ---------------------------------------------------------------------------
 * ENTRADAS ANALOGICAS
 * ------------------------------------------------------------------------ */

/* Potenciometro de seleccion de digito / opcion de menu: PA0 = ADC1_IN0 */
#define POT_ADC_CHANNEL		ADC_CHANNEL_0

/* Monitor de tension 0-3,3V (deteccion de sabotaje): PA7 = ADC2_IN7 */
#define VMON_PIN			GPIO_PIN_7
#define VMON_PORT			GPIOA
#define VMON_ADC_CHANNEL	ADC_CHANNEL_7

/* Umbral de disparo de la alarma por sobretension, en mV */
#define VMON_ALARM_MV		(1000u)

/* ---------------------------------------------------------------------------
 * SERVO DEL CERROJO (PB6 = TIM4_CH1, PWM 50 Hz)
 * ------------------------------------------------------------------------ */
#define SERVO_PIN			GPIO_PIN_6
#define SERVO_PORT			GPIOB
#define SERVO_CLOSED_DEG	(20u)
#define SERVO_OPEN_DEG		(90u)

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* BOARD_H_ */

/********************** end of file ******************************************/
