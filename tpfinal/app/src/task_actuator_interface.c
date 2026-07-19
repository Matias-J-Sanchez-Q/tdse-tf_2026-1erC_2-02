/*
 * task_actuator_interface.c - Interfaz del Task Actuator
 */

/********************** inclusions *******************************************/
#include "main.h"
#include "logger.h"
#include "board.h"
#include "app.h"
#include "task_actuator_attribute.h"
#include "task_actuator_interface.h"

/********************** external functions definition ************************/
void put_event_task_actuator(task_actuator_ev_t event, task_actuator_id_t identifier)
{
	task_actuator_dta_t *p_task_actuator_dta;

	p_task_actuator_dta = &task_actuator_dta_list[identifier];

	p_task_actuator_dta->event = event;
	p_task_actuator_dta->flag  = true;
}

/********************** end of file ******************************************/
