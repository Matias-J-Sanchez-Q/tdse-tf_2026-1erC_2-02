Cerradura Electronica - TA134 Taller de Sistemas Embebidos - Trabajo Final

Arquitectura: Bare Metal - Event-Triggered System (ETS)
Placa: NUCLEO-F103 (STM32CubeIDE 1.19.0, STM32Cube FW_F1 V1.8.6)

  SysTick => 1000 ticks/s (1 ms)
  Ejecutor ciclico: 1 vuelta por tick, y despues WFI (bajo consumo)


ESTRUCTURA
==========

  Core/Src/main.c
    SOLO inicializa hardware (reloj, GPIO, ADC1, ADC2, I2C1, USART2, TIM4)
    y arranca el ejecutor ciclico. Cero logica de aplicacion.
    Tambien tiene la ISR del boton azul (EXTI), que solo encola un evento.

  app/inc/board.h
    Unico lugar donde viven los numeros de pin. Ninguna tarea los conoce.

  app/src/app.c            EJECUTOR CICLICO
    Recorre la lista de tareas una vez por tick. Mide NOE / LET / BCET / WCET
    por tarea con el DWT, calcula el factor de uso U y cuenta deadlines
    perdidos. Al terminar la vuelta entra en SLEEP (WFI).

  --- ESCRUTAR ---

  app/src/task_sensor.c    Entradas digitales (PB0, sensor de puerta).
                           FSM de antirrebote de 4 estados por sensor.
                           Publica eventos en la cola del Task System.

  app/src/task_analog.c    Entradas analogicas (pote PA0, monitor PA7).
                           Multiplexa los dos ADC de a un canal por tick y
                           NO espera la conversion: la lee al tick siguiente.

  --- PROCESAR ---

  app/src/task_system.c    FSM de la cerradura. La unica tarea con logica de
                           negocio. No toca ni un GPIO: consume eventos y le
                           pide cosas al resto por sus interfaces.

                           Modos:  NORMAL -> ST_SYS_VERIFY, ST_SYS_MSG
                                   SET_UP -> ST_SYS_CHANGE, ST_SYS_MENU_*
                                   REPOSO -> ST_SYS_SLEEP (bajo consumo)

  --- ACTUAR ---

  app/src/task_actuator.c  Unico dueno de las salidas: LED de estado, alarma,
                           salida "clave OK" y servo del cerrojo. Pulsos y
                           parpadeos por contador de ticks, sin HAL_Delay.

  app/src/task_display.c   LCD 16x2 I2C. Patron framebuffer + shadow: las
                           tareas escriben en RAM (instantaneo) y el display
                           vuelca al LCD UN caracter por tick. Asi una linea
                           de 16 chars no rompe el presupuesto de 1 ms.

  app/src/task_storage.c   EEPROM AT24C32 (clave + registro de intentos).
                           Las escrituras se ENCOLAN y los 5 ms de ciclo de
                           escritura de la EEPROM se pagan con el tick, no con
                           HAL_Delay. Las lecturas salen de un espejo en RAM.

  --- INTERFACES ---

  task_system_interface.c   Cola de eventos (buffer circular). Es segura desde
                            una ISR: las operaciones son secciones criticas.
  task_actuator_interface.c Manda eventos a un actuador por su ID.
  task_analog_interface.h   Getters de las mediciones.
  task_display_interface.h  display_printf(), display_sleep(), ...
  task_storage_interface.h  Clave y registro de intentos.

  --- DRIVERS (Core) ---

  lcd_i2c.c   LCD 16x2 via PCF8574   (I2C, 0x27)
  ds3231.c    RTC                    (I2C, 0x68)
  at24c32.c   EEPROM 4 KB            (I2C, 0x57)
  servo.c     SG90 en TIM4_CH1 / PB6 (PWM 50 Hz)

  logger.h/.c  printf redirigido a la consola
  dwt.h        contador de ciclos (medicion de WCET)


REGLA DE ORO
============

  Ninguna tarea puede bloquear. Nada de HAL_Delay() dentro del loop.
  La suma de los WCET de todas las tareas tiene que dar menos de 1 ms.

  Para verificarlo, mirar por Live Expressions:

      g_app_wcet_us       peor vuelta completa, en us   (tiene que ser < 1000)
      g_app_u_pct         factor de uso de CPU, en %    (tiene que ser < 100)
      g_app_overrun_cnt   deadlines perdidos            (tiene que ser 0)
      task_dta_list[]     NOE / LET / BCET / WCET por tarea

      g_event_task_system_lost   eventos perdidos por cola llena (debe ser 0)


PENDIENTE (requisitos de la consigna que todavia no estan)
==========================================================

  - Modulo HM-10 (Bluetooth) + App
  - Dip switches
  - Diagramas de estado dibujados (las FSM ya estan en el codigo)
