
<img width="300" alt="FIUBA" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/logo-fiuba.png" />

**UNIVERSIDAD DE BUENOS AIRES**
**Facultad de Ingeniería**
**TA134 – Taller de Sistemas Embebidos**

# Memoria del Trabajo Final: Cerradura Electrónica de Alta Seguridad

Sistema de control de acceso multifunción con registro histórico y protección activa.

## Autores

| **Apellido, Nombre**  | **Padrón** |
| --------------------- | ---------- |
| Sanchez Q., Matias J. | 111060     |
| Laskowski, Marcos     | 104028     |
| Fanzi, Francisco      | 107510     |


**Docente**: Cruz, Juan Manuel 
**Tutor:** Lutenberg, Ariel

**Fecha:** Julio de 2026
**Cuatrimestre de cursada:** 1er cuatrimestre 2026
**Curso-Grupo:** 2-02

*Trabajo realizado en la Ciudad Autónoma de Buenos Aires.*


---

## Resumen

En este trabajo detallamos el diseño, desarrollo e implementación de una cerradura electrónica de alta seguridad basada en la plataforma NUCLEO-F103RB. A diferencia de los sistemas tradicionales con teclado, este dispositivo utiliza un potenciómetro para ingresar la clave de forma analógica (similar al dial de una caja fuerte) y una pantalla LCD para guiar al usuario. Al ingresar el código correcto, un servomotor se encarga de destrabar físicamente la puerta.

El sistema incluye:
 **Registro de accesos:** Guarda en una memoria externa no volátil la fecha y hora exacta de los últimos 10 intentos de apertura.
 **Sistema de alarmas:** Cuenta con un sensor magnético para detectar si la puerta fue forzada y un sensor de luz (LDR) para disparar una alarma sonora si alguien abre o vulnera el gabinete.

En cuanto al software, se programó un sistema eficiente y de bajo consumo. El procesador funciona de forma no bloqueante y, cada vez que termina de procesar las tareas pendientes, entra automáticamente en un modo de reposo (*sleep*). Esto asegura que el microcontrolador solo consuma energía durante las fracciones de segundo en las que realmente está trabajando, maximizando la duración de la batería.

---


## Registro de versiones

| Revisión | Cambios realizados | Fecha |
| :---: | --- | :---: |
| 1.0 | Creación del esqueleto y estructura base del documento. | 11/07/2026 |
| 1.1 | Redacción detallada, desarrollo y completado de las secciones del informe. | 15/07/2026 |


---

# Índice General

