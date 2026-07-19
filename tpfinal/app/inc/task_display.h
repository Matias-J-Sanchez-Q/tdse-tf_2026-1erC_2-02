/*
 * task_display.h - Task Display (LCD 16x2 sobre I2C / PCF8574)
 */

#ifndef TASK_DISPLAY_H_
#define TASK_DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

void task_display_init(void *parameters);
void task_display_update(void *parameters);

#ifdef __cplusplus
}
#endif

#endif /* TASK_DISPLAY_H_ */
