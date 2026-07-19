/*
 * task_analog.h - Task Analog (entradas analogicas: pote + monitor de tension)
 */

#ifndef TASK_ANALOG_H_
#define TASK_ANALOG_H_

#ifdef __cplusplus
extern "C" {
#endif

void task_analog_init(void *parameters);
void task_analog_update(void *parameters);

#ifdef __cplusplus
}
#endif

#endif /* TASK_ANALOG_H_ */
