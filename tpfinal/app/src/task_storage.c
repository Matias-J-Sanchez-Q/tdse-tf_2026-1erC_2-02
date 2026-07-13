/*
 * task_storage.c
 *
 * PERSISTENCIA (SET_UP en EEPROM externa AT24C32, I2C).
 *
 * El problema: la AT24C32 necesita ~5 ms de ciclo de escritura interno
 * despues de cada pagina. El driver original resolvia eso con HAL_Delay(5),
 * o sea bloqueando el micro 5 ms enteros: 5000 veces el presupuesto de la
 * vuelta del ejecutor ciclico.
 *
 * La solucion:
 *
 *   - Todas las escrituras se ENCOLAN (storage_save_password,
 *     storage_log_attempt). El que llama vuelve al instante.
 *   - Esta tarea las despacha de a una:
 *
 *       ST_STO_IDLE   -> hay algo en la cola?  -> ST_STO_WRITE
 *       ST_STO_WRITE  -> manda la transaccion I2C (una pagina, <= 16 bytes)
 *                        y arranca el contador                -> ST_STO_WAIT
 *       ST_STO_WAIT   -> descuenta 1 ms por tick; al llegar a 0 -> ST_STO_IDLE
 *
 *     Los 5 ms de espera se pagan con el tick del sistema, no con HAL_Delay:
 *     mientras tanto las demas tareas siguen corriendo normalmente.
 *
 *   - Todas las LECTURAS salen de un espejo en RAM que se carga una sola vez
 *     en el init. Durante el loop no hay ni una lectura I2C a la EEPROM, asi
 *     que el registro de intentos se puede pintar en pantalla gratis.
 *
 * Los registros son de 16 bytes alineados a 16, asi que ninguna escritura
 * cruza un limite de pagina y cada una es UNA sola transaccion.
 */

/********************** inclusions *******************************************/
#include "main.h"
#include <string.h>

#include "logger.h"
#include "board.h"
#include "app.h"
#include "bus_i2c.h"
#include "at24c32.h"
#include "task_storage_attribute.h"
#include "task_storage_interface.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"

/********************** macros and definitions *******************************/
#define EEPROM_WRITE_CYCLE_MS	(6ul)	/* 5 ms de datasheet + 1 de margen */
#define I2C_TIMEOUT_MS			(5ul)

/* Offsets dentro de un registro de intento (16 bytes) */
#define REC_YEAR	(0u)
#define REC_MONTH	(1u)
#define REC_DATE	(2u)
#define REC_HOUR	(3u)
#define REC_MINUTE	(4u)
#define REC_SECOND	(5u)
#define REC_DIGITS	(6u)	/* 6..9 */
#define REC_OK		(10u)
/* 11..15 sin usar (padding para alinear a 16) */

/********************** external data declaration ****************************/
extern I2C_HandleTypeDef hi2c1;

/********************** internal data definition *****************************/
task_storage_dta_t task_storage_dta;

/* Diagnostico: escrituras descartadas por cola llena (deberia quedarse en 0) */
volatile uint32_t g_storage_dropped;

const char *p_task_storage   = "Task Storage (AT24C32 EEPROM)";
const char *p_task_storage_  = "Non-Blocking Code";
const char *p_task_storage__ = "(Update by Time Code, period = 1mS)";

/********************** internal functions declaration ***********************/
static bool  queue_push(uint16_t addr, const uint8_t *data, uint8_t len);
static void  load_password_from_eeprom(void);
static void  load_attempts_from_eeprom(void);
static void  enqueue_attempt_header(void);
static uint16_t slot_addr(uint8_t slot);

/********************** external functions definition ************************/
void task_storage_init(void *parameters)
{
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu",
	            GET_NAME(task_storage_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_storage), p_task_storage);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_storage), p_task_storage_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_storage), p_task_storage__);

	task_storage_dta.state = ST_STO_IDLE;
	task_storage_dta.tick  = 0;
	task_storage_dta.head  = 0;
	task_storage_dta.tail  = 0;
	task_storage_dta.count = 0;

	/* Estas lecturas SI son bloqueantes, pero pasan una sola vez antes de
	   arrancar el ejecutor ciclico, asi que no afectan el WCET del loop. */
	load_password_from_eeprom();
	load_attempts_from_eeprom();

	LOGGER_INFO("   password_set = %s, attempts = %u",
	            task_storage_dta.password_set ? "true" : "false",
	            task_storage_dta.attempt_count);
}

