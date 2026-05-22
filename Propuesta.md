Propuesta de Trabajo Final: Cerradura Electrónica de Alta Seguridad

| **Apellido, Nombre**  | **Padrón** |
| --------------------- | ---------- |
| Sanchez Q., Matias J. | 111060     |
| Laskowski, Marcos     | 104028     |
| Fanzi, Francisco      | 107510     |

**Año-Cuatrimestre:** 2026-1erC
**Curso-Grupo:** 2-02

---

## 1. Selección del proyecto a implementar

### 1.1 Objetivo del proyecto y resultados esperados
El objetivo de este proyecto es diseñar e implementar un prototipo funcional de una cerradura electrónica de alta seguridad para una caja fuerte utilizando la placa NUCLEO-F103RB. El sistema permitirá el ingreso de una combinación numérica dígito a dígito mediante un dial analógico posicional (potenciómetro lineal mapeado de 0 a 9), validando la entrada contra una clave maestra almacenada en una memoria no volátil EEPROM.

Para cumplir con criterios estrictos de eficiencia energética, el sistema implementará un modo de operación de bajo consumo (`REPOSO`), donde se mantendrán apagados el display y la etapa de lectura del dial, quedando activos únicamente los sensores críticos de seguridad perimetral (sensor magnético de puerta y sensor lumínico LDR interno anti-sabotaje). El sistema integrará un Reloj de Tiempo Real (RTC) para colocar temporalmente los eventos de acceso y alarmas, permitiendo auditar el historial localmente. También se supervisará el estado de la batería de respaldo.


### 1.2 Proyectos similares
En base al objetivo se proponen posibles proyectos que requieren el uso de hardware específico y se ponderan los aspectos a tener en cuenta para seleccionar el más adecuado.

* **Cerradura Básica:** Control mediante teclado matricial digital (keypad) y pantalla, sin gestión avanzada de energía ni registro histórico.
* **Cerradura Avanzada (Nuestro Proyecto):** Control mediante dial analógico, monitoreo dual de seguridad (puerta + luz interna), gestión de bajo consumo, auditoría de batería y registro histórico en memoria EEPROM con RTC.
* **Cerradura IoT (Wi-Fi/RFID):** Control de acceso mediante tarjetas RFID y conexión a red Wi-Fi para registro en la nube.

Para comparar estas alternativas, se tienen en cuenta cinco aspectos característicos, los cuales se ponderan del 1 al 10:

1. **Disponibilidad del hardware (9):** Se evalúa si el proyecto puede implementarse utilizando hardware accesible en el mercado regional (Argentina).
2. **Seguridad y Robustez (10):** Capacidad del sistema para detectar sabotajes y operar offline.
3. **Costo (7):** Se evalúa el costo total del hardware requerido.
4. **Dificultad técnica (8):** Viabilidad técnica y cumplimiento de las exigencias de la materia (uso de ADC, I2C, DMA, máquinas de estado).
5. **Interés personal del equipo (8):** Motivación e interés en implementar la solución.

La siguiente tabla (Tabla 1.2.1) muestra los valores ponderados asignados a cada proyecto considerado:

| Criterio | Cerradura Básica | Cerradura Avanzada (Dial+RTC) | Cerradura IoT (Wi-Fi/RFID) |
| :--- | :---: | :---: | :---: |
| | **Puntaje / Ponderado** | **Puntaje / Ponderado** | **Puntaje / Ponderado** |
| **Disponibilidad de HW (peso: 9)** | 10 / 90 | 9 / 81 | 6 / 54 |
| **Seguridad y Robustez (peso: 10)** | 5 / 50 | 9 / 90 | 7 / 70 |
| **Costo (peso: 7)** | 9 / 63 | 7 / 49 | 4 / 28 |
| **Dificultad técnica (peso: 8)** | 4 / 32 | 9 / 72 | 8 / 64 |
| **Interés personal (peso: 8)** | 5 / 40 | 9 / 72 | 8 / 64 |
| **Puntaje Total** | **275** | **364** | **280** |
*Tabla 1.2.1: Comparación de proyectos.*


