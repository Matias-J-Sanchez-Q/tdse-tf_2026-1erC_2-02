/*
 * task_actuator.c
 *
 * ACTUAR.
 *
 * Unico dueno de las salidas del sistema (LED, alarma, pin OK y servo del
 * cerrojo). Ninguna otra tarea escribe un GPIO de salida ni toca el PWM:
 * todas le mandan eventos con put_event_task_actuator().
 *
 * Corre cada 1 ms. Codigo NO bloqueante: escrituras de GPIO y de un registro
 * del timer. Los pulsos y parpadeos se resuelven con contadores de ticks,
 * nunca con HAL_Delay(). Costo por tick: ~2 us.
 *
 * FSM (por actuador):
 *
 *   ST_ACT_OFF
 *     EV_ACT_ON    -> salida ON                       -> ST_ACT_ON
 *     EV_ACT_PULSE -> salida ON, tick = tick_max      -> ST_ACT_PULSE
 *     EV_ACT_BLINK -> tick = tick_max                 -> ST_ACT_BLINK
 *
 *   ST_ACT_ON
 *     EV_ACT_OFF   -> salida OFF                      -> ST_ACT_OFF
 *     EV_ACT_PULSE -> tick = tick_max                 -> ST_ACT_PULSE
 *
 *   ST_ACT_PULSE
 *     [tick > 0]   -> tick--                          (interno)
 *     [tick == 0]  -> salida OFF                      -> ST_ACT_OFF
 *     EV_ACT_OFF   -> salida OFF                      -> ST_ACT_OFF (corta el pulso)
 *     EV_ACT_ON    -> salida ON                       -> ST_ACT_ON  (lo sostiene)
 *
 *   ST_ACT_BLINK
 *     [tick > 0]   -> tick--                          (interno)
 *     [tick == 0]  -> togglea, tick = tick_max        (interno)
 *     EV_ACT_OFF   -> salida OFF                      -> ST_ACT_OFF
 *     EV_ACT_ON    -> salida ON                       -> ST_ACT_ON
 */

/********************** inclusions *******************************************/
#include "main.h"
#include "logger.h"
#include "board.h"
#include "app.h"
#include "servo.h"
#include "task_actuator_attribute.h"
#include "task_actuator_interface.h"

/********************** macros and definitions *******************************/
#define DEL_ACT_MIN			(0ul)
#define DEL_PULSE_MS		(2000ul)	/* duracion de los pulsos de OK / alarma */
#define DEL_BLINK_MS		(500ul)		/* semiperiodo del LED de estado         */

#define ACTUATOR_CFG_QTY	(sizeof(task_actuator_cfg_list) / sizeof(task_actuator_cfg_t))
#define ACTUATOR_DTA_QTY	ACTUATOR_CFG_QTY

/********************** internal data declaration ****************************/

/*  ID             Kind       Port              Pin              ON              OFF              tick_max      */
const task_actuator_cfg_t task_actuator_cfg_list[] =
{
	{ID_LED_STATUS, ACT_GPIO,  LED_STATUS_PORT,  LED_STATUS_PIN,  LED_STATUS_ON,  LED_STATUS_OFF,  DEL_BLINK_MS},
	{ID_ALARM,      ACT_GPIO,  ALARM_PORT,       ALARM_PIN,       ALARM_ON,       ALARM_OFF,       DEL_PULSE_MS},
	{ID_OK,         ACT_GPIO,  OK_PORT,          OK_PIN,          OK_ON,          OK_OFF,          DEL_PULSE_MS},
	{ID_LOCK,       ACT_SERVO, NULL,             0u,              GPIO_PIN_SET,   GPIO_PIN_RESET,  DEL_ACT_MIN }
};

task_actuator_dta_t task_actuator_dta_list[ACTUATOR_DTA_QTY];

/********************** internal functions declaration ***********************/
static void task_actuator_statechart(uint32_t index);
static void actuator_write(const task_actuator_cfg_t *p_cfg,
                           task_actuator_dta_t *p_dta,
                           bool on);

/********************** internal data definition *****************************/
const char *p_task_actuator   = "Task Actuator (Output Statechart)";
const char *p_task_actuator_  = "Non-Blocking Code";
const char *p_task_actuator__ = "(Update by Time Code, period = 1mS)";

/********************** external functions definition ************************/
void task_actuator_init(void *parameters)
{
	uint32_t index;
	const task_actuator_cfg_t *p_task_actuator_cfg;
	task_actuator_dta_t *p_task_actuator_dta;

	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu",
	            GET_NAME(task_actuator_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_actuator), p_task_actuator);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_actuator), p_task_actuator_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_actuator), p_task_actuator__);

	for (index = 0; ACTUATOR_DTA_QTY > index; index++)
	{
		p_task_actuator_cfg = &task_actuator_cfg_list[index];
		p_task_actuator_dta = &task_actuator_dta_list[index];

		p_task_actuator_dta->tick  = DEL_ACT_MIN;
		p_task_actuator_dta->state = ST_ACT_OFF;
		p_task_actuator_dta->event = EV_ACT_OFF;
		p_task_actuator_dta->flag  = false;

		/* Todas las salidas arrancan en reposo: alarma apagada, LED apagado,
		   OK en bajo y el cerrojo TRABADO. */
		actuator_write(p_task_actuator_cfg, p_task_actuator_dta, false);
	}
}

