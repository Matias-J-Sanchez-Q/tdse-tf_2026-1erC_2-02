/*
 * task_storage_attribute.h - Estados, datos y mapa de la EEPROM AT24C32
 */

#ifndef TASK_STORAGE_ATTRIBUTE_H_
#define TASK_STORAGE_ATTRIBUTE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "task_system_attribute.h"		/* PASSWORD_LEN */

/* ---------------------------------------------------------------------------
 * MAPA DE LA EEPROM (AT24C32: 4096 bytes, paginas de 32)
 *
 *   0x0000 - 0x0003   libre
 *   0x0004 - 0x0009   clave: [magic][set][d0][d1][d2][d3]
 *   0x0020 - 0x0023   header del ring de intentos: [magic_hi][magic_lo][next][count]
 *   0x0040 - 0x00DF   ring de intentos: 10 registros de 16 bytes
 *
 * Los registros son de 16 bytes y arrancan en direcciones multiplo de 16,
 * asi NINGUNA escritura cruza un limite de pagina de 32 bytes. Eso permite
 * resolver cada registro con UNA sola transaccion I2C, que es lo que hace
 * posible que la tarea sea acotada en tiempo.
 * ------------------------------------------------------------------------ */
#define PWD_STORE_ADDR			(0x0004u)
#define PWD_MAGIC				(0xC3u)
#define PWD_REC_SIZE			(2u + PASSWORD_LEN)

#define ATTEMPT_HDR_ADDR		(0x0020u)
#define ATTEMPT_HDR_SIZE		(4u)
#define ATTEMPT_MAGIC_HI		(0xA7u)
#define ATTEMPT_MAGIC_LO		(0x11u)

#define ATTEMPT_BASE_ADDR		(0x0040u)
#define ATTEMPT_MAX				(10u)
#define ATTEMPT_REC_SIZE		(16u)

/* Un intento de apertura, tal cual queda guardado */
typedef struct
{
	uint8_t	year;		/* anio - 2000 */
	uint8_t	month;
	uint8_t	date;
	uint8_t	hour;
	uint8_t	minute;
	uint8_t	second;
	uint8_t	digits[PASSWORD_LEN];
	bool	ok;
} storage_attempt_t;

/* ---------------------------------------------------------------------------
 * Cola de escrituras diferidas
 * ------------------------------------------------------------------------ */
#define STORAGE_QUEUE_LENGTH	(4u)

typedef struct
{
	uint16_t	addr;
	uint8_t		len;
	uint8_t		data[ATTEMPT_REC_SIZE];
} storage_op_t;

typedef enum task_storage_st
{
	ST_STO_IDLE,		/* sin trabajo pendiente                        */
	ST_STO_WRITE,		/* hay una op en la cola: la mando por I2C      */
	ST_STO_WAIT			/* espero el ciclo de escritura interno (5 ms)  */
} task_storage_st_t;

typedef struct
{
	task_storage_st_t	state;
	uint32_t			tick;			/* ms que faltan del ciclo de escritura */

	storage_op_t		queue[STORAGE_QUEUE_LENGTH];
	uint8_t				head;
	uint8_t				tail;
	uint8_t				count;

	/* Espejo en RAM de lo que hay en la EEPROM. Se carga UNA vez en el init.
	   Gracias a esto, ninguna lectura sale por I2C durante el loop: leer el
	   registro de intentos o la clave cuesta cero. */
	uint8_t				password[PASSWORD_LEN];
	bool				password_set;

	storage_attempt_t	attempts[ATTEMPT_MAX];
	uint8_t				attempt_next;	/* proximo slot a escribir */
	uint8_t				attempt_count;	/* cuantos hay guardados   */
} task_storage_dta_t;

extern task_storage_dta_t task_storage_dta;

#ifdef __cplusplus
}
#endif

#endif /* TASK_STORAGE_ATTRIBUTE_H_ */
