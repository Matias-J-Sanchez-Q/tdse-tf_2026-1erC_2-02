/*
 * task_actuator_interface.h - Interfaz del Task Actuator
 *
 * Unica forma de tocar una salida del sistema. Es una escritura directa a la
 * estructura de datos del actuador (no hay cola): el ultimo evento que llega
 * en el mismo tick es el que vale, que es exactamente lo que se quiere para
 * una salida.
 */

#ifndef TASK_ACTUATOR_INTERFACE_H_
#define TASK_ACTUATOR_INTERFACE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "task_actuator_attribute.h"

/********************** external functions declaration ***********************/
extern void put_event_task_actuator(task_actuator_ev_t event, task_actuator_id_t identifier);

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_ACTUATOR_INTERFACE_H_ */

/********************** end of file ******************************************/
