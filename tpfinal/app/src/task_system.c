/*
 * task_system.c
 *
 * PROCESAR. El cerebro de la cerradura.
 *
 * Es la unica tarea que tiene logica de negocio, y NO toca hardware: no lee
 * un GPIO, no escribe un GPIO, no habla I2C. Consume eventos de su cola y
 * pide cosas a las demas tareas por sus interfaces:
 *
 *      task_sensor   --\
 *      task_analog   ---+--> [cola de eventos] --> TASK SYSTEM (FSM)
 *      EXTI (boton B1)-/                                |
 *                                                       +--> task_actuator (LED, alarma, OK, servo)
 *                                                       +--> task_display   (LCD)
 *                                                       +--> task_storage   (EEPROM)
 *
 * Modos de operacion (requisito de la consigna):
 *
 *      NORMAL : ST_SYS_VERIFY  (+ ST_SYS_MSG para los mensajes temporales)
 *      SET_UP : ST_SYS_CHANGE, ST_SYS_MENU_SELECT, ST_SYS_MENU_LOG,
 *               ST_SYS_MENU_CLOCK
 *      REPOSO : ST_SYS_SLEEP   (bajo consumo: pantalla apagada + WFI)
 *
 * El boton azul B1 rota:  VERIFY -> CHANGE -> MENU -> VERIFY -> ...
 *
 * Codigo NO bloqueante. Todos los tiempos (mensajes, refresco de UI,
 * inactividad) se cuentan con contadores de ticks de 1 ms.
 */

/********************** inclusions *******************************************/
#include "main.h"
#include <string.h>
#include <stdio.h>

#include "logger.h"
#include "board.h"
#include "app.h"
#include "bus_i2c.h"
#include "ds3231.h"

#include "task_system_attribute.h"
#include "task_system_interface.h"
#include "task_analog_interface.h"
#include "task_actuator_attribute.h"
#include "task_actuator_interface.h"
#include "task_display_attribute.h"
#include "task_display_interface.h"
#include "task_storage_attribute.h"
#include "task_storage_interface.h"

/********************** macros and definitions *******************************/
#define DEL_SYS_MIN			(0ul)
#define DEL_UI_REFRESH_MS	(150ul)		/* cada cuanto se redibuja la UI     */
#define DEL_MSG_SHOW_MS		(2000ul)	/* cuanto dura un mensaje temporal   */
#define DEL_IDLE_TIMEOUT_MS	(15000ul)	/* inactividad antes de irse a dormir */

/* Si la clave fue correcta pero nadie llego a abrir la puerta, el cerrojo se
   vuelve a trabar solo despues de este tiempo. Sin esto, una clave acertada
   dejaria la traba abierta para siempre. */
#define DEL_RELOCK_MS		(10000ul)

/* Opciones del menu */
#define MENU_OPT_QTY		(2u)
#define MENU_OPT_LOG		(0u)
#define MENU_OPT_CLOCK		(1u)

/********************** internal data declaration ****************************/
task_system_dta_t task_system_dta;

static const char *const menu_opts[MENU_OPT_QTY] =
{
	"Ver registro",
	"Ver reloj"
};

/* Estado anterior de las salidas, para no re-emitir el mismo evento cada ms */
static bool b_alarm_prev;
static bool b_disarmed_prev;

/* --- Cache del RTC -------------------------------------------------------
   Leer el DS3231 es una transaccion I2C de ~250 us. Hacerla en el camino de
   "clave correcta" o en cada refresco de pantalla metia ese costo justo en
   el peor caso de la vuelta.

   En vez de eso, el reloj se lee UNA vez por segundo (que es toda la
   resolucion que tiene un reloj de todas formas) y se guarda aca. Tanto el
   menu como el registro de intentos usan esta copia: costo cero.

   El precio: el timestamp de un intento puede estar hasta 1 s desfasado.
   Para un log de una cerradura, es irrelevante. */
#define DEL_RTC_REFRESH_MS	(1000ul)

static DS3231_Time rtc_cache;
static uint32_t    tick_rtc;

