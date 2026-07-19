/*
 * task_display_attribute.h - Estados y datos del Task Display (LCD 16x2 I2C)
 */

#ifndef TASK_DISPLAY_ATTRIBUTE_H_
#define TASK_DISPLAY_ATTRIBUTE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define LCD_ROWS		(2u)
#define LCD_COLS		(16u)

typedef enum task_display_st
{
	ST_DISP_ON,			/* refrescando la pantalla                */
	ST_DISP_SLEEPING,	/* apagando display + backlight (reposo)  */
	ST_DISP_OFF,		/* pantalla apagada, no se hace nada      */
	ST_DISP_WAKING		/* reencendiendo                          */
} task_display_st_t;

typedef struct
{
	task_display_st_t	state;

	/* Lo que el sistema QUIERE mostrar (lo escriben las otras tareas) */
	char				framebuffer[LCD_ROWS][LCD_COLS];

	/* Lo que el LCD YA tiene escrito (lo mantiene esta tarea) */
	char				shadow[LCD_ROWS][LCD_COLS];

	/* Posicion del cursor del hardware, para no re-posicionarlo al pedo */
	uint8_t				hw_row;
	uint8_t				hw_col;
	bool				hw_cursor_valid;
} task_display_dta_t;

extern task_display_dta_t task_display_dta;

#ifdef __cplusplus
}
#endif

#endif /* TASK_DISPLAY_ATTRIBUTE_H_ */
