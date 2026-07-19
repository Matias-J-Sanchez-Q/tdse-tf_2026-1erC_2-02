/*
 * app.h - Ejecutor ciclico (Cyclic Executive)
 */

#ifndef APP_H_
#define APP_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include <stdint.h>
#include <stdbool.h>

/********************** external data declaration ****************************/
extern uint32_t g_app_cnt;			/* vueltas ejecutadas desde el arranque */

extern volatile uint32_t g_app_tick_cnt;	/* ticks de 1 ms pendientes      */

/* Metricas de tiempo real (Live Expressions) */
extern volatile uint32_t g_app_runtime_us;	/* ultima vuelta, en us          */
extern volatile uint32_t g_app_wcet_us;		/* peor vuelta, en us            */
extern volatile uint32_t g_app_u_x1000;		/* U x1000 (1000 = 100% de CPU)  */
extern volatile uint32_t g_app_u_pct;		/* U en %                        */
extern volatile uint32_t g_app_overrun_cnt;	/* deadlines perdidos (debe = 0) */

/********************** external functions declaration ***********************/
extern void app_init(void);
extern void app_update(void);

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* APP_H_ */

/********************** end of file ******************************************/
