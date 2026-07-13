/*
 * task_analog_interface.h
 *
 * Interfaz de LECTURA del Task Analog. Las demas tareas no acceden nunca al
 * ADC ni a task_analog_dta directamente: piden el valor por aca.
 */

#ifndef TASK_ANALOG_INTERFACE_H_
#define TASK_ANALOG_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Digito 0..9 que esta seleccionando el potenciometro */
extern uint8_t analog_get_digit(void);

/* Valor crudo del potenciometro (0..4095). Sirve para mapear N opciones. */
extern uint16_t analog_get_raw(void);

/* Mapea la posicion del pote a un indice 0..(options-1). options > 0. */
extern uint8_t analog_map_to_option(uint8_t options);

/* Ultima medicion del monitor de tension (PA7) en mV */
extern uint16_t analog_get_mv(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_ANALOG_INTERFACE_H_ */