/********************** internal functions declaration ***********************/
static void task_system_statechart(void);
static void handle_global_events(task_system_ev_t event);
static void update_outputs(void);
static void enter_state(task_system_st_t state);
static void show_message(const char *line0, const char *line1);
static void reset_digits(void);
static void push_digit(uint8_t digit);
static void confirm_verify(void);
static void confirm_change(void);
static void draw_ui(void);
static bool is_activity_event(task_system_ev_t event);
static void refresh_rtc_cache(void);

/********************** internal data definition *****************************/
const char *p_task_system   = "Task System (Lock Statechart)";
const char *p_task_system_  = "Non-Blocking Code";
const char *p_task_system__ = "(Update by Time Code, period = 1mS)";

/********************** external functions definition ************************/
void task_system_init(void *parameters)
{
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu",
	            GET_NAME(task_system_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_system), p_task_system);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_system), p_task_system_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_system), p_task_system__);

	init_event_task_system();

	task_system_dta.state       = ST_SYS_VERIFY;
	task_system_dta.event       = EV_SYS_NONE;
	task_system_dta.flag        = false;

	task_system_dta.tick_msg    = DEL_SYS_MIN;
	task_system_dta.tick_ui     = DEL_SYS_MIN;	/* dibuja en el primer tick */
	task_system_dta.tick_idle   = DEL_SYS_MIN;
	task_system_dta.tick_relock = DEL_SYS_MIN;

	task_system_dta.door_open      = false;
	task_system_dta.overvoltage    = false;
	task_system_dta.disarmed       = false;
	task_system_dta.door_seen_open = false;

	reset_digits();

	b_alarm_prev    = false;
	b_disarmed_prev = false;

	memset(&rtc_cache, 0, sizeof(rtc_cache));
	tick_rtc = DEL_SYS_MIN;		/* que lea el reloj en el primer tick libre */

	display_clear();
	display_printf(0, "Ingresar clave");
}

void task_system_update(void *parameters)
{
	/* Un evento por tick: la cola drena a 1000 ev/s, de sobra */
	if (any_event_task_system())
	{
		task_system_dta.event = get_event_task_system();
		task_system_dta.flag  = true;
	}
	else
	{
		task_system_dta.event = EV_SYS_NONE;
		task_system_dta.flag  = false;
	}

	/* Mantengo el reloj al dia (a lo sumo una lectura I2C por segundo) */
	refresh_rtc_cache();

	/* La SEGURIDAD corre siempre, en cualquier estado, incluso dormido:
	   puerta, sobretension, alarma y cerrojo no dependen de la UI. */
	if (task_system_dta.flag)
	{
		handle_global_events(task_system_dta.event);
	}

	task_system_statechart();

	update_outputs();
}

/********************** internal functions definition ************************/

/* Eventos que valen en cualquier estado (seguridad + inactividad) */
static void handle_global_events(task_system_ev_t event)
{
	/* Cualquier interaccion del usuario resetea el contador de inactividad
	   y despierta el sistema si estaba durmiendo. */
	if (is_activity_event(event))
	{
		task_system_dta.tick_idle = DEL_SYS_MIN;

		if (ST_SYS_SLEEP == task_system_dta.state)
		{
			display_wake();
			enter_state(ST_SYS_VERIFY);
		}
	}

	switch (event)
	{
		case EV_SYS_DOOR_OPEN:

			task_system_dta.door_open = true;

			/* El usuario efectivamente abrio: recien ahora tiene sentido
			   re-armar cuando la vuelva a cerrar. Y cancelo el re-trabado
			   automatico, porque ya paso por la puerta. */
			if (task_system_dta.disarmed)
			{
				task_system_dta.door_seen_open = true;
				task_system_dta.tick_relock    = DEL_SYS_MIN;
			}
			break;

		case EV_SYS_DOOR_CLOSE:

			task_system_dta.door_open = false;

			/* RE-ARMADO: solo si la puerta realmente se abrio antes.
			   Un EV_SYS_DOOR_CLOSE suelto (sin apertura previa) NO puede
			   volver a trabar el cerrojo: eso era lo que hacia que el servo
			   se cerrara justo despues de una clave correcta. */
			if (task_system_dta.disarmed && task_system_dta.door_seen_open)
			{
				task_system_dta.disarmed       = false;
				task_system_dta.door_seen_open = false;
				task_system_dta.tick_relock    = DEL_SYS_MIN;
			}
			break;

		case EV_SYS_OVERVOLTAGE:

			task_system_dta.overvoltage = true;
			break;

		case EV_SYS_NORMALVOLTAGE:

			task_system_dta.overvoltage = false;
			break;

		default:
			break;
	}
}

