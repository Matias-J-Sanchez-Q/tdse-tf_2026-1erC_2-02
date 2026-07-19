/*
 * task_sensor_attribute.h
 * Eventos, estados y datos del Task Sensor (entradas digitales antirrebote)
 */

#ifndef TASK_SENSOR_ATTRIBUTE_H_
#define TASK_SENSOR_ATTRIBUTE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "main.h"
#include "task_system_attribute.h"

/********************** typedef **********************************************/

/* Eventos internos del antirrebote */
typedef enum task_sensor_ev
{
	EV_BTN_UP,
	EV_BTN_DOWN
} task_sensor_ev_t;

/* Estados del antirrebote (FSM de 4 estados) */
typedef enum task_sensor_st
{
	ST_BTN_UP,
	ST_BTN_FALLING,
	ST_BTN_DOWN,
	ST_BTN_RISING
} task_sensor_st_t;

/* Sensores digitales del sistema.
   OJO: el boton azul B1 NO esta aca porque se gestiona por INTERRUPCION
   (EXTI), no por polling. Ver HAL_GPIO_EXTI_Callback en main.c.            */
typedef enum task_sensor_id
{
	ID_BTN_CONFIRM,		/* PB0: confirma el digito seleccionado */
	ID_DOOR				/* PA1: reed switch de la puerta        */
} task_sensor_id_t;

/* Configuracion (ROM), una entrada por sensor fisico */
typedef struct
{
	task_sensor_id_t	identifier;
	GPIO_TypeDef *		gpio_port;
	uint16_t			pin;
	GPIO_PinState		active_level;	/* nivel que cuenta como "activo"   */
	uint32_t			tick_max;		/* ventana de antirrebote, en ms    */
	task_system_ev_t	signal_down;	/* evento al pasar a activo         */
	task_system_ev_t	signal_up;		/* evento al pasar a inactivo       */
} task_sensor_cfg_t;

/* Datos dinamicos (RAM), una entrada por sensor fisico */
typedef struct
{
	uint32_t			tick;
	task_sensor_st_t	state;
	task_sensor_ev_t	event;
} task_sensor_dta_t;

/********************** external data declaration ****************************/
extern task_sensor_dta_t task_sensor_dta_list[];

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_SENSOR_ATTRIBUTE_H_ */

/********************** end of file ******************************************/
