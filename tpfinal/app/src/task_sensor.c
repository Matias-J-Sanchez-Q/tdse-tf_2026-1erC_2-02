/*
 * task_sensor.c
 *
 * ESCRUTAR (entradas digitales).
 *
 * Corre cada 1 ms desde el ejecutor ciclico. Lee los GPIO de entrada,
 * les aplica una FSM de antirrebote de 4 estados y, cuando una transicion
 * queda confirmada, publica el evento correspondiente en la cola del
 * Task System. No toca ninguna salida ni ninguna logica de negocio.
 *
 * Codigo NO bloqueante: solo lecturas de GPIO. Costo por tick: ~1 us.
 *
 * FSM de antirrebote (por sensor):
 *
 *   ST_BTN_UP
 *     EV_BTN_DOWN            -> tick = tick_max        -> ST_BTN_FALLING
 *
 *   ST_BTN_FALLING
 *     [tick > 0]             -> tick--                 (interno)
 *     EV_BTN_UP  [tick == 0] -> ST_BTN_UP              (rebote, ignorado)
 *     EV_BTN_DOWN[tick == 0] -> raise signal_down      -> ST_BTN_DOWN
 *
 *   ST_BTN_DOWN
 *     EV_BTN_UP              -> tick = tick_max        -> ST_BTN_RISING
 *
 *   ST_BTN_RISING
 *     [tick > 0]             -> tick--                 (interno)
 *     EV_BTN_DOWN[tick == 0] -> ST_BTN_DOWN            (rebote, ignorado)
 *     EV_BTN_UP  [tick == 0] -> raise signal_up        -> ST_BTN_UP
 */

/********************** inclusions *******************************************/
#include "main.h"
#include "logger.h"
#include "board.h"
#include "app.h"
#include "task_sensor_attribute.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"

/********************** macros and definitions *******************************/
#define DEL_BTN_MIN		(0ul)
#define DEL_BTN_MAX		(50ul)		/* 50 ms de antirrebote para el pulsador */
#define DEL_DOOR_MAX	(30ul)		/* 30 ms para el reed switch            */

#define SENSOR_CFG_QTY	(sizeof(task_sensor_cfg_list) / sizeof(task_sensor_cfg_t))
#define SENSOR_DTA_QTY	SENSOR_CFG_QTY

/********************** internal data declaration ****************************/

/*  ID              Port                Pin                 Activo               tick_max      signal_down          signal_up            */
const task_sensor_cfg_t task_sensor_cfg_list[] =
{
	{ID_BTN_CONFIRM, BTN_CONFIRM_PORT,   BTN_CONFIRM_PIN,    BTN_CONFIRM_PRESSED, DEL_BTN_MAX,  EV_SYS_BTN_CONFIRM,  EV_SYS_NONE       },
	{ID_DOOR,        DOOR_PORT,          DOOR_PIN,           DOOR_OPEN_LEVEL,     DEL_DOOR_MAX, EV_SYS_DOOR_OPEN,    EV_SYS_DOOR_CLOSE }
};

task_sensor_dta_t task_sensor_dta_list[SENSOR_DTA_QTY];

/********************** internal functions declaration ***********************/
static void task_sensor_statechart(uint32_t index);

/********************** internal data definition *****************************/
const char *p_task_sensor   = "Task Sensor (Debounce Statechart)";
const char *p_task_sensor_  = "Non-Blocking Code";
const char *p_task_sensor__ = "(Update by Time Code, period = 1mS)";

/********************** external functions definition ************************/
void task_sensor_init(void *parameters)
{
	uint32_t index;
	task_sensor_dta_t *p_task_sensor_dta;

	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu",
	            GET_NAME(task_sensor_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_sensor), p_task_sensor);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_sensor), p_task_sensor_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_sensor), p_task_sensor__);

	for (index = 0; SENSOR_DTA_QTY > index; index++)
	{
		p_task_sensor_dta = &task_sensor_dta_list[index];

		p_task_sensor_dta->state = ST_BTN_UP;
		p_task_sensor_dta->event = EV_BTN_UP;
		p_task_sensor_dta->tick  = DEL_BTN_MIN;
	}
}

void task_sensor_update(void *parameters)
{
	uint32_t index;

	for (index = 0; SENSOR_DTA_QTY > index; index++)
	{
		task_sensor_statechart(index);
	}
}

/********************** internal functions definition ************************/

/* Publica un evento en el Task System, salvo que sea EV_SYS_NONE
   (los sensores que no necesitan avisar el flanco de subida usan NONE). */
static void raise_if_any(task_system_ev_t signal)
{
	if (EV_SYS_NONE != signal)
	{
		put_event_task_system(signal);
	}
}

static void task_sensor_statechart(uint32_t index)
{
	const task_sensor_cfg_t *p_task_sensor_cfg = &task_sensor_cfg_list[index];
	task_sensor_dta_t       *p_task_sensor_dta = &task_sensor_dta_list[index];

	/* ESCRUTAR: lectura cruda del GPIO */
	if (p_task_sensor_cfg->active_level ==
	    HAL_GPIO_ReadPin(p_task_sensor_cfg->gpio_port, p_task_sensor_cfg->pin))
	{
		p_task_sensor_dta->event = EV_BTN_DOWN;
	}
	else
	{
		p_task_sensor_dta->event = EV_BTN_UP;
	}

	/* PROCESAR: FSM de antirrebote */
	switch (p_task_sensor_dta->state)
	{
		case ST_BTN_UP:

			if (EV_BTN_DOWN == p_task_sensor_dta->event)
			{
				p_task_sensor_dta->tick  = p_task_sensor_cfg->tick_max;
				p_task_sensor_dta->state = ST_BTN_FALLING;
			}
			break;

		case ST_BTN_FALLING:

			if (DEL_BTN_MIN < p_task_sensor_dta->tick)
			{
				p_task_sensor_dta->tick--;
			}
			else if (EV_BTN_UP == p_task_sensor_dta->event)
			{
				/* rebote: vuelvo sin emitir evento */
				p_task_sensor_dta->state = ST_BTN_UP;
			}
			else
			{
				raise_if_any(p_task_sensor_cfg->signal_down);
				p_task_sensor_dta->state = ST_BTN_DOWN;
			}
			break;

		case ST_BTN_DOWN:

			if (EV_BTN_UP == p_task_sensor_dta->event)
			{
				p_task_sensor_dta->tick  = p_task_sensor_cfg->tick_max;
				p_task_sensor_dta->state = ST_BTN_RISING;
			}
			break;

		case ST_BTN_RISING:

			if (DEL_BTN_MIN < p_task_sensor_dta->tick)
			{
				p_task_sensor_dta->tick--;
			}
			else if (EV_BTN_DOWN == p_task_sensor_dta->event)
			{
				/* rebote: vuelvo sin emitir evento */
				p_task_sensor_dta->state = ST_BTN_DOWN;
			}
			else
			{
				raise_if_any(p_task_sensor_cfg->signal_up);
				p_task_sensor_dta->state = ST_BTN_UP;
			}
			break;

		default:

			p_task_sensor_dta->tick  = DEL_BTN_MIN;
			p_task_sensor_dta->state = ST_BTN_UP;
			p_task_sensor_dta->event = EV_BTN_UP;
			break;
	}
}

/********************** end of file ******************************************/
