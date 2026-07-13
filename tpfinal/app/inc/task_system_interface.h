/*
 * task_system_interface.h - Cola de eventos del Task System
 */

#ifndef TASK_SYSTEM_INTERFACE_H_
#define TASK_SYSTEM_INTERFACE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include <stdbool.h>
#include "task_system_attribute.h"

/********************** external functions declaration ***********************/

/* Inicializa la cola. La llama task_system_init(). */
extern void init_event_task_system(void);

/* Encola un evento. Es SEGURA desde una ISR (seccion critica adentro). */
extern void put_event_task_system(task_system_ev_t event);

/* Saca el evento mas viejo. Devuelve EV_SYS_NONE si la cola esta vacia. */
extern task_system_ev_t get_event_task_system(void);

/* true si hay al menos un evento pendiente */
extern bool any_event_task_system(void);

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_SYSTEM_INTERFACE_H_ */

/********************** end of file ******************************************/
