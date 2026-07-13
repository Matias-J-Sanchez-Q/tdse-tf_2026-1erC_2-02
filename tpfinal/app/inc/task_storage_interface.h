/*
 * task_storage_interface.h
 *
 * Interfaz del Task Storage. Es la unica puerta de entrada a la EEPROM:
 * ninguna otra tarea incluye at24c32.h.
 *
 * Todas las funciones de LECTURA son instantaneas (leen el espejo en RAM).
 * Todas las funciones de ESCRITURA solo ENCOLAN: vuelven enseguida y el
 * Task Storage se encarga del I2C y de los 5 ms de ciclo de escritura de
 * la EEPROM sin bloquear el ejecutor ciclico.
 */

#ifndef TASK_STORAGE_INTERFACE_H_
#define TASK_STORAGE_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "task_storage_attribute.h"
#include "ds3231.h"

/* --- Clave ------------------------------------------------------------- */

/* true si hay una clave valida guardada */
extern bool storage_password_is_set(void);

/* true si los PASSWORD_LEN digitos coinciden con la clave guardada */
extern bool storage_password_matches(const uint8_t *digits);

/* Encola el guardado de una clave nueva (se hace efectivo en unos ms) */
extern void storage_save_password(const uint8_t *digits);

/* --- Registro de intentos ---------------------------------------------- */

/* Cantidad de intentos guardados (0..ATTEMPT_MAX) */
extern uint8_t storage_attempt_count(void);

/* Devuelve el intento numero index (0 = el mas viejo). Lectura instantanea.
   Devuelve false si index esta fuera de rango. */
extern bool storage_get_attempt(uint8_t index, storage_attempt_t *p_out);

/* Encola el guardado de un intento (correcto o fallido) con su timestamp */
extern void storage_log_attempt(const uint8_t *digits, bool ok, const DS3231_Time *t);

/* true si la EEPROM esta ocupada escribiendo */
extern bool storage_is_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_STORAGE_INTERFACE_H_ */