void task_storage_update(void *parameters)
{
	storage_op_t *p_op;

	switch (task_storage_dta.state)
	{
		case ST_STO_IDLE:

			if (0u < task_storage_dta.count)
			{
				task_storage_dta.state = ST_STO_WRITE;
			}
			break;

		case ST_STO_WRITE:

			/* El bus es de a uno por vuelta. Si el display ya lo tomo, me
			   quedo en este estado y escribo en el proximo tick. Total, la
			   escritura ya es asincronica: nadie la esta esperando. */
			if (!bus_i2c_request())
			{
				break;
			}

			p_op = &task_storage_dta.queue[task_storage_dta.tail];

			/* Una sola transaccion, <= 16 bytes, sin cruzar pagina.
			   A 400 kHz son ~0,4 ms: entra en la vuelta de 1 ms. */
			HAL_I2C_Mem_Write(&hi2c1, AT24C32_ADDRESS, p_op->addr,
			                  I2C_MEMADD_SIZE_16BIT,
			                  p_op->data, p_op->len, I2C_TIMEOUT_MS);

			/* Consumo la op */
			task_storage_dta.tail = (uint8_t)((task_storage_dta.tail + 1u) % STORAGE_QUEUE_LENGTH);
			task_storage_dta.count--;

			/* Los 5 ms de ciclo de escritura los cuenta el tick, NO HAL_Delay */
			task_storage_dta.tick  = EEPROM_WRITE_CYCLE_MS;
			task_storage_dta.state = ST_STO_WAIT;
			break;

		case ST_STO_WAIT:

			if (0u < task_storage_dta.tick)
			{
				task_storage_dta.tick--;
			}
			else
			{
				if (0u < task_storage_dta.count)
				{
					task_storage_dta.state = ST_STO_WRITE;
				}
				else
				{
					task_storage_dta.state = ST_STO_IDLE;
					put_event_task_system(EV_SYS_STORAGE_DONE);
				}
			}
			break;

		default:

			task_storage_dta.state = ST_STO_IDLE;
			break;
	}
}

/********************** interface *******************************************/
bool storage_password_is_set(void)
{
	return task_storage_dta.password_set;
}

bool storage_password_matches(const uint8_t *digits)
{
	if ((NULL == digits) || (!task_storage_dta.password_set))
	{
		return false;
	}

	return (0 == memcmp(digits, task_storage_dta.password, PASSWORD_LEN));
}

void storage_save_password(const uint8_t *digits)
{
	uint8_t rec[PWD_REC_SIZE];
	uint8_t i;

	if (NULL == digits)
	{
		return;
	}

	rec[0] = PWD_MAGIC;
	rec[1] = 1u;
	for (i = 0; i < PASSWORD_LEN; i++)
	{
		rec[2u + i] = digits[i];
	}

	/* El espejo en RAM se actualiza SOLO si la escritura se pudo encolar.
	   Si la cola estaba llena y se actualizaba igual, la clave andaba hasta
	   el proximo reset y despues volvia a ser la vieja: el usuario quedaba
	   afuera de su propia cerradura sin entender por que. Prefiero que el
	   cambio falle de una y se pueda reintentar. */
	if (queue_push(PWD_STORE_ADDR, rec, PWD_REC_SIZE))
	{
		for (i = 0; i < PASSWORD_LEN; i++)
		{
			task_storage_dta.password[i] = digits[i];
		}
		task_storage_dta.password_set = true;
	}
	else
	{
		g_storage_dropped++;
	}
}

uint8_t storage_attempt_count(void)
{
	return task_storage_dta.attempt_count;
}

bool storage_get_attempt(uint8_t index, storage_attempt_t *p_out)
{
	uint8_t slot;

	if ((NULL == p_out) || (index >= task_storage_dta.attempt_count))
	{
		return false;
	}

	/* index 0 = el mas viejo. El mas viejo esta en (next - count) del ring. */
	slot = (uint8_t)((task_storage_dta.attempt_next + ATTEMPT_MAX -
	                  task_storage_dta.attempt_count + index) % ATTEMPT_MAX);

	*p_out = task_storage_dta.attempts[slot];
	return true;
}

void storage_log_attempt(const uint8_t *digits, bool ok, const DS3231_Time *t)
{
	uint8_t            rec[ATTEMPT_REC_SIZE];
	storage_attempt_t *p_ram;
	uint8_t            slot;
	uint8_t            i;

	if ((NULL == digits) || (NULL == t))
	{
		return;
	}

	slot = task_storage_dta.attempt_next;

	/* 1) actualizo el espejo en RAM (asi la UI ya lo ve) */
	p_ram         = &task_storage_dta.attempts[slot];
	p_ram->year   = (uint8_t)(t->year - 2000u);
	p_ram->month  = t->month;
	p_ram->date   = t->date;
	p_ram->hour   = t->hours;
	p_ram->minute = t->minutes;
	p_ram->second = t->seconds;
	p_ram->ok     = ok;
	for (i = 0; i < PASSWORD_LEN; i++)
	{
		p_ram->digits[i] = digits[i];
	}

	/* 2) armo el registro binario y lo encolo */
	memset(rec, 0, sizeof(rec));
	rec[REC_YEAR]   = p_ram->year;
	rec[REC_MONTH]  = p_ram->month;
	rec[REC_DATE]   = p_ram->date;
	rec[REC_HOUR]   = p_ram->hour;
	rec[REC_MINUTE] = p_ram->minute;
	rec[REC_SECOND] = p_ram->second;
	for (i = 0; i < PASSWORD_LEN; i++)
	{
		rec[REC_DIGITS + i] = digits[i];
	}
	rec[REC_OK] = ok ? 1u : 0u;

	(void)queue_push(slot_addr(slot), rec, ATTEMPT_REC_SIZE);

	/* 3) avanzo el ring y encolo el header actualizado */
	task_storage_dta.attempt_next = (uint8_t)((slot + 1u) % ATTEMPT_MAX);
	if (task_storage_dta.attempt_count < ATTEMPT_MAX)
	{
		task_storage_dta.attempt_count++;
	}

	enqueue_attempt_header();
}

