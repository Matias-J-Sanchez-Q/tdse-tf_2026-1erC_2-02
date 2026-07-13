/*
 * task_analog.c
 *
 * ESCRUTAR (entradas analogicas).
 *
 * Multiplexa dos canales de ADC, uno por tick, de forma NO BLOQUEANTE:
 *
 *   tick N   : arranca la conversion del canal actual  (~2 us de CPU)
 *   tick N+1 : si el flag EOC esta en 1, lee el resultado y cambia de canal
 *
 * O sea: nunca se espera a que el ADC termine dentro del mismo tick. Cada
 * canal queda muestreado cada 4 ms, de sobra para un potenciometro y un
 * monitor de tension.
 *
 * Canales:
 *   ADC1_IN0 (PA0) -> potenciometro: selecciona digito 0..9 y opcion de menu
 *   ADC2_IN7 (PA7) -> monitor de tension 0..3,3V: dispara alarma por sabotaje
 *
 * Publica en el Task System:
 *   EV_SYS_POT_MOVED      (el pote se movio => cuenta como actividad)
 *   EV_SYS_OVERVOLTAGE    (paso el umbral)
 *   EV_SYS_NORMALVOLTAGE  (volvio a la normalidad)
 */

/********************** inclusions *******************************************/
#include "main.h"
#include "logger.h"
#include "board.h"
#include "app.h"
#include "task_analog_attribute.h"
#include "task_analog_interface.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"

/********************** macros and definitions *******************************/
#define ADC_FULL_SCALE			(4095u)
#define ADC_MV_FULL_SCALE		(3300u)

/* Cuanto se tiene que mover el pote para que cuente como actividad del usuario */
#define POT_MOVE_THRESHOLD		(120)

/* Histeresis del comparador de sobretension, para que no oscile en el umbral */
#define VMON_ALARM_MV_ON		(VMON_ALARM_MV)
#define VMON_ALARM_MV_OFF		(VMON_ALARM_MV - 100u)

/* Filtro exponencial simple del pote: y = y + (x - y)/4 */
#define POT_FILTER_SHIFT		(2u)

/********************** external data declaration ****************************/
extern ADC_HandleTypeDef hadc1;		/* potenciometro     - definido en main.c */
extern ADC_HandleTypeDef hadc2;		/* monitor de tension- definido en main.c */

/********************** internal data definition *****************************/
task_analog_dta_t task_analog_dta;

static bool b_conversion_pending;	/* hay una conversion en vuelo */

const char *p_task_analog   = "Task Analog (ADC multiplexado)";
const char *p_task_analog_  = "Non-Blocking Code";
const char *p_task_analog__ = "(Update by Time Code, period = 1mS)";

/********************** internal functions declaration ***********************/
static ADC_HandleTypeDef *current_adc(void);
static void process_pot(uint16_t raw);
static void process_vmon(uint16_t raw);

/********************** external functions definition ************************/
void task_analog_init(void *parameters)
{
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu",
	            GET_NAME(task_analog_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_analog), p_task_analog);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_analog), p_task_analog_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_analog), p_task_analog__);

	task_analog_dta.state       = ST_ANA_POT;
	task_analog_dta.pot_raw     = 0;
	task_analog_dta.pot_ref     = 0;
	task_analog_dta.pot_digit   = 0;
	task_analog_dta.vmon_raw    = 0;
	task_analog_dta.vmon_mv     = 0;
	task_analog_dta.overvoltage = false;

	b_conversion_pending = false;
}

void task_analog_update(void *parameters)
{
	ADC_HandleTypeDef *p_adc = current_adc();
	uint16_t raw;

	if (!b_conversion_pending)
	{
		/* Arranco la conversion y me voy: NO espero el resultado */
		HAL_ADC_Start(p_adc);
		b_conversion_pending = true;
		return;
	}

	/* Hay una conversion en vuelo: si todavia no termino, salgo y reintento
	   en el proximo tick. Tampoco espero aca. */
	if (RESET == __HAL_ADC_GET_FLAG(p_adc, ADC_FLAG_EOC))
	{
		return;
	}

	raw = (uint16_t)HAL_ADC_GetValue(p_adc);	/* leer el DR limpia el EOC */
	HAL_ADC_Stop(p_adc);
	b_conversion_pending = false;

	/* PROCESAR + cambio de canal para el proximo tick */
	if (ST_ANA_POT == task_analog_dta.state)
	{
		process_pot(raw);
		task_analog_dta.state = ST_ANA_VMON;
	}
	else
	{
		process_vmon(raw);
		task_analog_dta.state = ST_ANA_POT;
	}
}

/********************** interface (lectura para otras tareas) ****************/
uint8_t analog_get_digit(void)
{
	return task_analog_dta.pot_digit;
}

uint16_t analog_get_raw(void)
{
	return task_analog_dta.pot_raw;
}

uint8_t analog_map_to_option(uint8_t options)
{
	uint32_t sel;

	if (0u == options)
	{
		return 0u;
	}

	sel = ((uint32_t)task_analog_dta.pot_raw * options) / 4096u;

	if (sel >= options)
	{
		sel = (uint32_t)options - 1u;
	}

	return (uint8_t)sel;
}

uint16_t analog_get_mv(void)
{
	return task_analog_dta.vmon_mv;
}

/********************** internal functions definition ************************/
static ADC_HandleTypeDef *current_adc(void)
{
	return (ST_ANA_POT == task_analog_dta.state) ? &hadc1 : &hadc2;
}

static void process_pot(uint16_t raw)
{
	int32_t delta;

	/* Filtro exponencial: saca el ruido del pote sin agregar retardo notable */
	task_analog_dta.pot_raw = (uint16_t)(task_analog_dta.pot_raw +
	                          (((int32_t)raw - (int32_t)task_analog_dta.pot_raw) >> POT_FILTER_SHIFT));

	/* Mapeo a digito 0..9 */
	task_analog_dta.pot_digit = (uint8_t)(((uint32_t)task_analog_dta.pot_raw * 10u) / 4096u);
	if (task_analog_dta.pot_digit > 9u)
	{
		task_analog_dta.pot_digit = 9u;
	}

	/* Deteccion de movimiento (=> el usuario esta ahi => no dormir) */
	delta = (int32_t)task_analog_dta.pot_raw - (int32_t)task_analog_dta.pot_ref;
	if (delta < 0)
	{
		delta = -delta;
	}

	if (delta > POT_MOVE_THRESHOLD)
	{
		task_analog_dta.pot_ref = task_analog_dta.pot_raw;
		put_event_task_system(EV_SYS_POT_MOVED);
	}
}

static void process_vmon(uint16_t raw)
{
	task_analog_dta.vmon_raw = raw;
	task_analog_dta.vmon_mv  = (uint16_t)(((uint32_t)raw * ADC_MV_FULL_SCALE) / ADC_FULL_SCALE);

	/* Comparador con histeresis: solo aviso en los cambios de estado */
	if (!task_analog_dta.overvoltage)
	{
		if (task_analog_dta.vmon_mv > VMON_ALARM_MV_ON)
		{
			task_analog_dta.overvoltage = true;
			put_event_task_system(EV_SYS_OVERVOLTAGE);
		}
	}
	else
	{
		if (task_analog_dta.vmon_mv < VMON_ALARM_MV_OFF)
		{
			task_analog_dta.overvoltage = false;
			put_event_task_system(EV_SYS_NORMALVOLTAGE);
		}
	}
}

/********************** end of file ******************************************/
