/*
 * task_display.c
 *
 * ACTUAR (pantalla).
 *
 * El problema: escribir una linea de 16 caracteres en el LCD por I2C lleva
 * varios milisegundos. Si se hiciera de una, la vuelta del ejecutor ciclico
 * se pasaria larguisimo de 1 ms y se rompe el requisito de tiempo real.
 *
 * La solucion (patron "framebuffer + shadow"):
 *
 *   - Las demas tareas escriben lo que quieren mostrar en un framebuffer de
 *     2x16 chars en RAM. Eso es un memcpy: cuesta nanosegundos y NO bloquea.
 *   - Esta tarea compara framebuffer contra shadow (lo que el LCD realmente
 *     tiene escrito) y en cada tick manda UNA sola operacion I2C:
 *     o reposiciona el cursor, o escribe un caracter.
 *
 * Asi el costo por tick queda acotado y constante (una transaccion I2C de
 * 2 bytes x 2 nibbles). Una pantalla entera tarda a lo sumo unas decenas de
 * ms en volcarse, que para una UI es imperceptible.
 *
 * Ademas, como solo se mandan los caracteres que CAMBIARON, en regimen
 * permanente la tarea no genera trafico I2C practicamente nunca.
 */

/********************** inclusions *******************************************/
#include "main.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "board.h"
#include "app.h"
#include "bus_i2c.h"
#include "lcd_i2c.h"
#include "task_display_attribute.h"
#include "task_display_interface.h"

/********************** external data declaration ****************************/
extern I2C_HandleTypeDef hi2c1;

/********************** internal data definition *****************************/
task_display_dta_t task_display_dta;

const char *p_task_display   = "Task Display (LCD 16x2 I2C, framebuffer)";
const char *p_task_display_  = "Non-Blocking Code";
const char *p_task_display__ = "(Update by Time Code, period = 1mS)";

/********************** internal functions declaration ***********************/
static bool find_dirty_cell(uint8_t *p_row, uint8_t *p_col);

/********************** external functions definition ************************/
void task_display_init(void *parameters)
{
	uint8_t row;
	uint8_t col;

	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu",
	            GET_NAME(task_display_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_display), p_task_display);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_display), p_task_display_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_display), p_task_display__);

	/* La secuencia de arranque del HD44780 SI es bloqueante (el datasheet
	   exige esperas de ms). Se hace una sola vez aca, fuera del loop. */
	lcd_init();

	for (row = 0; row < LCD_ROWS; row++)
	{
		for (col = 0; col < LCD_COLS; col++)
		{
			task_display_dta.framebuffer[row][col] = ' ';
			task_display_dta.shadow[row][col]      = ' ';
		}
	}

	task_display_dta.state           = ST_DISP_ON;
	task_display_dta.hw_row          = 0;
	task_display_dta.hw_col          = 0;
	task_display_dta.hw_cursor_valid = false;
}