/* Recalcula las salidas a partir del estado y solo emite eventos cuando algo
   cambio de verdad (asi no se inunda al Task Actuator con un evento por ms). */
static void update_outputs(void)
{
	bool b_alarm;

	/* La alarma suena si el sistema esta armado y la puerta esta abierta,
	   o si detecto sobretension (sabotaje). Los pulsos de "clave incorrecta"
	   los maneja el propio Task Actuator con EV_ACT_PULSE. */
	b_alarm = (!task_system_dta.disarmed) &&
	          (task_system_dta.door_open || task_system_dta.overvoltage);

	if (b_alarm != b_alarm_prev)
	{
		b_alarm_prev = b_alarm;
		put_event_task_actuator(b_alarm ? EV_ACT_ON : EV_ACT_OFF, ID_ALARM);

		/* El LED de estado parpadea mientras hay alarma */
		if (b_alarm)
		{
			put_event_task_actuator(EV_ACT_BLINK, ID_LED_STATUS);
		}
		else
		{
			put_event_task_actuator(task_system_dta.disarmed ? EV_ACT_ON : EV_ACT_OFF,
			                        ID_LED_STATUS);
		}
	}

	/* El cerrojo sigue al estado de armado: desarmado = abierto */
	if (task_system_dta.disarmed != b_disarmed_prev)
	{
		b_disarmed_prev = task_system_dta.disarmed;

		put_event_task_actuator(task_system_dta.disarmed ? EV_ACT_ON : EV_ACT_OFF,
		                        ID_LOCK);

		if (!b_alarm)
		{
			put_event_task_actuator(task_system_dta.disarmed ? EV_ACT_ON : EV_ACT_OFF,
			                        ID_LED_STATUS);
		}
	}
}

static void task_system_statechart(void)
{
	task_system_ev_t event = task_system_dta.flag ? task_system_dta.event : EV_SYS_NONE;

	/* --- Re-trabado automatico (corre en cualquier estado, hasta dormido) ---
	   Si acerte la clave pero nunca abri la puerta, a los DEL_RELOCK_MS el
	   cerrojo se vuelve a trabar solo. Si la puerta SI se abrio, el contador
	   ya quedo cancelado y manda el ciclo abrir -> cerrar. */
	if (DEL_SYS_MIN < task_system_dta.tick_relock)
	{
		task_system_dta.tick_relock--;

		if (DEL_SYS_MIN == task_system_dta.tick_relock)
		{
			task_system_dta.disarmed       = false;
			task_system_dta.door_seen_open = false;
		}
	}

	/* --- Contador de inactividad (comun a todos los estados despiertos) --- */
	if (ST_SYS_SLEEP != task_system_dta.state)
	{
		if (task_system_dta.tick_idle < DEL_IDLE_TIMEOUT_MS)
		{
			task_system_dta.tick_idle++;
		}
		else if (ST_SYS_MSG != task_system_dta.state)
		{
			/* No me duermo con un mensaje en pantalla */
			display_sleep();
			enter_state(ST_SYS_SLEEP);
			return;
		}
	}

	/* --- El boton azul rota el modo, valga el estado que valga ----------- */
	if ((EV_SYS_BTN_MODE == event) && (ST_SYS_MSG != task_system_dta.state))
	{
		switch (task_system_dta.state)
		{
			case ST_SYS_VERIFY:	enter_state(ST_SYS_CHANGE);      break;
			case ST_SYS_CHANGE:	enter_state(ST_SYS_MENU_SELECT); break;
			default:			enter_state(ST_SYS_VERIFY);      break;
		}
		return;
	}

	/* --- FSM ------------------------------------------------------------- */
	switch (task_system_dta.state)
	{
		case ST_SYS_VERIFY:

			if (EV_SYS_BTN_CONFIRM == event)
			{
				push_digit(analog_get_digit());

				if (PASSWORD_LEN <= task_system_dta.digit_index)
				{
					confirm_verify();
					return;
				}
			}
			break;

		case ST_SYS_CHANGE:

			if (EV_SYS_BTN_CONFIRM == event)
			{
				push_digit(analog_get_digit());

				if (PASSWORD_LEN <= task_system_dta.digit_index)
				{
					confirm_change();
					return;
				}
			}
			break;

		case ST_SYS_MENU_SELECT:

			if (EV_SYS_BTN_CONFIRM == event)
			{
				if (MENU_OPT_LOG == analog_map_to_option(MENU_OPT_QTY))
				{
					enter_state(ST_SYS_MENU_LOG);
				}
				else
				{
					enter_state(ST_SYS_MENU_CLOCK);
				}
				return;
			}
			break;

		case ST_SYS_MENU_LOG:
		case ST_SYS_MENU_CLOCK:

			if (EV_SYS_BTN_CONFIRM == event)
			{
				enter_state(ST_SYS_MENU_SELECT);
				return;
			}
			break;

		case ST_SYS_MSG:

			if (DEL_SYS_MIN < task_system_dta.tick_msg)
			{
				task_system_dta.tick_msg--;
			}
			else
			{
				enter_state(ST_SYS_VERIFY);
				return;
			}
			break;

		case ST_SYS_SLEEP:

			/* Solo se sale por un evento de actividad, que ya lo trato
			   handle_global_events(). Aca no se hace nada: pantalla apagada
			   y el ejecutor ciclico entra en WFI => bajo consumo. */
			break;

		default:

			enter_state(ST_SYS_VERIFY);
			return;
	}

	/* --- Refresco de la UI ---------------------------------------------- */
	if (DEL_SYS_MIN < task_system_dta.tick_ui)
	{
		task_system_dta.tick_ui--;
	}
	else
	{
		task_system_dta.tick_ui = DEL_UI_REFRESH_MS;
		draw_ui();
	}
}