### 1.3 Selección de proyecto
Vistas las consideraciones tomadas en cuenta para discriminar cuál sería el proyecto óptimo para desarrollar, se decidió seguir adelante con la opción **Cerradura Avanzada (Dial+RTC)**. Esta opción supera a la cerradura básica ya que justifica orgánicamente el uso del conversor analógico-digital (ADC) para el dial, el LDR y la batería, y emplea el bus I2C para periféricos avanzados (RTC y EEPROM), cumpliendo de lleno con los requisitos de la materia. Por otro lado, evita la dependencia de conectividad Wi-Fi, lo cual podría presentar vulnerabilidades o complejidades fuera del alcance *bare-metal* estricto requerido.

Los principales desafíos que se van a afrontar son la implementación de la lectura por DMA, la estructuración correcta de los modos de bajo consumo sin perder eventos del sensor magnético, y la comunicación I2C no bloqueante en una arquitectura Super-Loop (Tick = 1ms).

---

## 2. Elicitación de requisitos y casos de uso

Nuestro proyecto se posiciona como una propuesta robusta e industrializable al integrar sensores analógicos y digitales, un dial posicional preciso, alarmas anti-sabotaje y la persistencia de un historial de aperturas real. Para cumplir con las pautas de la entrega, se detalla a continuación la arquitectura de hardware y software seleccionada:

**Hardware Obligatorio y Adicional:**
* **Interconexión:** Placa experimental con componentes soldados. Sin uso de protoboard ni cables Dupont.
* **Sensores Analógicos:** Potenciómetro lineal (dial numérico), sensor de luminosidad LDR (detección interna de intrusión/sabotaje) y un divisor resistivo conectado al ADC para monitorear el estado de la batería de respaldo.
* **Sensores Digitales:** Sensor magnético de apertura para monitoreo de la puerta.
* **Módulos de Memoria y Tiempo:** Módulo I2C que integra un reloj de tiempo real y una memoria EEPROM.
* **Interfaz Local:** Pantalla LCD-1602BLUE-I2C de 2x16 con adaptador PCF8574. Diodos LED indicadores (Verde para acceso, Rojo para alarma, Amarillo para batería baja) y Buzzer piezoeléctrico.
* **Entradas Digitales:** Pulsadores para funciones de Confirmar/Despertar e Historial. Bloque Dip Switch de 4 posiciones (DSW-04) para selección del modo administrador.

**Programación Obligatoria & Adicional:**
* **Arquitectura de Firmware:** Diseño Bare Metal orientado a eventos (*Event-Triggered System*) estructurado bajo el patrón de diseño clásico *Super-Loop*. 
* **Temporización Determinística:** Control temporal mediante el timer *SysTick* para proveer un latido constante de `Tick = 1ms`. El tiempo de cómputo del lazo cíclico principal respetará en todo momento la restricción de `1 vuelta < 1ms`.
* **Diseño Formal:** Flujo lógico gobernado por diagramas de máquinas de estado automatizadas mediante *itemisCREATE*, contando con al menos 3 modos: NORMAL (Reposo/Ingreso), SET_UP y ALARMA.
* **Manejo de Periféricos:** Lecturas analógicas continuas empleando el periférico ADC mapeado directamente con canales **DMA**. Gestión del bus I2C mediante interrupciones o polling no bloqueante.

