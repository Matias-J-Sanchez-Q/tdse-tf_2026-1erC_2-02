/*
 * task_system_interface.c
 *
 * Cola de eventos del Task System (productor/consumidor, buffer circular).
 *
 * IMPORTANTE: put_event_task_system() se llama tanto desde tareas como desde
 * la ISR del boton azul (EXTI). Por eso las operaciones sobre la cola son
 * SECCIONES CRITICAS: se deshabilitan las interrupciones para que una ISR no
 * corrompa los indices a mitad de una actualizacion.
 *
 * Si la cola se llena, el evento nuevo se DESCARTA. Nunca se bloquea ni se
 * pisa un evento viejo sin querer: perder una pulsacion es mucho menos grave
 * que colgar el ejecutor ciclico.
 */

/********************** inclusions *******************************************/
#include "main.h"
#include "logger.h"
#include "board.h"
#include "app.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"

/********************** macros and definitions *******************************/
#define QUEUE_LENGTH	(16ul)

typedef struct
{
	uint32_t			head;
	uint32_t			tail;
	uint32_t			count;
	task_system_ev_t	queue[QUEUE_LENGTH];
} event_task_system_queue_t;

/********************** internal data definition *****************************/
static volatile event_task_system_queue_t event_task_system_queue;

/* Diagnostico: cuantos eventos se perdieron por cola llena (mirar por
   Live Expressions; deberia quedarse en 0). */
volatile uint32_t g_event_task_system_lost;

/********************** external functions definition ************************/
void init_event_task_system(void)
{
	uint32_t i;

	event_task_system_queue.head  = 0;
	event_task_system_queue.tail  = 0;
	event_task_system_queue.count = 0;

	for (i = 0; i < QUEUE_LENGTH; i++)
	{
		event_task_system_queue.queue[i] = EV_SYS_NONE;
	}

	g_event_task_system_lost = 0;
}

void put_event_task_system(task_system_ev_t event)
{
	__asm("CPSID i");	/* seccion critica: tambien entra desde una ISR */

	if (QUEUE_LENGTH > event_task_system_queue.count)
	{
		event_task_system_queue.queue[event_task_system_queue.head] = event;

		event_task_system_queue.head++;
		if (QUEUE_LENGTH == event_task_system_queue.head)
		{
			event_task_system_queue.head = 0;
		}

		event_task_system_queue.count++;
	}
	else
	{
		g_event_task_system_lost++;
	}

	__asm("CPSIE i");
}

task_system_ev_t get_event_task_system(void)
{
	task_system_ev_t event = EV_SYS_NONE;

	__asm("CPSID i");

	if (0ul < event_task_system_queue.count)
	{
		event = event_task_system_queue.queue[event_task_system_queue.tail];
		event_task_system_queue.queue[event_task_system_queue.tail] = EV_SYS_NONE;

		event_task_system_queue.tail++;
		if (QUEUE_LENGTH == event_task_system_queue.tail)
		{
			event_task_system_queue.tail = 0;
		}

		event_task_system_queue.count--;
	}

	__asm("CPSIE i");

	return event;
}

bool any_event_task_system(void)
{
	bool b_any;

	__asm("CPSID i");
	b_any = (0ul < event_task_system_queue.count);
	__asm("CPSIE i");

	return b_any;
}

/********************** end of file ******************************************/
