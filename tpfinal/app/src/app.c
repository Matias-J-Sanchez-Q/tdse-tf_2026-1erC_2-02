/*
 * app.c
 *
 * EJECUTOR CICLICO (Cyclic Executive) - Bare Metal, Event-Triggered System.
 *
 * El SysTick dispara cada 1 ms y su callback incrementa g_app_tick_cnt.
 * app_update() ve ese contador, y por cada tick pendiente corre UNA vuelta
 * completa de todas las tareas, en orden fijo:
 *
 *      ESCRUTAR            PROCESAR          ACTUAR
 *   +--------------+   +--------------+   +----------------+
 *   | task_sensor  |   | task_system  |   | task_actuator  |
 *   | task_analog  |-->|    (FSM)     |-->| task_display   |
 *   +--------------+   +--------------+   | task_storage   |
 *                                          +----------------+
 *
 * Cuando termina la vuelta, el micro entra en SLEEP (WFI) y se queda ahi
 * hasta la proxima interrupcion (el SysTick, a mas tardar): BAJO CONSUMO.
 * El CPU solo esta despierto el tiempo que realmente trabaja.
 *
 * Cada tarea es NO BLOQUEANTE y se instrumenta con el contador de ciclos del
 * DWT, de donde salen NOE / LET / BCET / WCET por tarea y el factor de uso U
 * del sistema. Todo eso se mira por Live Expressions en el debugger.
 *
 * REQUISITO: la suma de los WCET de todas las tareas tiene que ser < 1 ms,
 * o sea U < 1. Ver g_app_u_pct.
 */

/********************** inclusions *******************************************/
#include "main.h"

#include "logger.h"
#include "dwt.h"

#include "board.h"
#include "app.h"
#include "bus_i2c.h"
#include "task_sensor.h"
#include "task_analog.h"
#include "task_system.h"
#include "task_actuator.h"
#include "task_display.h"
#include "task_storage.h"

/********************** macros and definitions *******************************/
#define G_APP_CNT_INI		(0ul)
#define G_APP_TICK_CNT_INI	(0ul)

#define TASK_X_NOE_INI		(0ul)
#define TASK_X_LET_INI		(0ul)
#define TASK_X_BCET_INI		(0xFFFFFFFFul)
#define TASK_X_WCET_INI		(0ul)

/* Periodo del ejecutor ciclico, en us. Es el deadline de la vuelta entera. */
#define APP_PERIOD_US		(1000ul)

typedef struct
{
	void (*task_init)(void *);
	void (*task_update)(void *);
	void  *parameters;
	const char *name;
} task_cfg_t;

typedef struct
{
	uint32_t NOE;		/* cantidad de ejecuciones                    */
	uint32_t LET;		/* Last  Execution Time  (us)                 */
	uint32_t BCET;		/* Best  Case Execution Time (us)             */
	uint32_t WCET;		/* Worst Case Execution Time (us) <- el clave */
} task_dta_t;

/********************** internal data declaration ****************************/

/* ORDEN IMPORTANTE: escrutar -> procesar -> actuar.
   Asi un evento generado por un sensor en el tick N lo procesa el sistema y
   lo ejecutan los actuadores en el MISMO tick N: latencia de 1 ms de punta
   a punta, sin depender del orden de llegada. */
const task_cfg_t task_cfg_list[] =
{
	{task_sensor_init,		task_sensor_update,		NULL, "task_sensor"  },	/* escrutar: GPIO  */
	{task_analog_init,		task_analog_update,		NULL, "task_analog"  },	/* escrutar: ADC   */
	{task_system_init,		task_system_update,		NULL, "task_system"  },	/* procesar: FSM   */
	{task_actuator_init,	task_actuator_update,	NULL, "task_actuator"},	/* actuar: salidas */
	{task_display_init,		task_display_update,	NULL, "task_display" },	/* actuar: LCD     */
	{task_storage_init,		task_storage_update,	NULL, "task_storage" }	/* actuar: EEPROM  */
};

#define TASK_QTY	(sizeof(task_cfg_list) / sizeof(task_cfg_t))

/********************** internal data definition *****************************/
const char *p_app	= "Bare Metal - Event-Triggered Systems (ETS)";
const char *p_app_	= "App - Cerradura Electronica (Cyclic Executive)";
const char *p_app__	= "(Update by Time Code, period = 1mS)";

/********************** external data declaration ****************************/
uint32_t g_app_cnt;

volatile uint32_t g_app_tick_cnt;

/* --- Metricas del sistema (mirar por Live Expressions) --------------------
 *   g_app_runtime_us  : lo que tardo la ULTIMA vuelta completa (us)
 *   g_app_wcet_us     : la PEOR vuelta desde el arranque (us)  <- el numero
 *   g_app_u_x1000     : U = WCET / periodo, x1000 (1000 = 100% de CPU)
 *   g_app_u_pct       : U en porcentaje
 *   g_app_overrun_cnt : veces que una vuelta se paso de 1 ms (deberia ser 0)
 * ------------------------------------------------------------------------ */