| Grupo                    | ID       | Descripción                                                                                                                                             |
| :----------------------- | :------- | :------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Gestión de Energía**   | **RQ01** | El sistema debe operar en bajo consumo (REPOSO), activando el LCD y el escrutinio del potenciómetro solo tras una interrupción de usuario.              |
| **Auditoría de Sistema** | **RQ02** | El sistema debe medir continuamente la tensión de la batería por ADC e informar mediante el LED amarillo y un mensaje si el nivel es bajo.              |
| **Seguridad y Acceso**   | **RQ03** | El sistema debe validar el ingreso de la contraseña actual antes de habilitar la sobrescritura de una nueva combinación en el modo SET_UP.              |
|                          | **RQ04** | En modo REPOSO, la apertura no validada de la puerta (MC-38) o la detección de luz interna (LDR) debe activar la alarma visual y sonora.                |
| **Almacenamiento**       | **RQ05** | El sistema debe registrar en la EEPROM la estampa de tiempo (RTC) de las aperturas exitosas y mostrar el último evento al pulsar el botón de Historial. |
*Tabla 2.1: Requisitos del proyecto.*

En las tablas 2.2 a 2.4 se presentan 3 casos de uso para el sistema.

| Elemento | Definición |
| :--- | :--- |
| **Disparador** | El usuario desea ingresar su combinación para abrir la caja fuerte. |
| **Precondiciones** | El sistema se encuentra en modo REPOSO (Bajo consumo). La puerta está cerrada y no hay alarmas activas. |
| **Flujo principal** | El usuario presiona el botón de despertar. El LCD se enciende, el sistema lee el nivel de batería por ADC (advierte si es bajo) y activa el potenciómetro. El usuario ingresa la secuencia confirmando dígito a dígito. El sistema valida contra la EEPROM; al ser correcta, abre la cerradura, registra la hora del RTC en memoria y, tras unos segundos de inactividad, retorna al modo REPOSO. |
| **Flujos alternativos** | a. El usuario ingresa una clave incorrecta: El sistema enciende el LED rojo, alerta con el Buzzer, limpia el ingreso y retorna a REPOSO por inactividad. |
*Tabla 2.2: Caso de uso 1 - Despertar, Ingreso de Combinación y Audición de Batería.*

| Elemento | Definición |
| :--- | :--- |
| **Disparador** | El usuario solicita ver la información de la última apertura. |
| **Precondiciones** | La interfaz está "despierta". El módulo RTC y la EEPROM están comunicados correctamente por I2C. |
| **Flujo principal** | El usuario presiona el botón "Historial". El microcontrolador solicita por bus I2C la última estampa de tiempo guardada en la EEPROM y la imprime en el LCD (ej. "Últ. Ap: DD/MM HH:MM") durante 5 segundos antes de volver automáticamente a la pantalla de ingreso normal. |
| **Flujos alternativos** | a. Falla de lectura I2C: El sistema muestra el mensaje estático "Error de Memoria" temporalmente y vuelve al menú de ingreso sin bloquearse. |
*Tabla 2.3: Caso de uso 2 - Auditoría del Historial de Aperturas.*

| Elemento                | Definición                                                                                                                                                                                                                                                                                                                                                          |
| :---------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Disparador**          | El administrador necesita cambiar la clave maestra del sistema.                                                                                                                                                                                                                                                                                                     |
| **Precondiciones**      | La caja fuerte está abierta y el administrador tiene acceso al hardware interno para accionar el Dip Switch.                                                                                                                                                                                                                                                        |
| **Flujo principal**     | El usuario acciona el Dip Switch físico pasando al estado SET_UP. El sistema solicita primero ingresar la clave actual. Si es correcta, el LCD pide la "NUEVA CLAVE". El usuario la ingresa con el dial analógico y, al confirmar, el microcontrolador sobreescribe la EEPROM. Al regresar el Dip Switch a su posición original, el sistema retorna al modo normal. |
| **Flujos alternativos** | a. Se ingresa mal la clave actual: El sistema deniega el acceso al cambio de clave y muestra un error en pantalla.<br>b. El usuario vuelve el Dip Switch a la posición normal antes de terminar: Se aborta la operación y se mantiene la clave original.                                                                                                            |
*Tabla 2.4: Caso de uso 3 - Cambio de Clave de Seguridad Validado (SET_UP).*