/* Dibuja la pantalla del estado actual. Solo escribe en el framebuffer:
   NO habla I2C (de eso se encarga el Task Display). */
static void draw_ui(void)
{
	switch (task_system_dta.state)
	{
		case ST_SYS_VERIFY:

			display_printf(0, "Ingresar clave");
			display_printf(1, "Dig %u/%u: %u",
			               task_system_dta.digit_index + 1u, PASSWORD_LEN,
			               analog_get_digit());
			break;

		case ST_SYS_CHANGE:

			display_printf(0, "Cambiar clave");
			display_printf(1, "Dig %u/%u: %u",
			               task_system_dta.digit_index + 1u, PASSWORD_LEN,
			               analog_get_digit());
			break;

		case ST_SYS_MENU_SELECT:

			display_printf(0, "Menu (PB0 = OK)");
			display_printf(1, ">%s", menu_opts[analog_map_to_option(MENU_OPT_QTY)]);
			break;

		case ST_SYS_MENU_LOG:
		{
			uint8_t           count = storage_attempt_count();
			uint8_t           index;
			storage_attempt_t att;

			if (0u == count)
			{
				display_printf(0, "Sin intentos");
				display_printf(1, "PB0 = volver");
				break;
			}

			index = analog_map_to_option(count);

			if (storage_get_attempt(index, &att))
			{
				display_printf(0, "%02u/%02u %02u:%02u:%02u",
				               att.month, att.date,
				               att.hour, att.minute, att.second);
				display_printf(1, "%u/%u %u%u%u%u %s",
				               index + 1u, count,
				               att.digits[0], att.digits[1],
				               att.digits[2], att.digits[3],
				               att.ok ? "OK" : "FALLO");
			}
			break;
		}

		case ST_SYS_MENU_CLOCK:

			/* Sin I2C: uso la copia que mantiene refresh_rtc_cache() */
			display_printf(0, "%02u:%02u:%02u",
			               rtc_cache.hours, rtc_cache.minutes, rtc_cache.seconds);
			display_printf(1, "%02u/%02u/%04u PB0",
			               rtc_cache.date, rtc_cache.month, rtc_cache.year);
			break;

		default:
			break;
	}
}