bool storage_is_busy(void)
{
	return (ST_STO_IDLE != task_storage_dta.state);
}

/********************** internal functions definition ************************/
static uint16_t slot_addr(uint8_t slot)
{
	return (uint16_t)(ATTEMPT_BASE_ADDR + ((uint16_t)slot * ATTEMPT_REC_SIZE));
}

static bool queue_push(uint16_t addr, const uint8_t *data, uint8_t len)
{
	storage_op_t *p_op;

	if ((task_storage_dta.count >= STORAGE_QUEUE_LENGTH) ||
	    (len > ATTEMPT_REC_SIZE))
	{
		return false;	/* cola llena: se descarta (no se bloquea nunca) */
	}

	p_op       = &task_storage_dta.queue[task_storage_dta.head];
	p_op->addr = addr;
	p_op->len  = len;
	memcpy(p_op->data, data, len);

	task_storage_dta.head = (uint8_t)((task_storage_dta.head + 1u) % STORAGE_QUEUE_LENGTH);
	task_storage_dta.count++;

	return true;
}

static void enqueue_attempt_header(void)
{
	uint8_t hdr[ATTEMPT_HDR_SIZE];

	hdr[0] = ATTEMPT_MAGIC_HI;
	hdr[1] = ATTEMPT_MAGIC_LO;
	hdr[2] = task_storage_dta.attempt_next;
	hdr[3] = task_storage_dta.attempt_count;

	(void)queue_push(ATTEMPT_HDR_ADDR, hdr, ATTEMPT_HDR_SIZE);
}

static void load_password_from_eeprom(void)
{
	uint8_t rec[PWD_REC_SIZE];
	uint8_t i;

	AT24C32_ReadBuffer(PWD_STORE_ADDR, rec, PWD_REC_SIZE);

	if ((PWD_MAGIC == rec[0]) && (1u == rec[1]))
	{
		for (i = 0; i < PASSWORD_LEN; i++)
		{
			task_storage_dta.password[i] = rec[2u + i];
		}
		task_storage_dta.password_set = true;
	}
	else
	{
		memset(task_storage_dta.password, 0, PASSWORD_LEN);
		task_storage_dta.password_set = false;
	}
}

static void load_attempts_from_eeprom(void)
{
	uint8_t hdr[ATTEMPT_HDR_SIZE];
	uint8_t rec[ATTEMPT_REC_SIZE];
	uint8_t slot;
	uint8_t i;

	AT24C32_ReadBuffer(ATTEMPT_HDR_ADDR, hdr, ATTEMPT_HDR_SIZE);

	if ((ATTEMPT_MAGIC_HI == hdr[0]) && (ATTEMPT_MAGIC_LO == hdr[1]) &&
	    (hdr[2] < ATTEMPT_MAX) && (hdr[3] <= ATTEMPT_MAX))
	{
		task_storage_dta.attempt_next  = hdr[2];
		task_storage_dta.attempt_count = hdr[3];
	}
	else
	{
		/* EEPROM virgen o con un formato viejo: la formateo */
		task_storage_dta.attempt_next  = 0;
		task_storage_dta.attempt_count = 0;
		enqueue_attempt_header();
		memset(task_storage_dta.attempts, 0, sizeof(task_storage_dta.attempts));
		return;
	}

	/* Cargo el espejo en RAM de todo el ring */
	for (slot = 0; slot < ATTEMPT_MAX; slot++)
	{
		AT24C32_ReadBuffer(slot_addr(slot), rec, ATTEMPT_REC_SIZE);

		task_storage_dta.attempts[slot].year   = rec[REC_YEAR];
		task_storage_dta.attempts[slot].month  = rec[REC_MONTH];
		task_storage_dta.attempts[slot].date   = rec[REC_DATE];
		task_storage_dta.attempts[slot].hour   = rec[REC_HOUR];
		task_storage_dta.attempts[slot].minute = rec[REC_MINUTE];
		task_storage_dta.attempts[slot].second = rec[REC_SECOND];
		task_storage_dta.attempts[slot].ok     = (0u != rec[REC_OK]);

		for (i = 0; i < PASSWORD_LEN; i++)
		{
			task_storage_dta.attempts[slot].digits[i] = rec[REC_DIGITS + i];
		}
	}
}

/********************** end of file ******************************************/