volatile uint32_t g_app_runtime_us;
volatile uint32_t g_app_wcet_us;
volatile uint32_t g_app_u_x1000;
volatile uint32_t g_app_u_pct;
volatile uint32_t g_app_overrun_cnt;

task_dta_t task_dta_list[TASK_QTY];

/********************** external functions definition ************************/
void app_init(void)
{
	uint32_t index;

	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %lu", GET_NAME(app_init), HAL_GetTick());

	LOGGER_INFO(" %s is a %s", GET_NAME(app), p_app);
	LOGGER_INFO(" %s is a %s", GET_NAME(app), p_app_);
	LOGGER_INFO(" %s is a %s", GET_NAME(app), p_app__);

	g_app_cnt         = G_APP_CNT_INI;
	g_app_runtime_us  = 0;
	g_app_wcet_us     = 0;
	g_app_u_x1000     = 0;
	g_app_u_pct       = 0;
	g_app_overrun_cnt = 0;

	/* Contador de ciclos del DWT: de aca salen todos los tiempos */
	cycle_counter_init();

	for (index = 0; TASK_QTY > index; index++)
	{
		(*task_cfg_list[index].task_init)(task_cfg_list[index].parameters);

		task_dta_list[index].NOE  = TASK_X_NOE_INI;
		task_dta_list[index].LET  = TASK_X_LET_INI;
		task_dta_list[index].BCET = TASK_X_BCET_INI;
		task_dta_list[index].WCET = TASK_X_WCET_INI;
	}

	/* Seccion critica: el tick lo toca la ISR del SysTick */
	__asm("CPSID i");
	g_app_tick_cnt = G_APP_TICK_CNT_INI;
	__asm("CPSIE i");
}

void app_update(void)
{
	uint32_t index;
	bool     b_time_update_required = false;

	__asm("CPSID i");
	if (G_APP_TICK_CNT_INI < g_app_tick_cnt)
	{
		g_app_tick_cnt--;
		b_time_update_required = true;
	}
	__asm("CPSIE i");

	while (b_time_update_required)
	{
		g_app_cnt++;
		g_app_runtime_us = 0;

		/* Libero el token del bus I2C: en esta vuelta una sola tarea va a
		   poder usarlo. Eso acota el peor caso de la vuelta a UNA transaccion
		   I2C, que es lo que hace predecible el WCET. */
		bus_i2c_new_cycle();

		/* --- Una vuelta del ejecutor ciclico ---------------------------- */
		for (index = 0; TASK_QTY > index; index++)
		{
			cycle_counter_reset();

			(*task_cfg_list[index].task_update)(task_cfg_list[index].parameters);

			task_dta_list[index].NOE++;
			task_dta_list[index].LET = cycle_counter_get_time_us();

			if (task_dta_list[index].BCET > task_dta_list[index].LET)
			{
				task_dta_list[index].BCET = task_dta_list[index].LET;
			}

			if (task_dta_list[index].WCET < task_dta_list[index].LET)
			{
				task_dta_list[index].WCET = task_dta_list[index].LET;
			}

			g_app_runtime_us += task_dta_list[index].LET;
		}

		/* --- Metricas de la vuelta completa ----------------------------- */
		if (g_app_wcet_us < g_app_runtime_us)
		{
			g_app_wcet_us = g_app_runtime_us;

			/* Factor de uso U = WCET / periodo */
			g_app_u_x1000 = (g_app_wcet_us * 1000ul) / APP_PERIOD_US;
			g_app_u_pct   = (g_app_wcet_us * 100ul)  / APP_PERIOD_US;
		}

		if (g_app_runtime_us > APP_PERIOD_US)
		{
			/* La vuelta no entro en su periodo: se perdio un deadline.
			   Si esto sube, hay que aligerar alguna tarea. */
			g_app_overrun_cnt++;
		}

		/* --- BAJO CONSUMO ----------------------------------------------- */
		/* Termino el trabajo del tick: duermo el CPU hasta la proxima
		   interrupcion (el SysTick de 1 ms, a mas tardar). El micro deja de
		   ejecutar instrucciones: es la diferencia entre un super-loop que
		   gira al pedo al 100% y uno que solo consume lo que trabaja. */
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

		__asm("CPSID i");
		if (G_APP_TICK_CNT_INI < g_app_tick_cnt)
		{
			g_app_tick_cnt--;
			b_time_update_required = true;
		}
		else
		{
			b_time_update_required = false;
		}
		__asm("CPSIE i");
	}
}

/* Callback del SysTick: el latido de 1 ms de todo el sistema */
void HAL_SYSTICK_Callback(void)
{
	g_app_tick_cnt++;
}

/********************** end of file ******************************************/