/* Transicion de estado con sus acciones de entrada */
static void enter_state(task_system_st_t state)
{
	task_system_dta.state    = state;
	task_system_dta.tick_ui  = DEL_SYS_MIN;	/* redibujar ya */
	task_system_dta.flag     = false;

	switch (state)
	{
		case ST_SYS_VERIFY:
		case ST_SYS_CHANGE:

			reset_digits();
			display_clear();
			break;

		case ST_SYS_MENU_SELECT:
		case ST_SYS_MENU_LOG:
		case ST_SYS_MENU_CLOCK:

			display_clear();
			break;

		case ST_SYS_SLEEP:

			/* la pantalla ya se apago en el llamador */
			break;

		default:
			break;
	}
}

static void show_message(const char *line0, const char *line1)
{
	display_clear();
	display_printf(0, "%s", line0);
	display_printf(1, "%s", line1);

	task_system_dta.tick_msg = DEL_MSG_SHOW_MS;
	task_system_dta.tick_ui  = DEL_MSG_SHOW_MS + 1ul;	/* que no redibuje encima */
	task_system_dta.state    = ST_SYS_MSG;
	task_system_dta.flag     = false;
}

static void reset_digits(void)
{
	task_system_dta.digit_index = 0;
	memset(task_system_dta.digit_buffer, 0, PASSWORD_LEN);
}

static void push_digit(uint8_t digit)
{
	if (task_system_dta.digit_index < PASSWORD_LEN)
	{
		task_system_dta.digit_buffer[task_system_dta.digit_index] = digit;
		task_system_dta.digit_index++;
	}
}

/* Se completaron los 4 digitos en modo NORMAL: comparo contra la EEPROM */
static void confirm_verify(void)
{
	bool b_ok;

	if (!storage_password_is_set())
	{
		show_message("No hay clave", "guardada aun");
		reset_digits();
		return;
	}

	b_ok = storage_password_matches(task_system_dta.digit_buffer);

	if (b_ok)
	{
		/* Clave correcta: desarmo y abro la traba.

		   door_seen_open arranca en false: hasta que la puerta no se abra de
		   verdad, ningun evento de "puerta cerrada" puede volver a trabar el
		   cerrojo. Lo unico que puede re-trabarlo es el timeout de abajo. */
		task_system_dta.disarmed       = true;
		task_system_dta.door_seen_open = false;
		task_system_dta.tick_relock    = DEL_RELOCK_MS;

		put_event_task_actuator(EV_ACT_PULSE, ID_OK);
		show_message("Clave correcta", "Puerta abierta");
	}
	else
	{
		put_event_task_actuator(EV_ACT_PULSE, ID_ALARM);
		show_message("Clave incorrecta", "");
	}

	/* Queda registrado el intento (bueno o malo) con fecha y hora.
	   Uso el cache del RTC (cero I2C) y storage_log_attempt() solo ENCOLA:
	   este camino, que es el mas caliente de todos, no toca el bus. */
	storage_log_attempt(task_system_dta.digit_buffer, b_ok, &rtc_cache);

	reset_digits();
}

/* Se completaron los 4 digitos en modo SET_UP: guardo la clave nueva */
static void confirm_change(void)
{
	char buf[LCD_COLS + 1];

	storage_save_password(task_system_dta.digit_buffer);

	snprintf(buf, sizeof(buf), "%u%u%u%u",
	         task_system_dta.digit_buffer[0], task_system_dta.digit_buffer[1],
	         task_system_dta.digit_buffer[2], task_system_dta.digit_buffer[3]);

	show_message("Clave guardada:", buf);

	reset_digits();
}

static bool is_activity_event(task_system_ev_t event)
{
	return ((EV_SYS_BTN_MODE    == event) ||
	        (EV_SYS_BTN_CONFIRM == event) ||
	        (EV_SYS_POT_MOVED   == event));
}

/* Relee el DS3231 una vez por segundo, y solo si el bus I2C esta libre en
   esta vuelta. Si esta ocupado, no espera: reintenta en el proximo tick. */
static void refresh_rtc_cache(void)
{
	if (DEL_SYS_MIN < tick_rtc)
	{
		tick_rtc--;
		return;
	}

	if (!bus_i2c_request())
	{
		return;		/* el bus lo tomo otra tarea: pruebo en el proximo tick */
	}

	DS3231_GetTime(&rtc_cache);
	tick_rtc = DEL_RTC_REFRESH_MS;
}

/********************** end of file ******************************************/
