/*
 * task_actuator_attribute.h
 * Eventos, estados y datos del Task Actuator (todas las salidas del sistema)
 */

#ifndef TASK_ACTUATOR_ATTRIBUTE_H_
#define TASK_ACTUATOR_ATTRIBUTE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "main.h"
#include <stdbool.h>

/********************** typedef **********************************************/

/* Eventos que excitan al Task Actuator */
typedef enum task_actuator_ev
{
	EV_ACT_OFF,			/* apagar / cerrar traba                           */
	EV_ACT_ON,			/* encender / abrir traba (queda asi)              */
	EV_ACT_PULSE,		/* encender por tick_max ms y apagarse solo        */
	EV_ACT_BLINK		/* parpadear con semiperiodo tick_max (solo GPIO)  */
} task_actuator_ev_t;

/* Estados del Task Actuator */
typedef enum task_actuator_st
{
	ST_ACT_OFF,
	ST_ACT_ON,
	ST_ACT_PULSE,
	ST_ACT_BLINK
} task_actuator_st_t;

/* Que clase de salida fisica es */
typedef enum task_actuator_kind
{
	ACT_GPIO,			/* pin digital on/off                              */
	ACT_SERVO			/* servo del cerrojo (PWM): OFF=trabado, ON=abierto */
} task_actuator_kind_t;

/* Actuadores del sistema */
typedef enum task_actuator_id
{
	ID_LED_STATUS,		/* LD2 (PA5): estado del sistema        */
	ID_ALARM,			/* PA4: buzzer / sirena                 */
	ID_OK,				/* PA6: salida "clave correcta"         */
	ID_LOCK				/* PB6: servo del cerrojo               */
} task_actuator_id_t;

/* Configuracion (ROM), una entrada por actuador */
typedef struct
{
	task_actuator_id_t		identifier;
	task_actuator_kind_t	kind;
	GPIO_TypeDef *			gpio_port;	/* solo si kind == ACT_GPIO */
	uint16_t				pin;		/* solo si kind == ACT_GPIO */
	GPIO_PinState			act_on;
	GPIO_PinState			act_off;
	uint32_t				tick_max;	/* duracion del pulso / semiperiodo del blink, en ms */
} task_actuator_cfg_t;

/* Datos dinamicos (RAM), una entrada por actuador */
typedef struct
{
	uint32_t				tick;
	task_actuator_st_t		state;
	task_actuator_ev_t		event;
	bool					flag;		/* hay un evento sin procesar          */
	bool					level;		/* nivel logico actual (para el blink) */
} task_actuator_dta_t;

/********************** external data declaration ****************************/
extern task_actuator_dta_t task_actuator_dta_list[];

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_ACTUATOR_ATTRIBUTE_H_ */

/********************** end of file ******************************************/