void task_display_update(void *parameters)
{
	uint8_t row;
	uint8_t col;

	switch (task_display_dta.state)
	{
		case ST_DISP_ON:

			/* Busco la primera celda cuyo contenido deseado difiera del que
			   el LCD tiene escrito. Si no hay ninguna, no hago I2C. */
			if (!find_dirty_cell(&row, &col))
			{
				break;
			}

			/* El bus I2C es de a uno por vuelta. Si otra tarea ya lo tomo,
			   me voy sin hacer nada y reintento en el proximo tick: la UI se
			   dibuja un milisegundo mas tarde y a nadie le importa. */
			if (!bus_i2c_request())
			{
				break;
			}

			/* Una sola operacion I2C por tick: o muevo el cursor, o escribo. */
			if ((!task_display_dta.hw_cursor_valid) ||
			    (task_display_dta.hw_row != row) ||
			    (task_display_dta.hw_col != col))
			{
				lcd_set_cursor(row, col);
				task_display_dta.hw_row          = row;
				task_display_dta.hw_col          = col;
				task_display_dta.hw_cursor_valid = true;
			}
			else
			{
				lcd_send_data(task_display_dta.framebuffer[row][col]);
				task_display_dta.shadow[row][col] = task_display_dta.framebuffer[row][col];

				/* El LCD auto-incrementa el cursor */
				task_display_dta.hw_col++;
				if (task_display_dta.hw_col >= LCD_COLS)
				{
					task_display_dta.hw_cursor_valid = false;
				}
			}
			break;

		case ST_DISP_SLEEPING:
		{
			uint8_t off = 0x00;		/* todo el PCF8574 en 0 => backlight off */

			if (!bus_i2c_request())
			{
				break;				/* reintento en el proximo tick */
			}

			lcd_send_cmd(0x08);		/* display off */
			HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, &off, 1, 10);

			task_display_dta.state = ST_DISP_OFF;
			break;
		}

		case ST_DISP_OFF:

			/* Dormido: no se toca el bus I2C. Aca es donde el sistema
			   realmente baja el consumo. */
			break;

		case ST_DISP_WAKING:

			if (!bus_i2c_request())
			{
				break;				/* reintento en el proximo tick */
			}

			lcd_send_cmd(0x0C);		/* display on (reenciende el backlight) */

			/* Invalido la shadow para forzar el redibujado completo */
			memset(task_display_dta.shadow, 0, sizeof(task_display_dta.shadow));
			task_display_dta.hw_cursor_valid = false;
			task_display_dta.state           = ST_DISP_ON;
			break;

		default:

			task_display_dta.state = ST_DISP_ON;
			break;
	}
}

/********************** interface (para las demas tareas) ********************/
void display_clear_row(uint8_t row)
{
	if (row < LCD_ROWS)
	{
		memset(task_display_dta.framebuffer[row], ' ', LCD_COLS);
	}
}

void display_clear(void)
{
	uint8_t row;

	for (row = 0; row < LCD_ROWS; row++)
	{
		display_clear_row(row);
	}
}

void display_write(uint8_t row, uint8_t col, const char *str)
{
	uint8_t i;

	if ((row >= LCD_ROWS) || (col >= LCD_COLS) || (NULL == str))
	{
		return;
	}

	for (i = col; (i < LCD_COLS) && ('\0' != *str); i++)
	{
		task_display_dta.framebuffer[row][i] = *str++;
	}
}

void display_printf(uint8_t row, const char *fmt, ...)
{
	char    line[LCD_COLS + 1];
	va_list args;

	if (row >= LCD_ROWS)
	{
		return;
	}

	va_start(args, fmt);
	vsnprintf(line, sizeof(line), fmt, args);
	va_end(args);

	/* Relleno con espacios hasta el final, asi no quedan restos del texto
	   anterior en la fila. */
	display_clear_row(row);
	display_write(row, 0, line);
}

void display_sleep(void)
{
	if (ST_DISP_ON == task_display_dta.state)
	{
		task_display_dta.state = ST_DISP_SLEEPING;
	}
}

void display_wake(void)
{
	if ((ST_DISP_OFF == task_display_dta.state) ||
	    (ST_DISP_SLEEPING == task_display_dta.state))
	{
		task_display_dta.state = ST_DISP_WAKING;
	}
}

/********************** internal functions definition ************************/

/* Devuelve true y la posicion de la primera celda "sucia" (framebuffer !=
   shadow). Recorre en orden de escritura del LCD, asi el auto-incremento
   del cursor acierta la mayoria de las veces y no hay que reposicionarlo. */
static bool find_dirty_cell(uint8_t *p_row, uint8_t *p_col)
{
	uint8_t row;
	uint8_t col;

	for (row = 0; row < LCD_ROWS; row++)
	{
		for (col = 0; col < LCD_COLS; col++)
		{
			if (task_display_dta.framebuffer[row][col] !=
			    task_display_dta.shadow[row][col])
			{
				*p_row = row;
				*p_col = col;
				return true;
			}
		}
	}

	return false;
}

/********************** end of file ******************************************/
