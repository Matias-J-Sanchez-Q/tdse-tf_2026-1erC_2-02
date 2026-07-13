/*
 * task_storage.h - Task Storage (EEPROM AT24C32: clave + registro de intentos)
 */

#ifndef TASK_STORAGE_H_
#define TASK_STORAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

void task_storage_init(void *parameters);
void task_storage_update(void *parameters);

#ifdef __cplusplus
}
#endif

#endif /* TASK_STORAGE_H_ */
