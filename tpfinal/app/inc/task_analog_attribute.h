/*
 * task_analog_attribute.h - Estados y datos del Task Analog
 */

#ifndef TASK_ANALOG_ATTRIBUTE_H_
#define TASK_ANALOG_ATTRIBUTE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Los canales se leen de a uno por tick, alternando, asi ninguna vuelta del
   ejecutor paga las dos conversiones juntas. */
typedef enum task_analog_st
{
	ST_ANA_POT,		/* este tick le toca al potenciometro (ADC1_IN0)  */
	ST_ANA_VMON		/* este tick le toca al monitor de tension (ADC2_IN7) */
} task_analog_st_t;

typedef struct
{
	task_analog_st_t	state;

	uint16_t			pot_raw;		/* 0..4095, valor filtrado           */
	uint16_t			pot_ref;		/* referencia para detectar movimiento */
	uint8_t				pot_digit;		/* 0..9 derivado de pot_raw          */

	uint16_t			vmon_raw;		/* 0..4095                           */
	uint16_t			vmon_mv;		/* 0..3300 mV                        */
	bool				overvoltage;	/* estado actual del comparador      */
} task_analog_dta_t;

extern task_analog_dta_t task_analog_dta;

#ifdef __cplusplus
}
#endif

#endif /* TASK_ANALOG_ATTRIBUTE_H_ */
