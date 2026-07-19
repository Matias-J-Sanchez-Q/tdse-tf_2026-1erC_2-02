/*
 * task_system_attribute.h
 * Eventos, estados y datos del Task System (la FSM principal de la cerradura)
 */

#ifndef TASK_SYSTEM_ATTRIBUTE_H_
#define TASK_SYSTEM_ATTRIBUTE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include <stdint.h>
#include <stdbool.h>

/********************** macros ***********************************************/
#define PASSWORD_LEN		(4u)

/********************** typedef **********************************************/

/* Eventos que excitan al Task System. Los publican task_sensor, task_analog,
   task_storage y la ISR del boton azul, siempre via put_event_task_system(). */
typedef enum task_system_ev
{
	EV_SYS_NONE = 0,

	/* Entradas digitales */
	EV_SYS_BTN_MODE,		/* boton azul B1 (EXTI): rota el modo           */
	EV_SYS_BTN_CONFIRM,		/* PB0 (polling + antirrebote): confirma digito */
	EV_SYS_DOOR_OPEN,		/* la puerta paso a abierta                     */
	EV_SYS_DOOR_CLOSE,		/* la puerta paso a cerrada                     */

	/* Entradas analogicas */
	EV_SYS_POT_MOVED,		/* el potenciometro se movio (=> hay actividad) */
	EV_SYS_OVERVOLTAGE,		/* la tension de PA7 supero el umbral           */
	EV_SYS_NORMALVOLTAGE,	/* la tension de PA7 volvio a valores normales  */

	/* Notificaciones de otras tareas */
	EV_SYS_STORAGE_DONE		/* la EEPROM termino de escribir                */
} task_system_ev_t;

/* Estados del Task System.
   Los MODOS de operacion que pide la consigna se mapean asi:
     NORMAL  -> ST_SYS_VERIFY (+ ST_SYS_MSG)
     SET_UP  -> ST_SYS_CHANGE y ST_SYS_MENU_*
     REPOSO  -> ST_SYS_SLEEP  (bajo consumo)                                */
typedef enum task_system_st
{
	ST_SYS_VERIFY,			/* NORMAL: ingreso una clave y la comparo       */
	ST_SYS_CHANGE,			/* SET_UP: cargo una clave nueva                */
	ST_SYS_MENU_SELECT,		/* SET_UP: elijo opcion de menu con el pote     */
	ST_SYS_MENU_LOG,		/* SET_UP: recorro el registro de intentos      */
	ST_SYS_MENU_CLOCK,		/* SET_UP: veo el reloj del DS3231              */
	ST_SYS_MSG,				/* mostrando un mensaje temporal                */
	ST_SYS_SLEEP			/* REPOSO: pantalla apagada, bajo consumo       */
} task_system_st_t;

typedef struct
{
	task_system_st_t	state;
	task_system_ev_t	event;
	bool				flag;			/* hay un evento sin procesar        */

	uint32_t			tick_msg;		/* ms que le quedan al mensaje       */
	uint32_t			tick_ui;		/* ms hasta el proximo refresco      */
	uint32_t			tick_idle;		/* ms de inactividad acumulados      */
	uint32_t			tick_relock;	/* ms hasta el re-trabado automatico */

	uint8_t				digit_index;	/* digitos cargados hasta ahora      */
	uint8_t				digit_buffer[PASSWORD_LEN];

	bool				door_open;
	bool				overvoltage;
	bool				disarmed;		/* true tras una clave correcta      */

	/* Ciclo de apertura: despues de desarmar, el sistema NO se re-arma hasta
	   haber visto la puerta ABRIRSE y despues CERRARSE. Sin este latch,
	   cualquier evento de "puerta cerrada" espurio (rebote del reed, ruido,
	   o el reed cableado al reves) vuelve a trabar el cerrojo en la cara del
	   usuario justo despues de acertar la clave. */
	bool				door_seen_open;
} task_system_dta_t;

/********************** external data declaration ****************************/
extern task_system_dta_t task_system_dta;

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_SYSTEM_ATTRIBUTE_H_ */

/********************** end of file ******************************************/
