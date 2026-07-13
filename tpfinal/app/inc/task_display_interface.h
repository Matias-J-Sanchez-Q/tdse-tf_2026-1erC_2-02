/*
 * task_display_interface.h
 *
 * Interfaz del Task Display. El resto del sistema NUNCA llama a lcd_*()
 * directamente (esas funciones son bloqueantes): escribe en el framebuffer
 * con estas funciones, que son instantaneas, y el Task Display se encarga
 * de volcarlo al LCD de a un caracter por tick.
 */

#ifndef TASK_DISPLAY_INTERFACE_H_
#define TASK_DISPLAY_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Deja la fila en blanco */
extern void display_clear_row(uint8_t row);

/* Deja las dos filas en blanco */
extern void display_clear(void);

/* Escribe un texto en (row, col). Recorta a los 16 caracteres de la fila.
   NO borra el resto de la fila: para eso usar display_printf(), que
   completa con espacios. */
extern void display_write(uint8_t row, uint8_t col, const char *str);

/* Escribe una fila entera con formato, rellenando con espacios hasta el
   final. Es la que se usa normalmente desde el Task System. */
extern void display_printf(uint8_t row, const char *fmt, ...);

/* Apaga / enciende la pantalla y el backlight (modo reposo). */
extern void display_sleep(void);
extern void display_wake(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_DISPLAY_INTERFACE_H_ */