- [Capítulo 1: Introducción general](#capítulo-1-introducción-general)
- [Capítulo 2: Introducción específica](#capítulo-2-introducción-específica)
- [Capítulo 3: Diseño e implementación](#capítulo-3-diseño-e-implementación)
- [Capítulo 4: Ensayos y resultados](#capítulo-4-ensayos-y-resultados)
- [Capítulo 5: Conclusiones](#capítulo-5-conclusiones)
- [Capítulo 6: Uso de herramientas de IA](#capítulo-6-uso-de-herramientas-de-ia)
- [Capítulo 7: Bibliografía y referencias](#capítulo-7-bibliografía-y-referencias)

---

# Capítulo 1: Introducción general

## 1.1 Análisis de necesidad y objetivos

Los sistemas de seguridad modernos para cajas de seguridad enfrentan un doble desafío: bloquear eficazmente el acceso a personas no autorizadas y mantener un registro auditable de toda actividad. Para dar respuesta a esta necesidad, este proyecto se centra en la construcción de un prototipo de control de acceso autónomo y eficiente, diseñado para cumplir con los siguientes objetivos específicos:

1.  Autenticación local sin necesidad de teclados matriciales, utilizando un dial analógico.
2.  Almacenamiento persistente de un *log* de los últimos 10 intentos de acceso.
3.  Vigilancia activa del entorno físico (apertura de puerta y luminosidad interna del gabinete).
4.  Gestión eficiente del consumo, suspendiendo la interfaz visual y el CPU tras períodos de inactividad.

## 1.2 Comparativa de mercado

Los sistemas comerciales de control de acceso de gama de entrada suelen emplear teclados matriciales o de membrana. Estos componentes presentan una desventaja: el desgaste por uso continuo permite a una persona no autorizada deducir los dígitos que componen la clave. El prototipo desarrollado mitiga este riesgo reemplazando el teclado por un dial basado en un potenciómetro, el cual centraliza la interacción y evita dejar un patrón de desgaste diferencial.

Adicionalmente, se mejora la robustez del sistema de monitoreo mediante la implementación de un bus I²C. La sincronización de un RTC de precisión con una memoria EEPROM permite el guardado persistente de eventos con sus respectivas estampas temporales, proporcionando trazabilidad de accesos.

---

# Capítulo 2: Introducción específica

## 2.1 Requisitos del sistema

La Tabla 2.1 resume los requisitos funcionales del sistema, clasificados por área, junto con su estado de cumplimiento en la versión entregada.

| Grupo | ID | Descripción | Estado |
| :--- | :---: | :--- | :---: |
| **Gestión de Energía** | **RQ01** | El sistema debe operar en bajo consumo (REPOSO), suspendiendo el LCD y el CPU tras 15 s de inactividad, y despertando ante actividad del usuario. | Cumplido |
| **Interfaz de Usuario** | **RQ02** | El sistema debe proveer retroalimentación visual mostrando el estado del menú y los eventos en una pantalla LCD operada por bus I²C. | Cumplido |
| **Seguridad y Acceso** | **RQ03** | La contraseña estará formada por 4 dígitos numéricos (0 a 9), ingresados y confirmados individualmente. | Cumplido |
| | **RQ04** | El sistema debe permitir la selección de dígitos mapeando la tensión de un potenciómetro lineal. | Cumplido |
| | **RQ05** | El sistema debe habilitar el modo de configuración (SET_UP) para el cambio de contraseña **únicamente tras una apertura exitosa**. | **Cumplido**|
| **Actuación** | **RQ06** | Tras la validación exitosa, el sistema debe accionar un servomotor mediante PWM para liberar la traba física. | Cumplido |
| **Seguridad y Alarmas** | **RQ07** | En modo armado, la apertura no validada de la puerta o la detección de luz interna (LDR) debe activar una alarma visual y sonora. | Cumplido |
| | **RQ08** | El sistema debe advertir mediante señales visuales y sonoras el ingreso de una combinación incorrecta. | Cumplido |
| **Almacenamiento** | **RQ09** | El sistema debe registrar en la EEPROM la estampa de tiempo provista por el RTC tras cada intento de acceso. | Cumplido |
| | **RQ10** | El sistema debe recuperar de memoria y mostrar en el LCD el registro histórico de intentos. | Cumplido |

<p align="center"><em>Tabla 2.1: Requisitos funcionales del sistema y estado de cumplimiento.</em></p>

## 2.2 Casos de uso

**Caso de uso 1: ingreso de combinación**

| Elemento | Definición |
| :--- | :--- |
| Disparador | El usuario desea ingresar su combinación para abrir la caja fuerte. |
| Precondiciones | El sistema está en modo verificación, la puerta cerrada y sin alarmas activas. |
| Flujo principal | El usuario ingresa la secuencia de 4 dígitos con el dial, confirmando cada uno con el pulsador. El sistema valida contra la clave persistida en EEPROM; si es correcta, libera el servomotor, desactiva la alarma y registra el intento con la estampa del RTC. |
| Flujo alternativo | Clave incorrecta: el sistema pulsa la alarma (LED rojo + buzzer) durante 2 s, mantiene la traba puesta y registra el intento fallido. |
| Flujo alternativo | Clave correcta pero puerta nunca abierta: transcurridos 10 s, el cerrojo se vuelve a trabar y el sistema se re-activa automáticamente. |

**Caso de uso 2: consulta del historial de aperturas**

| Elemento | Definición |
| :--- | :--- |
| Disparador | El usuario quiere ver los últimos intentos de acceso. |
| Precondiciones | El sistema debe estar en modo MENÚ. |
| Flujo principal | El usuario navega al submenú de historial con el dial. El sistema recupera los últimos 10 intentos (fecha, hora, dígitos ingresados y resultado) y los muestra en el LCD, navegables con el potenciómetro. |
| Flujo alternativo | Inactividad: tras 15 s sin interacción, el sistema suspende el display y entra en REPOSO. |

**Caso de uso 3: cambio de clave de seguridad**

| Elemento | Definición |
| :--- | :--- |
| Disparador | El usuario necesita cambiar la clave maestra. |
| Precondiciones | El sistema debe estar en modo CAMBIO DE CLAVE. |
| Flujo principal | El usuario ingresa los 4 dígitos nuevos con el dial. Al confirmar el cuarto, la clave se encola para su escritura en EEPROM y el sistema muestra confirmación en pantalla. |
| Flujo alternativo | Si la cola de escritura de la EEPROM está saturada, el cambio se rechaza y la clave anterior se mantiene intacta, evitando un estado inconsistente entre RAM y memoria no volátil. |

## 2.3 Descripción de módulos y tecnologías utilizadas

### 2.3.1 Potenciómetro como dial analógico

Potenciómetro lineal de 10 kΩ. El valor de 12 bits se filtra y se mapea linealmente al rango de dígitos 0 a 9. El mismo canal se reutiliza para navegar las opciones del menú y para detectar actividad del usuario.

### 2.3.2 Divisor con fotorresistencia (LDR)

LDR-05 en divisor resistivo con una resistencia fija de 10 kΩ. Al abrirse el gabinete, la luz incidente modifica la tensión del divisor, el firmware compara esa tensión contra un umbral configurable.

### 2.3.3 Reloj de tiempo real y memoria EEPROM

Módulo combinado DS3231 (RTC) con EEPROM AT24C32 integrada y respaldo por batería CR2032, comunicado por I²C1    .

### 2.3.4 Display LCD

LCD 16x2 con *backpack* I²C basado en PCF8574, sobre el mismo bus I²C1.

### 2.3.5 Actuador de traba

Servomotor SG90 controlado por PWM, alimentado desde una fuente externa de 5 V independiente de la NUCLEO, con GND compartido.

### 2.3.6 Sensores digitales

Sensor magnético (MS-38BL) para detección de apertura de puerta (PA1) y *boton analogico* (TS4-5) para la confirmación de dígitos (PB0). Ambos con pull-up interno y antirrebote por software.

---


# Capítulo 3: Diseño e implementación

## 3.1 Hardware del sistema

Para entender la arquitectura física de nuestra cerradura, primero presentamos un **diagrama en bloques general** que muestra la conexión de todos los periféricos a la placa NUCLEO-F103RB y cómo se alimenta cada etapa (destacando el uso de una fuente externa de 5 V para el servomotor). 


<div align="center">
<img width="600" alt="Diagrama en bloques general" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/esquematico_completo.png" />
<p><em>Figura 3.1: Diagrama en bloques general del sistema.</em></p>
</div>


Luego, para analizar la electrónica en detalle, dividimos los circuitos en tres esquemas eléctricos específicos: la etapa de entradas (sensores analógicos y digitales), la topología de comunicación del bus I²C compartido, y la etapa de salidas que controla los actuadores y las alarmas.

<div align="center">
<img width="600" alt="Entradas analógicas y digitales" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/entrada_analogica_digitales.png" />
<p><em>Figura 3.2: Esquema eléctrico de las entradas del sistema.</em></p>
</div>

<div align="center">
<img width="600" alt="Bus I2C" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/bus_i2c.png" />
<p><em>Figura 3.3: Topología del bus I2C compartido.</em></p>
</div>

<div align="center">
<img width="600" alt="Salidas del sistema" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/salida.png" />
<p><em>Figura 3.4: Esquema eléctrico de las salidas y actuadores.</em></p>
</div>



Para complementar los diagramas teóricos, a continuación mostramos el montaje físico final de la cerradura. En estas imágenes se puede observar cómo fueron dispuestos la placa principal, la placa experimental soldada, los módulos y el cableado dentro del gabinete de madera.

<div align="center">
<img width="600" alt="Salidas del sistema" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/interior1.jpg" />
<p><em>Figura 3.5: Vista interior del gabinete mostrando el montaje físico y cableado general.</em></p>
</div>

<div align="center">
<img width="600" alt="Salidas del sistema" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/interior2.jpg" />
<p><em>Figura 3.6: Detalle de las conexiones sobre la placa experimental y disposición de los módulos.</em></p>
</div>

<div align="center">
<img width="600" alt="Salidas del sistema" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/frente.jpeg" />
<p><em>Figura 3.7: Vista frontal del proyecto.</em></p>
</div>


La Tabla 3.1 resume la asignación de pines. 

| Periférico | Pin(es) | Función |
| :--- | :--- | :--- |
| ADC1_IN0 | PA0 | Potenciómetro (dial de dígitos) |
| ADC2_IN7 | PA7 | Divisor LDR + 10 kΩ (detección de luz) |
| I²C1 (remapeado) | PB8 (SCL), PB9 (SDA) | LCD, RTC y EEPROM compartidos |
| TIM4_CH1 (PWM) | PB6 | Servomotor SG90 |
| EXTI | PC13 (B1) | Cambio de modo de operación (interrupción) |
| GPIO entrada | PB0 | Confirmación de dígito (pull-up, polling + antirrebote) |
| GPIO entrada | PA1 | Sensor magnético de puerta (pull-up, polling + antirrebote) |
| GPIO salida | PA4 | Alarma: LED rojo + buzzer |
| GPIO salida | PA6 | Salida "clave correcta" (LED verde) |
| GPIO salida | PA5 | LED de estado (LD2 de la NUCLEO) |

<p align="center"><em>Tabla 3.1: Asignación de pines del sistema.</em></p>

## 3.2 Diseño del firmware

### 3.2.1 Arquitectura: ejecutivo cíclico y Event-Triggered System

El firmware se organiza como un **ejecutivo cíclico** gobernado por el `SysTick`, configurado a 1000 ticks por segundo. El *callback* del SysTick incrementa un contador de ticks pendientes; el lazo principal detecta ese contador y, por cada tick pendiente, ejecuta **una vuelta completa** de la lista de tareas en orden fijo:

```
    ESCRUTAR              PROCESAR              ACTUAR
 ┌──────────────┐     ┌──────────────┐     ┌────────────────┐
 │ task_sensor  │     │ task_system  │     │ task_actuator  │
 │ task_analog  │───▶│    (FSM)     │───▶ │ task_display   │
 └──────────────┘     └──────────────┘     │ task_storage   │
                                           └────────────────┘
```

El orden no es arbitrario: garantiza que un evento generado por un sensor en el tick *N* sea procesado por la FSM y ejecutado por los actuadores **en ese mismo tick**, acotando la latencia de punta a punta a 1 ms.

Ninguna tarea bloquea. No se utiliza `HAL_Delay()` en el lazo principal: todas las temporizaciones (mensajes, refresco de UI, inactividad, pulsos de alarma, ciclo de escritura de la EEPROM) se resuelven con contadores de ticks.

Las tareas se comunican **exclusivamente por interfaces**, nunca por variables globales compartidas:

- `task_system_interface`: cola de eventos circular de 16 posiciones. Es segura frente a interrupciones (las operaciones sobre los índices son secciones críticas), ya que la ISR del botón B1 también encola eventos.
- `task_actuator_interface`: despacha eventos a un actuador identificado por su ID.
- `task_analog_interface`, `task_display_interface`, `task_storage_interface`: exponen la funcionalidad de cada tarea sin revelar su implementación.

### 3.2.2 Reloj del sistema

El sistema opera a **64 MHz** (HSI 8 MHz ÷ 2 × PLL 16). Esta configuración exige dos ajustes que no son opcionales: `FLASH_LATENCY_2` (dos estados de espera; con 0 el núcleo lee instrucciones corruptas) y un prescaler de ADC de ÷6, que deja el reloj de conversión en 10,67 MHz, por debajo del máximo de 14 MHz que admite el periférico.

### 3.2.3 Máquinas de estado

El sistema implementa cinco FSM independientes, una por tarea:

**FSM del sistema (`task_system`)** — es la única tarea con lógica de negocio y no accede a ningún periférico. Sus estados se corresponden con los modos de operación exigidos por la consigna:

| Modo | Estados |
| :--- | :--- |
| NORMAL | `ST_SYS_VERIFY`, `ST_SYS_MSG` |
| SET_UP | `ST_SYS_CHANGE`, `ST_SYS_MENU_SELECT`, `ST_SYS_MENU_LOG`, `ST_SYS_MENU_CLOCK` |
| REPOSO | `ST_SYS_SLEEP` |

**FSM de antirrebote (`task_sensor`)** — cuatro estados por entrada digital (`ST_BTN_UP`, `ST_BTN_FALLING`, `ST_BTN_DOWN`, `ST_BTN_RISING`), con ventana de 50 ms para el pulsador y 30 ms para el sensor magnético. Solo emite un evento cuando la transición queda confirmada.

**FSM de actuadores (`task_actuator`)** — `ST_ACT_OFF`, `ST_ACT_ON`, `ST_ACT_PULSE`, `ST_ACT_BLINK`. Es el único módulo del programa que escribe una salida física.

**FSM del display (`task_display`)** y **FSM de almacenamiento (`task_storage`)** — descritas en s3.3.

A continuación, se presenta el diagrama de la máquina de estados del sistema:

<div align="center">
<img width="900" alt="Salidas del sistema" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/diagrama_estados.jpeg" />
<p><em>Figura 3.7: Diagrama de la máquina de estados del sistema.</em></p>
</div>


### 3.2.4 Lógica de armado y re-armado del cerrojo

Una clave correcta **desactiva** el sistema y libera la traba. El re-activado exige un ciclo completo *abrir → cerrar* de la puerta: un evento de "puerta cerrada" aislado no puede volver a trabar el cerrojo. Adicionalmente, si el usuario acierta la clave pero nunca llega a abrir la puerta, un temporizador de 10 s vuelve a trabar el cerrojo y re-arma el sistema, evitando que una apertura autorizada deje la caja desprotegida indefinidamente.

Mientras la condición de alarma esté activa, la salida correspondiente se vuelve afirma en cada ciclo en lugar de accionarse solo en el flanco. Esto impide que un pulso de corta duración (por ejemplo, el aviso de clave incorrecta) deje la sirena en silencio con la puerta abierta.



## 3.3 Persistencia en EEPROM
 Cada registro de intento almacena la fecha y hora del RTC, los 4 dígitos ingresados y el resultado (correcto/incorrecto). El historial es un *buffer* circular: al llenarse, el intento más nuevo sobrescribe al más antiguo.


## 3.4 Bajo consumo

Para cuidar la batería, diseñamos el sistema de forma tal que no esté gastando energía sin sentido. Cuando el microcontrolador termina de ejecutar sus tareas del momento y se asegura de que no tiene ningún trabajo atrasado, lo mandamos a un "modo reposo". 

Básicamente, el procesador se pausa y solo se vuelve a despertar cuando necesita hacer algo nuevo. A diferencia de un programa tradicional que está funcionando al 100% todo el tiempo aunque no tenga nada para hacer, el cerebro de nuestra cerradura solo gasta energía el tiempo que realmente está trabajando.

Además de "dormir" al microcontrolador, cuando el sistema detecta que nadie está usando la cerradura, apagamos por completo la pantalla LCD y su luz de fondo, ya que es el componente que más energía consume.
---

# Capítulo 4: Ensayos y resultados

> **NOTA IMPORTANTE:** Las mediciones de las secciones 4.2 a 4.5 corresponden a la versión anterior del firmware (super-loop a 8 MHz) y **han sido invalidadas** por la reescritura descrita en el Capítulo 3. Todas deben volver a tomarse sobre el binario definitivo. Los valores se dejan explícitamente en blanco antes que reportar cifras que no se corresponden con el código entregado.

## 4.1 Prueba de integración (Video)

*   **Enlace al video:** https://drive.google.com/file/d/1aX3uYLCGlMD5LRpf_hL87mvzt0ArB9Q4/view?usp=drivesdk
## 4.2 Salida de la pantalla Console & Build Analyzer

| Sección | Tamaño [Bytes] |
| :--- | :---: |
| **.text** | 34.476|
| **.data** | 188|
| **.bss** | 2.868|

| Región | Usado [Bytes] | Total [Bytes] | % |
| :--- | :---: | :---: | :---: |
| FLASH | 34.664| 131072 | 26,45  |
| RAM | 3.056 | 20480 | 14,92  |



## 4.3 Medición y análisis de tiempos de ejecución (WCET)

### 4.3.1 Metodología

El firmware está instrumentado con el contador de ciclos del DWT (Data Watchpoint and Trace) del Cortex-M3. Antes de invocar cada tarea se reinicia el contador; al retornar se lee, obteniéndose el tiempo de ejecución de esa tarea en esa vuelta. Sobre esa base se acumulan, para cada tarea:

- **BCET** *(Best-Case Execution Time)* — mejor caso observado, en µs
- **WCET** *(Worst-Case Execution Time)* — **peor caso observado**, en µs

Todas estas variables son inspeccionables por *Live Expressions* en el arreglo `task_dta_list[]`. A nivel de sistema se registran además `g_app_runtime_us` (duración de la última vuelta completa), `g_app_wcet_us` (peor vuelta observada) y `g_app_overrun_cnt` (cantidad de vueltas que excedieron el período).

**Nota:** El WCET aquí reportado es un **peor caso observado**, no un peor caso demostrado por análisis estático. Para que la medición sea representativa, el sistema debe ejercitarse recorriendo los caminos más costosos: refresco completo del LCD, escritura de clave en EEPROM, navegación del registro de intentos y disparo de la alarma.

### 4.3.2 Resultados



| Tarea | BCET [µs] | WCET [µs] |
| :--- |  :---: | :---: |
| **Vuelta completa** |300 | 615|

## 4.4 Cálculo del Factor de Uso (U) de la CPU

El factor de uso se define como el cociente entre el tiempo de cómputo del peor caso y el período de despacho:

$$U = \frac{C}{T}$$

donde **T = 1000 µs** es el período del ejecutivo cíclico, fijado por el SysTick. T es un dato de diseño, no una incógnita: no se despeja a partir de U, sino que U se calcula a partir de él.

La condición de planificabilidad es **U < 1**. El firmware calcula este valor en tiempo de ejecución y lo expone en `g_app_u_pct`.

> - **Factor de uso (U = C/T):** 43 %


## 4.5 Medición y análisis de consumo

| Modo de operación | Corriente 
| :--- | :---: |
| Ingresando clave (NORMAL) | 7,6 mA| 
| Apertura de puerta (servo + lógica) | 12,4 mA | 
| Reposo (display off + WFI) | 4.7 mA|

<p align="center"><em>Tabla 4.1: Consumo energético del sistema.</em></p>


---

# Capítulo 5: Conclusiones

## 5.1 Resultados obtenidos

El prototipo cumple con la mayoría de los requisitos funcionales planteados. La integración del RTC DS3231 con la memoria EEPROM sobre un mismo bus I²C permitió construir un sistema de auditoría persistente, y el aislamiento de la alimentación del servomotor evitó que sus picos de consumo perturbaran el ADC del microcontrolador.

El aprendizaje más relevante del trabajo no estuvo en la funcionalidad sino en el **tiempo real**. La primera versión del firmware funcionaba correctamente desde el punto de vista del usuario, pero violaba sistemáticamente el presupuesto temporal: el `HAL_Delay(5)` de la EEPROM bloqueaba el micro cinco veces el período completo del ejecutivo, y la escritura del LCD lo hacía por varios milisegundos más. Que un sistema "ande" no implica que sea determinista. Reescribirlo como Event-Triggered System obligó a atacar cada fuente de bloqueo por separado (framebuffer para el display, cola diferida para la EEPROM, conversión en dos ticks para el ADC, árbitro de token para el bus compartido) y solo entonces el factor de uso se volvió una magnitud medible y acotada.

## 5.2 Próximos pasos


1.   **Endurecer el bus I²C:** actualmente, si un esclavo deja de responder, el HAL sale por timeout pero no hay rutina de recuperación del bus.
2. **Ensamble definitivo:** dentro de un gabinete impreso en 3D para evaluar la disposición de los sensores frente a variables físicas reales.

3. **Cifrado de datos en la memoria EEPROM:** Implementar algoritmos de encriptación (como AES de bajo consumo o hashing para la clave) para asegurar que los registros de auditoría y la contraseña no puedan ser leídos directamente si se extrae físicamente el chip de memoria.
4. **Integración de conectividad inalámbrica:** Incorporar un módulo de comunicación (como un ESP32 o módulo Bluetooth) para permitir la apertura remota mediante una aplicación móvil y la descarga inalámbrica del historial de accesos guardado en la EEPROM.

---


# Capítulo 6: Uso de herramientas de IA

Durante el desarrollo del proyecto se utilizaron herramientas de Inteligencia Artificial para:

* **Redacción y estructuración de informes:** Organizar el esqueleto de esta memoria técnica, mejorar la fluidez de la redacción y resumir las ideas principales del documento.
* **Documentación de código:** Agilizar la creación de comentarios claros y estructurados dentro del firmware del sistema.
* **Resolución de problemas y depuración:** Consultar de forma rápida la interpretación de errores de compilación y analizar comportamientos inesperados del microcontrolador.
* **Selección y recomendación de componentes:** Validar especificaciones de periféricos, comparar alternativas de hardware y verificar compatibilidades de conexión.

Esto nos permitió resolver de forma ágil las dudas del día a día, permitiéndonos concentrar el esfuerzo en la lógica del sistema y la integración del prototipo físico.

---

# Capítulo 7: Bibliografía y referencias

[1] STMicroelectronics. *STM32F103RB Datasheet*.

[2] STMicroelectronics. *RM0008 – STM32F10xxx Reference Manual*.

[3] STMicroelectronics. *UM1724 – STM32 Nucleo-64 boards user manual*.

[4] Maxim Integrated. *DS3231 Extremely Accurate I²C-Integrated RTC/TCXO/Crystal Datasheet*.

[5] Microchip/Atmel. *AT24C32 EEPROM Datasheet*.

[6] Texas Instruments. *PCF8574 Remote 8-Bit I/O Expander Datasheet*.

[7] TowerPro. *SG90 Micro Servo Datasheet*.

[8] Repositorio de entregas: `https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02`