void task_actuator_update(void *parameters)
{
	uint32_t index;

	for (index = 0; ACTUATOR_DTA_QTY > index; index++)
	{
		task_actuator_statechart(index);
	}
}

/********************** internal functions definition ************************/

/* Unico punto del programa que escribe una salida fisica */
static void actuator_write(const task_actuator_cfg_t *p_cfg,
                           task_actuator_dta_t *p_dta,
                           bool on)
{
	p_dta->level = on;

	switch (p_cfg->kind)
	{
		case ACT_GPIO:

			HAL_GPIO_WritePin(p_cfg->gpio_port, p_cfg->pin,
			                  on ? p_cfg->act_on : p_cfg->act_off);
			break;

		case ACT_SERVO:

			/* ON = traba liberada, OFF = traba puesta */
			Servo_SetAngle(on ? SERVO_OPEN_DEG : SERVO_CLOSED_DEG);
			break;

		default:
			break;
	}
}

static void task_actuator_statechart(uint32_t index)
{
	const task_actuator_cfg_t *p_cfg = &task_actuator_cfg_list[index];
	task_actuator_dta_t       *p_dta = &task_actuator_dta_list[index];

	switch (p_dta->state)
	{
		case ST_ACT_OFF:

			if (p_dta->flag)
			{
				p_dta->flag = false;

				if (EV_ACT_ON == p_dta->event)
				{
					actuator_write(p_cfg, p_dta, true);
					p_dta->state = ST_ACT_ON;
				}
				else if (EV_ACT_PULSE == p_dta->event)
				{
					actuator_write(p_cfg, p_dta, true);
					p_dta->tick  = p_cfg->tick_max;
					p_dta->state = ST_ACT_PULSE;
				}
				else if (EV_ACT_BLINK == p_dta->event)
				{
					actuator_write(p_cfg, p_dta, true);
					p_dta->tick  = p_cfg->tick_max;
					p_dta->state = ST_ACT_BLINK;
				}
			}
			break;

		case ST_ACT_ON:

			if (p_dta->flag)
			{
				p_dta->flag = false;

				if (EV_ACT_OFF == p_dta->event)
				{
					actuator_write(p_cfg, p_dta, false);
					p_dta->state = ST_ACT_OFF;
				}
				else if (EV_ACT_PULSE == p_dta->event)
				{
					p_dta->tick  = p_cfg->tick_max;
					p_dta->state = ST_ACT_PULSE;
				}
				else if (EV_ACT_BLINK == p_dta->event)
				{
					p_dta->tick  = p_cfg->tick_max;
					p_dta->state = ST_ACT_BLINK;
				}
			}
			break;

		case ST_ACT_PULSE:

			if (p_dta->flag)
			{
				p_dta->flag = false;

				if (EV_ACT_OFF == p_dta->event)
				{
					actuator_write(p_cfg, p_dta, false);
					p_dta->state = ST_ACT_OFF;
					break;
				}
				if (EV_ACT_ON == p_dta->event)
				{
					actuator_write(p_cfg, p_dta, true);
					p_dta->state = ST_ACT_ON;
					break;
				}
				if (EV_ACT_PULSE == p_dta->event)
				{
					/* re-disparo: reinicio la cuenta */
					p_dta->tick = p_cfg->tick_max;
					break;
				}
			}

			if (DEL_ACT_MIN < p_dta->tick)
			{
				p_dta->tick--;
			}
			else
			{
				actuator_write(p_cfg, p_dta, false);
				p_dta->state = ST_ACT_OFF;
			}
			break;

		case ST_ACT_BLINK:

			if (p_dta->flag)
			{
				p_dta->flag = false;

				if (EV_ACT_OFF == p_dta->event)
				{
					actuator_write(p_cfg, p_dta, false);
					p_dta->state = ST_ACT_OFF;
					break;
				}
				if (EV_ACT_ON == p_dta->event)
				{
					actuator_write(p_cfg, p_dta, true);
					p_dta->state = ST_ACT_ON;
					break;
				}
			}

			if (DEL_ACT_MIN < p_dta->tick)
			{
				p_dta->tick--;
			}
			else
			{
				actuator_write(p_cfg, p_dta, !p_dta->level);
				p_dta->tick = p_cfg->tick_max;
			}
			break;

		default:

			p_dta->tick  = DEL_ACT_MIN;
			p_dta->state = ST_ACT_OFF;
			p_dta->event = EV_ACT_OFF;
			p_dta->flag  = false;
			actuator_write(p_cfg, p_dta, false);
			break;
	}
}

/********************** end of file ******************************************/
