/*
 * bus_i2c.h - Arbitro del bus I2C
 *
 * EL PROBLEMA
 * -----------
 * Tres tareas comparten el mismo bus I2C: task_display (LCD), task_storage
 * (EEPROM) y task_system (RTC). Cada transaccion I2C es tiempo de RELOJ DE
 * PARED: no importa cuan rapido corra el CPU, mover 19 bytes a 400 kHz tarda
 * ~430 us y punto.
 *
 * Si en un mismo tick las tres tareas usan el bus, se suman:
 *
 *      LCD (~250 us) + EEPROM (~430 us) + RTC (~250 us) = ~930 us
 *
 * y con eso solo ya casi no entra en el presupuesto de 1 ms. Sumandole el
 * resto de las tareas, la vuelta se pasa del deadline. Eso es exactamente lo
 * que hacia que el factor de uso U se fuera arriba de 100%.
 *
 * LA SOLUCION
 * -----------
 * Un token: en cada vuelta del ejecutor ciclico se puede hacer UNA sola
 * transaccion I2C. La primera tarea que lo pide se lo lleva; las demas se
 * quedan sin bus y REINTENTAN en el proximo tick (no esperan, no bloquean).
 *
 * Asi el peor caso de I2C por vuelta queda acotado a una sola transaccion,
 * y el WCET del sistema es predecible.
 *
 * Uso:
 *      if (!bus_i2c_request())
 *      {
 *          return;     // el bus ya esta tomado: pruebo en el proximo tick
 *      }
 *      HAL_I2C_...     // tengo el bus para mi
 */

#ifndef BUS_I2C_H_
#define BUS_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* La llama el ejecutor ciclico al empezar cada vuelta: libera el token. */
extern void bus_i2c_new_cycle(void);

/* Pide el bus para esta vuelta. true = te lo quedaste, false = ya lo tomo
   otra tarea, reintenta en el proximo tick. */
extern bool bus_i2c_request(void);

/* Diagnostico: cuantas veces una tarea se quedo sin bus y tuvo que esperar
   un tick. Que sea > 0 esta bien; que crezca sin parar significa que el bus
   esta saturado. */
extern volatile uint32_t g_bus_i2c_deferred;

#ifdef __cplusplus
}
#endif

#endif /* BUS_I2C_H_ */
