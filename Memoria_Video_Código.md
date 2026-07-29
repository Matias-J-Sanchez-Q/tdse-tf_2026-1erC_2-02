<div align="center">
<img width="300" alt="FIUBA" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/logo-fiuba.png" />

**TA134 – Taller de Sistemas Embebidos**

# Memoria del Trabajo Final: Cerradura Electrónica de Alta Seguridad

Sistema de control de acceso multifunción con registro histórico y protección activa.

## Autores

| **Apellido, Nombre**  | **Padrón** |
| --------------------- | ---------- |
| Sanchez Q., Matias J. | 111060     |
| Laskowski, Marcos     | 104028     |
| Fanzi, Francisco      | 107510     |


**Docente**: Cruz, Juan Manuel<br>
**Tutor:** Lutenberg, Ariel<br>
**Fecha:** Julio de 2026<br>
**Cuatrimestre de cursada:** 1er cuatrimestre 2026<br>
**Curso-Grupo:** 2-02

*Trabajo realizado en la Ciudad Autónoma de Buenos Aires.*


---
</div>


## Resumen

En este trabajo detallamos el diseño, desarrollo e implementación de una cerradura electrónica de alta seguridad basada en la plataforma NUCLEO-F103RB. A diferencia de los sistemas tradicionales con teclado, este dispositivo utiliza un potenciómetro para ingresar la clave de forma analógica, emulando el dial de una caja fuerte tradicional, y emplea una pantalla LCD para guiar al usuario hasta que un servomotor destraba la puerta tras la validación correcta. Para garantizar la trazabilidad y seguridad del recinto, el dispositivo guarda en una memoria externa no volátil la fecha y hora exacta de los últimos 10 intentos de apertura. Adicionalmente, cuenta con un sensor magnético de puerta y un sensor de luz (LDR) que disparan una alarma sonora si el gabinete es vulnerado. 

Desde la perspectiva del software, se desarrolló un sistema eficiente y de bajo consumo enfocado en la autonomía. El procesador funciona de forma no bloqueante y, cada vez que termina de procesar los eventos del usuario, entra automáticamente en un modo de reposo (*sleep*). Esta arquitectura guiada por eventos asegura que el microcontrolador solo consuma energía durante las fracciones de segundo en las que realmente está operando, maximizando la vida útil de la batería.

---


## Registro de versiones

| Revisión | Cambios realizados | Fecha |
| :---: | --- | :---: |
| 1.0 | Creación del esqueleto y estructura base del documento. | 11/07/2026 |
| 1.1 | Redacción detallada, desarrollo y completado de las secciones del informe. | 15/07/2026 |
| 1.2 | Correcciones según devolución de primer entrega| 29/07/2026 |

<em>Tabla 0.1 — Registro de versiones del documento.</em><br><br>

---

# Índice General

- [Capítulo 1: Introducción general](#capítulo-1-introducción-general)
  - [1.1 Análisis de necesidad](#11-análisis-de-necesidad)
  - [1.2 Objetivos](#12-objetivos)
  - [1.3 Productos comerciales disponibles](#13-productos-comerciales-disponibles)
  - [1.4 Comparación con el prototipo desarrollado](#14-comparación-con-el-prototipo-desarrollado)
  - [1.5 Alcance del prototipo](#15-alcance-del-prototipo)
- [Capítulo 2: Introducción específica](#capítulo-2-introducción-específica)
  - [2.1 Requisitos del sistema](#21-requisitos-del-sistema)
  - [2.2 Casos de uso](#22-casos-de-uso)
  - [2.3 Descripción de módulos y tecnologías utilizadas](#23-descripción-de-módulos-y-tecnologías-utilizadas)
    - [2.3.1 Potenciómetro como dial analógico](#231-potenciómetro-como-dial-analógico)
    - [2.3.2 Divisor con fotorresistencia (LDR)](#232-divisor-con-fotorresistencia-ldr)
    - [2.3.3 Reloj de tiempo real y memoria EEPROM](#233-reloj-de-tiempo-real-y-memoria-eeprom)
    - [2.3.4 Display LCD](#234-display-lcd)
    - [2.3.5 Actuador de traba](#235-actuador-de-traba)
    - [2.3.6 Sensores digitales](#236-sensores-digitales)
- [Capítulo 3: Diseño e implementación](#capítulo-3-diseño-e-implementación)
  - [3.1 Hardware del sistema](#31-hardware-del-sistema)
  - [3.2 Diseño del firmware](#32-diseño-del-firmware)
    - [3.2.1 Arquitectura: ejecutivo cíclico y Event-Triggered System](#321-arquitectura-ejecutivo-cíclico-y-event-triggered-system)
    - [3.2.2 Reloj del sistema](#322-reloj-del-sistema)
    - [3.2.3 Máquinas de estado](#323-máquinas-de-estado)
- [Capítulo 4: Ensayos y resultados](#capítulo-4-ensayos-y-resultados)
  - [4.1 Prueba de integración (Video)](#41-prueba-de-integración-video)
  - [4.2 Pruebas funcionales de hardware y firmware](#42-pruebas-funcionales-de-hardware-y-firmware)
  - [4.3 Ocupación de memoria: Console & Build Analyzer](#43-ocupación-de-memoria-console--build-analyzer)
  - [4.4 Medición y análisis de tiempos de ejecución (WCET)](#44-medición-y-análisis-de-tiempos-de-ejecución-wcet)
    - [4.4.1 Metodología](#441-metodología)
    - [4.4.2 Resultados](#442-resultados)
  - [4.5 Cálculo del Factor de Uso (U) de la CPU](#45-cálculo-del-factor-de-uso-u-de-la-cpu)
  - [4.6 Medición y análisis de consumo](#46-medición-y-análisis-de-consumo)
  - [4.7 Cumplimiento de requisitos](#47-cumplimiento-de-requisitos)
- [Capítulo 5: Conclusiones](#capítulo-5-conclusiones)
  - [5.1 Resultados obtenidos](#51-resultados-obtenidos)
  - [5.2 Próximos pasos](#52-próximos-pasos)
- [Capítulo 6: Uso de herramientas de IA](#capítulo-6-uso-de-herramientas-de-ia)
- [Capítulo 7: Bibliografía y referencias](#capítulo-7-bibliografía-y-referencias)
---

# Capítulo 1: Introducción general
 
## 1.1 Análisis de necesidad
 
 
Los sistemas de guarda de valores (cajas fuertes de uso doméstico, hotelero y de oficina) enfrentan un doble desafío: bloquear eficazmente el acceso a personas no autorizadas y mantener un registro auditable de toda actividad sobre el gabinete. Al analizar la oferta comercial, se observa que ninguna de las gamas tradicionales atiende este segundo punto de manera integral. Por un lado, los productos de gama de entrada resuelven bien la barrera básica mediante cerraduras electrónicas simples, pero carecen por completo de registro histórico. Por otro lado, los equipos de gama alta o industriales apuestan por blindajes extremos y diales mecánicos tradicionales que, si bien son sumamente robustos frente a ataques físicos, impiden la reprogramación ágil de las claves y no ofrecen trazabilidad digital.
 
En un uso compartido (hotel, oficina, consorcio, caja chica de un comercio), esta ausencia generalizada de auditoría implica que ante un faltante no existe evidencia de cuándo se abrió el gabinete, cuántos intentos fallidos hubo ni si la puerta quedó abierta fuera de horario. La necesidad detectada, entonces, no es diseñar "una cerradura más", sino sumar auditoría persistente y monitoreo activo del entorno físico a un control de acceso autónomo, alimentado a batería, combinando la seguridad antidesgaste de un dial con las ventajas de trazabilidad de los sistemas digitales modernos.
 
## 1.2 Objetivos
 
Se plantea la construcción de un prototipo de control de acceso autónomo y de bajo consumo, con los siguientes objetivos específicos:
 
1. Autenticación local de clave de 4 dígitos sin teclado matricial ni de membrana, utilizando un dial analógico (potenciómetro sobre ADC).
2. Almacenamiento persistente de la clave y de un *log* en anillo de los últimos 10 intentos de acceso, con estampa temporal.
3. Vigilancia activa del entorno físico del gabinete (estado de apertura de puerta y nivel de tensión de alimentación).
4. Gestión eficiente del consumo, suspendiendo la interfaz visual y el CPU tras períodos de inactividad.

## 1.3 Productos comerciales disponibles

Para dimensionar el trabajo y contextualizar la solución desarrollada, se relevó la oferta disponible en el mercado argentino, identificando dos grandes familias de productos: las cerraduras digitales económicas de teclado y las cajas de seguridad de gama alta con dial de combinación.

**1. Producto de gama de entrada: [Caja fuerte digital y llave]** [https://www.mercadolibre.com.ar/caja-fuerte-caja-digital-y-llave-global-23x17x17cm-negra-oficina-hogar/p/MLA34953591](https://www.mercadolibre.com.ar/caja-fuerte-caja-digital-y-llave-global-23x17x17cm-negra-oficina-hogar/p/MLA34953591)


<div align="center">
<img width="400" alt="Caja fuerte digital GLOBAL" src=https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/Matias-J-Sanchez-Q-patch-1/Imagenes/caja_fuerte_teclado.png />
<p><em>Figura 1.1: Caja de seguridad comercial de gama de entrada con teclado de membrana.</em></p>
</div>

* Características: Gabinete de acero compacto de 3 kg con teclado electrónico de membrana (código programable de 3 a 8 dígitos) y cerradura mecánica de emergencia. Cuenta con cierre por doble pasador motorizado, bloqueo temporal tras tres intentos fallidos e indicadores LED simples (abierto, batería baja, bloqueado).
* Costo aproximado: ~$85.000 ARS ($57 USD).

**2. Producto de gama alta / industrial: [Caja fuerte con dial SentrySafe]** [https://www.mercadolibre.com.ar/caja-fuerte-sentrysafe-a-prueba-de-fuego-y-agua-082-pies-negro/p/MLA63399897?matt_tool=38087446&utm_source=google_shopping&utm_medium=organic&pdp_filters=item_id%3AMLA1654557953&from=gshop](https://www.mercadolibre.com.ar/caja-fuerte-sentrysafe-a-prueba-de-fuego-y-agua-082-pies-negro/p/MLA63399897?matt_tool=38087446&utm_source=google_shopping&utm_medium=organic&pdp_filters=item_id%3AMLA1654557953&from=gshop)


<div align="center">
<img width="400" alt="Caja fuerte digital GLOBAL" src=https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/Matias-J-Sanchez-Q-patch-1/Imagenes/caja_fuerte_dial.png />
<p><em>Figura 1.2: Caja de seguridad con dial de gama alta.</em></p>
</div>


* Características: Gabinete pesado de alta seguridad con protección ignífuga y hermética. Utiliza una interfaz de acceso mediante dial mecánico de combinación graduada de 3 a 4 posiciones combinado con llave de seguridad. Presenta una alta resistencia a ataques físicos y forzado mecánico.
* Costo aproximado: ~1.300.000 ARS ($870 USD).

**Análisis de brecha tecnológica:**
Ninguno de los dos productos comerciales relevados (Figura 1.1 y 1.2) ofrece un sistema de auditoría ni trazabilidad. En el modelo económico, el bloqueo por clave incorrecta es volátil y no genera registros. En el modelo SentrySafe, el dial es puramente mecánico (o de combinación prefijada sin electrónica), lo que impide la reprogramación ágil de claves, la gestión de menús en pantalla o la integración de registradores temporales (*timestamps*). Asimismo, ninguno de los productos comerciales monitorea de forma activa la apertura de puerta ni la intrusión lumínica en el gabinete una vez cerrado.

## 1.4 Comparación con el prototipo desarrollado

La Tabla 1.1 contrasta las prestaciones de los dos productos comerciales de referencia contra el prototipo desarrollado en este trabajo.

| Aspecto | Producto económico  | Producto gama alta (SentrySafe Dial) | Prototipo desarrollado |
| :--- | :--- | :--- | :--- |
| **Ingreso de clave** | Teclado de membrana (3-8 dígitos) | Dial mecánico graduado (combinación fija) | Dial analógico (potenciómetro + ADC), 4 dígitos |
| **Interfaz de usuario** | LEDs de estado simples | Dial mecánico graduado (sin pantalla) | LCD 16×2 con menú interactivo de dos niveles |
| **Persistencia / Clave** | Memoria interna del módulo | Combinación mecánica por discos de levas | EEPROM externa AT24C32 (I2C) con byte de firma |
| **Registro de eventos** | No disponible | No disponible | *Log* circular de 10 intentos en EEPROM |
| **Base de tiempo** | No posee | No posee | RTC DS3231 (I2C), estampa temporal por evento |
| **Monitoreo de puerta** | No | No | Sensor magnético con lógica de armado/desarmado |
| **Seguridad anti-sabotaje** | Bloqueo temporal de teclado | Barrera física / Ignífuga | Alarma sonora/visual + Sensor LDR interno |
| **Gestión de consumo** | Reposo básico del controlador | N/A (Sistema puramente mecánico) | Modo reposo (*Sleep*) con apagado de display |
| **Actuador** | Doble pasador motorizado | Pestillos de acero con traba de dial | Servomotor SG90 sobre maqueta |
| **Barrera física** | Acero (3 kg) | Acero blindado ignífugo/hermético | No implementada (fuera de alcance) |
| **Respaldo mecánico** | Dos llaves de emergencia | Llave de bloqueo tubular | No implementado |
| **Costo y disponibilidad** | ~$85.000 ARS, stock comercial | ~$1.300.000 ARS, stock comercial | Prototipo de laboratorio |

<p align="center"><em>Tabla 1.1: Comparación de prestaciones entre productos comerciales y el prototipo desarrollado.</em></p>

De la Tabla 1.1 se desprenden dos conclusiones principales:

1. **Aporte funcional del prototipo:** El diseño desarrollado resuelve falencias presentes en ambas gamas de mercado. Frente al producto económico, reemplaza el teclado de membrana por un dial analógico (evitando el desgaste diferencial de teclas que expone la clave a observadores) y agrega trazabilidad de accesos con fecha y hora. Frente al producto de gama alta con dial mecánico, el prototipo digitaliza la experiencia: permite cambiar la combinación de forma electrónica desde un menú en display, gestionar modos de bajo consumo y reaccionar mediante alarmas activas ante intrusiones lumínicas o aperturas no autorizadas.
2. **Límites de alcance e ingeniería física:** Las soluciones comerciales resuelven la protección estructural (gabinete de acero, resistencia al fuego, pasadores de alta resistencia mecánica y cerraduras mecánicas de respaldo) que este proyecto dejó deliberadamente fuera de alcance. El prototipo no busca competir como un producto final terminado, sino como la demostración funcional de un módulo electrónico de control, interfaz y auditoría no disponible en el mercado actual.

## 1.5 Alcance del prototipo

El trabajo abarca exclusivamente la electrónica de control, la interfaz de usuario y la lógica de registro e interrupciones, implementadas sobre una placa NUCLEO-F103RB y montadas en una maqueta experimental. Quedan explícitamente fuera de alcance el diseño mecánico del gabinete blindado, el dimensionamiento del actuador de cierre definitivo de alto torque y la conectividad remota para descarga del *log*.

---

# Capítulo 2: Introducción específica

En esta sección se encuentran los requisitos originales y los modificados, además de los casos de uso.

## 2.1 Requisitos del sistema

A lo largo del ciclo de vida del proyecto, los requisitos funcionales experimentaron una evolución. Inicialmente, durante la etapa de planificacion, se plantearon las funcionalidades base que debía cumplir el prototipo para garantizar un control de acceso seguro y de bajo consumo. La Tabla 2.1 detalla esta primera aproximación de los requisitos.

| Grupo | ID | Descripción |
| :--- | :---: | :--- |
| **Gestión de Energía** | **RQ01** | El sistema debe operar en bajo consumo (REPOSO), activando el LCD y el escrutinio del potenciómetro solo tras una interrupción de usuario. |
| **Auditoría de Sistema** | **RQ02** | El sistema debe medir continuamente la tensión de la batería por ADC e informar mediante el LED amarillo y un mensaje si el nivel es bajo. |
| **Seguridad y Acceso** | **RQ03** | El sistema debe validar el ingreso de la contraseña actual antes de habilitar la sobrescritura de una nueva combinación en el modo SET_UP. |
| | **RQ04** | En modo REPOSO, la apertura no validada de la puerta (MC-38) o la detección de luz interna (LDR) debe activar la alarma visual y sonora. |
| **Almacenamiento** | **RQ05** | El sistema debe registrar en la EEPROM la estampa de tiempo (RTC) de las aperturas exitosas y mostrar el último evento al pulsar el botón de Historial. |
| **Seguridad y Acceso** | **RQ06** | La contraseña de acceso estará formada por 4 dígitos decimales, cada uno comprendido entre 0 y 9. |

<p align="center"><em>Tabla 2.1: Requisitos funcionales preliminares (Fase de ideación).</em></p>


Posteriormente, conforme avanzó el desarrollo del firmware y la selección de la arquitectura de hardware, estos requerimientos iniciales se desglosaron y refinaron para abarcar todos los aspectos operativos de la cerradura electrónica. La Tabla 2.2 presenta la versión definitiva de los requisitos del sistema implementados en la entrega final.

| Grupo | ID | Descripción |
| :--- | :---: | :--- |
| **Gestión de Energía** | **RQ01** | El sistema debe operar en bajo consumo (REPOSO), suspendiendo el LCD y el CPU tras 15 s de inactividad, y despertando ante actividad del usuario. |
| **Interfaz de Usuario** | **RQ02** | El sistema debe proveer retroalimentación visual mostrando el estado del menú y los eventos en una pantalla LCD operada por bus I²C. |
| **Seguridad y Acceso** | **RQ03** | La contraseña estará formada por 4 dígitos numéricos (0 a 9), ingresados y confirmados individualmente. |
| | **RQ04** | El sistema debe permitir la selección de dígitos mapeando la tensión de un potenciómetro lineal. |
| | **RQ05** | El sistema debe habilitar el modo de configuración (SET_UP) para el cambio de contraseña únicamente tras una apertura exitosa. |
| **Actuación** | **RQ06** | Tras la validación exitosa, el sistema debe accionar un servomotor mediante PWM para liberar la traba física. |
| **Seguridad y Alarmas** | **RQ07** | En modo armado, la apertura no validada de la puerta o la detección de luz interna (LDR) debe activar una alarma visual y sonora. |
| | **RQ08** | El sistema debe advertir mediante señales visuales y sonoras el ingreso de una combinación incorrecta. |
| **Almacenamiento** | **RQ09** | El sistema debe registrar en la EEPROM la estampa de tiempo provista por el RTC tras cada intento de acceso. |
| | **RQ10** | El sistema debe recuperar de memoria y mostrar en el LCD el registro histórico de intentos. |

<p align="center"><em>Tabla 2.2: Evolución final de los requisitos funcionales del sistema.</em></p>

Más adelante, se detalla si se cumplieron o no los requisitos, y se da la razón en caso de no haberse implementado.

## 2.2 Casos de uso
 
Se identificaron tres casos de uso principales, detallados en las Tablas 2.2 a 2.4. Cada uno se corresponde con uno de los modos de operación descritos más adelante en la sección 3.2.3.
 
La Tabla 2.2 describe el caso de uso más frecuente: la apertura del gabinete mediante el ingreso de la combinación.
 
| Elemento | Definición |
| :--- | :--- |
| Disparador | El usuario desea ingresar su combinación para abrir la caja fuerte. |
| Precondiciones | El sistema está en modo verificación, la puerta cerrada y sin alarmas activas. |
| Flujo principal | El usuario ingresa la secuencia de 4 dígitos con el dial, confirmando cada uno con el pulsador. El sistema valida contra la clave persistida en EEPROM; si es correcta, libera el servomotor, desactiva la alarma y registra el intento con la estampa del RTC. |
| Flujo alternativo | Clave incorrecta: el sistema pulsa la alarma (LED rojo + buzzer) durante 2 s, mantiene la traba puesta y registra el intento fallido. |
| Flujo alternativo | Clave correcta pero puerta nunca abierta: transcurridos 10 s, el cerrojo se vuelve a trabar y el sistema se re-activa automáticamente. |
 
<p align="center"><em>Tabla 2.2: Caso de uso 1 — ingreso de combinación.</em></p>

La Tabla 2.3 describe la consulta del registro de auditoría, funcionalidad que da sentido a la persistencia en EEPROM y que constituye la principal diferencia frente a los productos comerciales relevados en la Tabla 1.1.
 
| Elemento | Definición |
| :--- | :--- |
| Disparador | El usuario quiere ver los últimos intentos de acceso. |
| Precondiciones | El sistema debe estar en modo MENÚ. |
| Flujo principal | El usuario navega al submenú de historial con el dial. El sistema recupera los últimos 10 intentos (fecha, hora, dígitos ingresados y resultado) y los muestra en el LCD, navegables con el potenciómetro. |
| Flujo alternativo | Inactividad: tras 15 s sin interacción, el sistema suspende el display y entra en REPOSO. |
 
<p align="center"><em>Tabla 2.3: Caso de uso 2 — consulta del historial de aperturas.</em></p>

Por último, la Tabla 2.4 detalla el cambio de la clave maestra, operación habilitada únicamente tras una apertura exitosa según el requisito RQ05 de la Tabla 2.1.
 
| Elemento | Definición |
| :--- | :--- |
| Disparador | El usuario necesita cambiar la clave maestra. |
| Precondiciones | El sistema debe estar en modo CAMBIO DE CLAVE. |
| Flujo principal | El usuario ingresa los 4 dígitos nuevos con el dial. Al confirmar el cuarto, la clave se encola para su escritura en EEPROM y el sistema muestra confirmación en pantalla. |
| Flujo alternativo | Si la cola de escritura de la EEPROM está saturada, el cambio se rechaza y la clave anterior se mantiene intacta, evitando un estado inconsistente entre RAM y memoria no volátil. |
 
<p align="center"><em>Tabla 2.4: Caso de uso 3 — cambio de clave de seguridad.</em></p>



## 2.3 Descripción de módulos y tecnologías utilizadas

A continuación se describen los principales componentes de hardware, módulos periféricos y actuadores seleccionados para la implementación física del prototipo. Cada elemento fue elegido con el propósito de satisfacer los requisitos funcionales de seguridad, interfaz de usuario y eficiencia energética detallados previamente, operando de manera coordinada bajo el control de la placa principal NUCLEO-F103RB.

### 2.3.1 Potenciómetro como dial analógico

<div align="center">
<img width="500" alt="Potenciómetro lineal 10k" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/PM-103LI.JPG" />
<p><em>Figura 2.1: Potenciómetro lineal utilizado como dial analógico.</em></p>
</div>

Potenciómetro lineal de 10 kΩ. El valor de 12 bits se filtra y se mapea linealmente al rango de dígitos 0 a 9. El mismo canal se reutiliza para navegar las opciones del menú y para detectar actividad del usuario.

### 2.3.2 Divisor con fotorresistencia (LDR)

<div align="center">
<img width="500" alt="Fotorresistencia LDR-05" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/LDR-05.JPG" />
<p><em>Figura 2.2: Fotorresistencia LDR-05 para detección de intrusión lumínica.</em></p>
</div>

LDR-05 en divisor resistivo con una resistencia fija de 10 kΩ. Al abrirse el gabinete, la luz incidente modifica la tensión del divisor, el firmware compara esa tensión contra un umbral configurable.

### 2.3.3 Reloj de tiempo real y memoria EEPROM

<div align="center">
<img width="500" alt="Módulo RTC DS3231 y EEPROM AT24C32" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/CLON-REAL.TIME.JPG" />
<p><em>Figura 2.3: Módulo RTC DS3231 con memoria EEPROM integrada.</em></p>
</div>

Módulo combinado DS3231 (RTC) con EEPROM AT24C32 integrada y respaldo por batería CR2032, comunicado por I²C1.

### 2.3.4 Display LCD

<div align="center">
<img width="500" alt="Display LCD 16x2" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/LCD-1602BLUE-I2C24741AA013.jpg" />
<p><em>Figura 2.4: Display LCD 16x2 con adaptador de bus I²C.</em></p>
</div>

LCD 16x2 con *backpack* I²C basado en PCF8574, sobre el mismo bus I²C1.

### 2.3.5 Actuador de traba

<div align="center">
<img width="500" alt="Servomotor SG90" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/servomotor.png" />
<p><em>Figura 2.5: Micro servomotor SG90 para el accionamiento de la traba.</em></p>
</div>

Servomotor SG90 controlado por PWM, alimentado desde una fuente externa de 5 V independiente de la NUCLEO, con GND compartido.

### 2.3.6 Sensores digitales

<div align="center">
<!-- Nota de edición: El link proporcionado para el sensor magnético coincide con la imagen del potenciómetro. Se aplicó el link exactamente como fue provisto. -->
<img width="500" alt="Sensores digitales" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/SENSOR_MAGNETICO.jpg" />
<p><em>Figura 2.6: Entradas digitales del sistema.</em></p>
</div>

Sensor magnético (MC-38) para detección de apertura de puerta (PA1) y *boton analogico* (TS4-5) para la confirmación de dígitos (PB0). Ambos con pull-up interno y antirrebote por software.


---

# Capítulo 3: Diseño e implementación
 
En este capítulo se detalla el proceso de diseño y la construcción del prototipo, abordando de manera integral tanto la arquitectura física como la estructura lógica del sistema.
 
## 3.1 Hardware del sistema
 
Para entender la arquitectura física de nuestra cerradura partimos del diagrama en bloques general de la Figura 3.1, donde se observa la conexión de todos los periféricos a la placa NUCLEO-F103RB y el reparto de la alimentación entre etapas. En particular, la Figura 3.1 destaca la fuente externa de 5 V dedicada al servomotor: al mantenerla separada de la alimentación de la NUCLEO se evita que los picos de corriente del actuador perturben la referencia del ADC.
 
<div align="center">
<img width="600" alt="Diagrama en bloques general" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/esquematico_completo.png" />
<p><em>Figura 3.1: Diagrama en bloques general del sistema.</em></p>
</div>

Para analizar la electrónica en detalle dividimos los circuitos en tres esquemas específicos, presentados en las Figuras 3.2 a 3.4.
 
La Figura 3.2 muestra la etapa de entradas. Allí se distinguen las dos entradas analógicas —el potenciómetro lineal de 10 kΩ sobre PA0 y el divisor LDR + 10 kΩ sobre PA7— y las dos entradas digitales con *pull-up* interno, correspondientes al pulsador de confirmación (PB0) y al sensor magnético de puerta (PA1).
 
<div align="center">
<img width="600" alt="Entradas analógicas y digitales" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/entrada_analogica_digitales.png" />
<p><em>Figura 3.2: Esquema eléctrico de las entradas del sistema.</em></p>
</div>

La Figura 3.3 detalla la topología del bus I²C1 remapeado a PB8/PB9, sobre el cual conviven los tres esclavos del sistema: el expansor PCF8574 del LCD (0x27), el RTC DS3231 (0x68) y la EEPROM AT24C32 (0x57). Como se aprecia en la Figura 3.3, las resistencias de *pull-up* son únicas para todo el bus, ya que los módulos comerciales traen las suyas y su conexión en paralelo reduce la resistencia equivalente.
 
<div align="center">
<img width="600" alt="Bus I2C" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/bus_i2c.png" />
<p><em>Figura 3.3: Topología del bus I²C compartido.</em></p>
</div>

Finalmente, la Figura 3.4 presenta la etapa de salida: el servomotor SG90 comandado por PWM desde PB6 y las salidas de señalización sobre PA4 (LED rojo + *buzzer* de alarma) y PA6 (LED verde de clave correcta).
 
<div align="center">
<img width="600" alt="Salidas del sistema" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/salida.png" />
<p><em>Figura 3.4: Esquema eléctrico de las salidas y actuadores.</em></p>
</div>

Para complementar los diagramas teóricos, las Figuras 3.5 a 3.7 documentan el montaje físico final. La Figura 3.5 ofrece una vista general del interior del gabinete de madera, con la disposición de la placa principal, la placa experimental y el cableado. La Figura 3.6 y la Figura 3.7 amplían el detalle de las conexiones soldadas sobre la placa experimental y la ubicación de los módulos I²C. La Figura 3.8, por último, muestra la vista frontal del prototipo terminado, con el dial, el LCD y el pulsador de confirmación accesibles al usuario.
 
<div align="center">
<img width="600" alt="Vista interior del gabinete" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/Matias-J-Sanchez-Q-patch-1/Imagenes/atras_gabinete.jpeg" />
<p><em>Figura 3.5: Vista interior del gabinete mostrando el montaje físico y cableado general.</em></p>
</div>

<div align="center">
<img width="600" alt="Detalle de la placa experimental" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/Matias-J-Sanchez-Q-patch-1/Imagenes/pcb.jpeg" />
<p><em>Figura 3.6: Detalle de las conexiones sobre la placa experimental y disposición de los módulos.</em></p>
</div>

<div align="center">
<img width="600" alt="Detalle de la placa experimental" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/Matias-J-Sanchez-Q-patch-1/Imagenes/pcb_2.jpeg" />
<p><em>Figura 3.7: Vista alternativa de la disposición interior, detallando la distribución de los componentes.</em></p>
</div>

<div align="center">
<img width="600" alt="Vista frontal del prototipo" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/frente.jpeg" />
<p><em>Figura 3.8: Vista frontal del proyecto.</em></p>
</div>

La Tabla 3.1 resume la asignación de pines que se desprende de las Figuras 3.2 a 3.4.
 
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
 
En esta sección se detalla la concepción y estructura del software desarrollado para el microcontrolador. El firmware fue diseñado bajo un enfoque *bare-metal* fuertemente orientado a eventos, priorizando la modularidad, el determinismo temporal y la eficiencia energética. A continuación se describe la arquitectura de ejecución elegida, las configuraciones críticas del reloj del sistema y la implementación de la lógica de control mediante máquinas de estado finito no bloqueantes. 
 
### 3.2.1 Arquitectura: ejecutivo cíclico y Event-Triggered System
 
El firmware se organiza como un **ejecutivo cíclico** gobernado por el `SysTick`, configurado a 1000 ticks por segundo. El *callback* del SysTick incrementa un contador de ticks pendientes; el lazo principal detecta ese contador y, por cada tick pendiente, ejecuta **una vuelta completa** de la lista de tareas en el orden fijo que ilustra la Figura 3.9:
 
```
    ESCRUTAR              PROCESAR              ACTUAR
 ┌──────────────┐     ┌──────────────┐     ┌────────────────┐
 │ task_sensor  │     │ task_system  │     │ task_actuator  │
 │ task_analog  │───▶│    (FSM)     │───▶ │ task_display   │
 └──────────────┘     └──────────────┘     │ task_storage   │
                                           └────────────────┘
```
 
<p><em>Figura 3.9: Orden de despacho de las tareas dentro de una vuelta del ejecutivo cíclico.</em></p>
</div>

El orden que muestra la Figura 3.9 no es arbitrario: garantiza que un evento generado por un sensor en el tick *N* sea procesado por la FSM y ejecutado por los actuadores en ese mismo tick, acotando la latencia de punta a punta a 1 ms.
 
Ninguna tarea bloquea. No se utiliza `HAL_Delay()` en el lazo principal: todas las temporizaciones (mensajes, refresco de UI, inactividad, pulsos de alarma, ciclo de escritura de la EEPROM) se resuelven con contadores de ticks.
 
Las tareas se comunican **exclusivamente por interfaces**, nunca por variables globales compartidas:
 
- `task_system_interface`: cola de eventos circular de 16 posiciones. Es segura frente a interrupciones (las operaciones sobre los índices son secciones críticas), ya que la ISR del botón B1 también encola eventos.
- `task_actuator_interface`: despacha eventos a un actuador identificado por su ID.
- `task_analog_interface`, `task_display_interface`, `task_storage_interface`: exponen la funcionalidad de cada tarea sin revelar su implementación.

### 3.2.2 Reloj del sistema
 
El sistema opera a **64 MHz** (HSI 8 MHz ÷ 2 × PLL 16). Esta configuración exige dos ajustes que no son opcionales: `FLASH_LATENCY_2` (dos estados de espera; con 0 el núcleo lee instrucciones corruptas) y un prescaler de ADC de ÷6, que deja el reloj de conversión en 10,67 MHz, por debajo del máximo de 14 MHz que admite el periférico.
 


### 3.2.3 Máquinas de estado
 
El sistema implementa cinco FSM independientes, una por tarea. Solo una de ellas concentra la lógica de negocio; las cuatro restantes son de servicio y traducen eventos de hardware en eventos de aplicación, o viceversa.
 
**FSM del sistema (`task_system`)** — es la única tarea con lógica de negocio y no accede a ningún periférico. Sus estados se corresponden con los tres modos de operación exigidos por la consigna según la distribución que resume la Tabla 3.2.
 
| Modo | Estados |
| :--- | :--- |
| NORMAL | `ST_SYS_VERIFY`, `ST_SYS_MSG` |
| SET_UP | `ST_SYS_CHANGE`, `ST_SYS_MENU_SELECT`, `ST_SYS_MENU_LOG`, `ST_SYS_MENU_CLOCK` |
| REPOSO | `ST_SYS_SLEEP` |
 
<p align="center"><em>Tabla 3.2: Correspondencia entre modos de operación y estados de la FSM del sistema.</em></p>

En la Figura 3.10 se presenta el diagrama de la máquina de estados del sistema.
 
<div align="center">
<img width="900" alt="Diagrama de estados del sistema" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/main/Imagenes/diagrama_estados.jpeg" />
<p><em>Figura 3.10: Diagrama de la máquina de estados del sistema.</em></p>
</div>

Se observa que, tras el *reset*, la FSM adopta como estado inicial `ST_SYS_VERIFY`, correspondiente al modo NORMAL: es el estado en el que el sistema pasa la mayor parte del tiempo y el único desde el cual puede liberarse la traba.
 
Dentro de `ST_SYS_VERIFY` se aprecia un autolazo disparado por el evento de confirmación del pulsador (PB0). Cada activación de ese lazo captura el dígito que indica el dial e incrementa el índice de la posición en curso, sin abandonar el estado. Recién cuando se confirma el cuarto dígito la FSM evalúa la combinación contra la clave persistida en EEPROM y transiciona a `ST_SYS_MSG`, llevando consigo el resultado de la comparación. Este último es un estado transitorio y temporizado: mientras permanece en él, la FSM sostiene el mensaje en el LCD y solicita a `task_actuator` el pulso correspondiente —LED verde ante acierto, LED rojo y *buzzer* ante error— y al expirar el temporizador retorna incondicionalmente a `ST_SYS_VERIFY`. La ausencia de cualquier otra salida desde `ST_SYS_MSG` es deliberada: garantiza que el sistema no pueda quedar retenido mostrando un mensaje.
 
Las transiciones horizontales del diagrama, rotuladas con el evento del pulsador de usuario B1 (PC13, atendido por interrupción externa), son las que recorren cíclicamente los tres modos de operación. Desde `ST_SYS_VERIFY` se accede a `ST_SYS_CHANGE`, desde allí a `ST_SYS_MENU_SELECT`, y desde este último se regresa a `ST_SYS_VERIFY`, cerrando el ciclo. Se observa que este recorrido no depende del estado interno de ingreso de dígitos: al cambiar de modo, el índice de dígito se reinicia y la combinación parcial se descarta.
 
En el modo SET_UP se distinguen dos ramas independientes. La rama de `ST_SYS_CHANGE` replica el mecanismo de ingreso de cuatro dígitos de `ST_SYS_VERIFY`, pero al completarse no compara la combinación sino que la encola para su escritura diferida en EEPROM y deriva a `ST_SYS_MSG` con la confirmación. La rama de `ST_SYS_MENU_SELECT` implementa el nivel superior del menú: el dial recorre las opciones sin generar transiciones, y solo la confirmación con PB0 produce el descenso al segundo nivel, hacia `ST_SYS_MENU_LOG` —donde se recorre el registro de los últimos diez intentos leído de la EEPROM— o hacia `ST_SYS_MENU_CLOCK` —donde se consulta la base de tiempo del RTC—. Ambos estados hoja retornan a `ST_SYS_MENU_SELECT`, de modo que el menú nunca se abandona lateralmente.
 
Por último, se observa que `ST_SYS_SLEEP` es alcanzable desde todos los estados anteriores mediante una única condición común: el vencimiento del temporizador de 15 s sin actividad del usuario. Al ingresar, la FSM apaga el LCD y su retroiluminación y habilita la suspensión del núcleo. La transición de salida es igualmente convergente: cualquier evento encolado —movimiento del dial, pulsador, B1 o sensor magnético de puerta— despierta el sistema y lo devuelve al modo NORMAL. Esta topología, con un único nodo de reposo alcanzable desde todos los modos, es la que permite acotar el consumo sin multiplicar las transiciones del diagrama.
 
Cabe señalar que la condición de armado del cerrojo, no aparece como estado en la Figura 3.10. Se modela como una variable de la FSM y no como un nodo, porque es ortogonal a los modos de operación: el sistema puede estar armado o desarmado en cualquiera de ellos, y representarlo como estado duplicaría el diagrama completo.
 
Las cuatro FSM restantes son de servicio y no aparecen en la Figura 3.10:
 
**FSM de antirrebote (`task_sensor`)** — cuatro estados por entrada digital (`ST_BTN_UP`, `ST_BTN_FALLING`, `ST_BTN_DOWN`, `ST_BTN_RISING`), con ventana de 50 ms para el pulsador y 30 ms para el sensor magnético. Los estados intermedios `ST_BTN_FALLING` y `ST_BTN_RISING` son los que absorben el rebote mecánico: solo si la entrada se mantiene estable durante toda la ventana la FSM avanza al estado firme y emite el evento correspondiente; ante cualquier rebote regresa al estado de partida sin notificar nada.
 
**FSM de actuadores (`task_actuator`)** — `ST_ACT_OFF`, `ST_ACT_ON`, `ST_ACT_PULSE`, `ST_ACT_BLINK`. Es el único módulo del programa que escribe una salida física. Los estados `ST_ACT_PULSE` y `ST_ACT_BLINK` incorporan su propio temporizador, lo que permite a `task_system` solicitar un pulso de duración acotada con una sola llamada y desentenderse de su finalización.
 
**FSM del display (`task_display`)** y **FSM de almacenamiento (`task_storage`)** — operan sobre el bus I²C de la Figura 3.3 mediante un árbitro de *token*.
 
 ---

# Capítulo 4: Ensayos y resultados

Este capítulo expone los ensayos realizados para validar el correcto funcionamiento de la cerradura electrónica, así como las métricas de rendimiento y eficiencia obtenidas sobre el prototipo final. 

## 4.1 Prueba de integración (Video)

*   **Enlace al video:** [Ver video en Google Drive](https://drive.google.com/file/d/1aX3uYLCGlMD5LRpf_hL87mvzt0ArB9Q4/view?usp=drivesdk)

<div align="center">
<img width="600" alt="Captura del video de integración" src= https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/Matias-J-Sanchez-Q-patch-1/Imagenes/video_prueba.png />
<p><em>Figura 4.1: Captura de pantalla del ensayo de integración mostrando la validación del acceso.</em></p>
</div>

En el registro audiovisual se demuestra el funcionamiento completo del prototipo, partiendo desde el modo de reposo hasta la apertura exitosa del cerrojo tras ingresar la clave correspondiente mediante el dial analógico como se muestra en la Figura 4.1. Asimismo, se expone la respuesta del sistema ante una contraseña inválida y el accionamiento de las alertas visuales y sonoras.

## 4.2 Pruebas funcionales de hardware y firmware

Para verificar la correcta integración de los componentes antes del despliegue final sobre el prototipo, se realizaron diversas pruebas funcionales de validación tanto a nivel físico como lógico, resumidas en la Tabla 4.1.

| Subsistema | Ensayo realizado | Resultado / Criterio de validación | Estado |
| :--- | :--- | :--- | :---: |
| **Hardware** | Verificación de continuidad  | Ausencia de cortocircuitos o falsos contactos en la placa experimental | ✅ |
| **Hardware** | Acondicionamiento de entradas analógicas | Correcta atenuación y respuesta lineal en el ADC (potenciómetro y LDR) | ✅ |
| **Firmware** | FSM de antirrebote de pulsadores | Filtrado exitoso de rebotes mecánicos en el botón de confirmación y sensor magnético | ✅ |
| **Firmware** | Muestreo ADC y mapeo de dígitos | Mapeo estable de las cuentas del conversor a valores discretos (0 a 9) | ✅ |
| **Firmware** | Persistencia en EEPROM y RTC | Lectura y escritura correcta de marcas de tiempo e historial por bus I²C | ✅ |
| **Firmware** | Máquina de estados global | Transiciones robustas y sin bloqueos entre `NORMAL`, `SET_UP` y `REPOSO` | ✅ |

<p align="center"><em>Tabla 4.1: Resumen de ensayos funcionales de hardware y firmware.</em></p>


## 4.3 Ocupación de memoria: Console & Build Analyzer
 
La Figura 4.2 reproduce la salida del *Build Analyzer* de STM32CubeIDE para el binario entregado, sirviendo como evidencia de la compilación exitosa.

<div align="center">
<img width="800" alt="Build Analyzer" src="https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02/raw/Matias-J-Sanchez-Q-patch-1/Imagenes/Build_analyzer.png" />
<p><em>Figura 4.2: Reporte de uso de memoria RAM y FLASH.</em></p>
</div>

Para facilitar el análisis de estos resultados, la Tabla 4.1 consolida el tamaño de cada sección del binario (`.text`, `.data` y `.bss`) y su impacto directo sobre la ocupación efectiva en las regiones físicas del microcontrolador STM32F103RB.

| Región Física | Secciones del Binario | Usado [Bytes] | Total Disponible [Bytes] | Ocupación |
| :--- | :--- | :---: | :---: | :---: |
| **FLASH** | `.text` (34.476) + `.data` (188) | 34.664 | 131.072 | **26,45 %** |
| **RAM** | `.bss` (2.868) + `.data` (188) | 3.056 | 20.480 | **14,92 %** |

<p align="center"><em>Tabla 4.1: Desglose de secciones del binario y ocupación de memoria.</em></p>

Como se observa en la métrica final, el firmware utiliza aproximadamente un cuarto de la memoria de programa (FLASH) disponible y menos de un sexto de la memoria dinámica (RAM), dejando un margen operativo sumamente holgado para las mejoras y expansiones propuestas en la sección 5.2.
 
## 4.4 Medición y análisis de tiempos de ejecución (WCET)

Para garantizar el determinismo y el cumplimiento de las restricciones de tiempo real del ejecutivo cíclico, resulta fundamental analizar el desempeño temporal del microcontrolador. En esta sección se describe el método de perfilado utilizado mediante el hardware interno del procesador Cortex-M3 y se exponen los resultados empíricos obtenidos para el mejor y el peor caso de ejecución.

### 4.4.1 Metodología
 
El firmware está instrumentado con el contador de ciclos del DWT (*Data Watchpoint and Trace*) del Cortex-M3. Antes de invocar cada tarea se reinicia el contador; al retornar se lee, obteniéndose el tiempo de ejecución de esa tarea en esa vuelta. Sobre esa base se acumulan, para cada tarea:
 
- **BCET** *(Best-Case Execution Time)* — mejor caso observado, en µs.
- **WCET** *(Worst-Case Execution Time)* — peor caso observado, en µs.

Todas estas variables son inspeccionables por *Live Expressions* en el arreglo `task_dta_list[]`. A nivel de sistema se registran además `g_app_runtime_us` (duración de la última vuelta completa), `g_app_wcet_us` (peor vuelta observada) y `g_app_overrun_cnt` (cantidad de vueltas que excedieron el período).
 
**Nota:** El WCET aquí reportado es un **peor caso observado**, no un peor caso demostrado por análisis estático. Para que la medición sea representativa, el sistema debe ejercitarse recorriendo los caminos más costosos: refresco completo del LCD, escritura de clave en EEPROM, navegación del registro de intentos y disparo de la alarma.
 
### 4.4.2 Resultados
 
La Tabla 4.3 presenta los tiempos de ejecución medidos sobre la vuelta completa del ejecutivo cíclico descrito en la Figura 3.9.
 
| Tarea | BCET [µs] | WCET [µs] |
| :--- | :---: | :---: |
| **Vuelta completa** | 300 | 615 |
 
<p align="center"><em>Tabla 4.3: Tiempos de ejecución medidos mediante el contador DWT.</em></p>

## 4.5 Cálculo del Factor de Uso (U) de la CPU
 
El factor de uso se define como el cociente entre el tiempo de cómputo del peor caso y el período de despacho, según la Ecuación (4.1):
 
$$U = \frac{C}{T} \qquad (4.1)$$
 
donde **T = 1000 µs** es el período del ejecutivo cíclico, fijado por el SysTick. T es un dato de diseño, no una incógnita: no se despeja a partir de U, sino que U se calcula a partir de él.
 
Reemplazando en la Ecuación (4.1) el WCET de la Tabla 4.3, se obtiene un Factor de uso máximo del **61,5 %**. El firmware calcula este valor en tiempo de ejecución y lo expone en `g_app_u_pct`.
 
 El valor total obtenido ($U = 0,615$) corresponde a una **cota conservadora** (peor caso absoluto). Representa el instante de mayor exigencia computacional de la cerradura, que ocurre únicamente cuando se combinan múltiples eventos de hardware lentos en un mismo ciclo. 

En contraste, las mediciones experimentales demostraron que durante el régimen de operación normal (espera de ingreso de dígitos o validación), el tiempo de ejecución es sensiblemente menor, acercándose al BCET de 300 µs (lo que representa un uso de CPU de apenas el 30 %). 

Dado que la condición de planificabilidad $U < 1$ se verifica con holgura incluso en el pico de mayor estrés, se demuestra que el microcontrolador cuenta con margen de procesamiento suficiente. Este tiempo ocioso garantizado (al menos 385 µs en el peor de los casos) es el que la arquitectura aprovecha eficientemente para suspender el núcleo mediante la instrucción `WFI`, respaldando la estrategia de bajo consumo detallada en la sección de mediciones energéticas.
 
## 4.6 Medición y análisis de consumo
 
 
Para evaluar el impacto energético del sistema, se procedió a medir exclusivamente la corriente consumida en los distintos modos de operación.
* La medición se realizó intercalando un multímetro configurado como amperímetro en serie con la línea de alimentación.
* Se registraron los valores en tres escenarios clave: durante la validación de clave, al accionar el servomotor y durante la suspensión del procesador (modo reposo).

La Tabla 4.4 resume el consumo medido en los tres regímenes de operación del sistema.
 
| Modo de operación | Corriente |
| :--- | :---: |
| Ingresando clave (NORMAL) | 7,6 mA |
| Apertura de puerta (servo + lógica) | 12,4 mA |
| Reposo (display off + WFI) | 4,7 mA |
 
<p align="center"><em>Tabla 4.4: Consumo energético del sistema por modo de operación.</em></p>

La comparación entre la primera y la tercera fila de la Tabla 4.4 cuantifica el efecto de la estrategia de bajo consumo descrita en la sección 3.4: apagar el LCD y suspender el CPU reduce la corriente a poco más de la mitad respecto del modo activo.
 
## 4.7 Cumplimiento de requisitos

La Tabla 4.5 resume el estado final de cumplimiento de los requisitos del sistema, discriminando el aporte y la implementación a nivel de hardware y firmware. Se incluye también el requisito preliminar de monitoreo de batería para mantener la trazabilidad con la propuesta original.

| ID | Requisito (versión final) | Hardware | Software | Estado final |
| :---: | :--- | :---: | :---: | :---: |
| **RQ01** | El sistema debe operar en bajo consumo (REPOSO), suspendiendo el LCD y el CPU tras 15 s de inactividad. | 🟢 | 🟢 | ✅ |
| **RQ02** | El sistema debe proveer retroalimentación visual en una pantalla LCD operada por bus I²C. | 🟢 | 🟢 | ✅ |
| **RQ03** | La contraseña estará formada por 4 dígitos numéricos (0 a 9), confirmados individualmente. | N/A | 🟢 | ✅ |
| **RQ04** | El sistema debe permitir la selección de dígitos mapeando la tensión de un potenciómetro lineal. | 🟢 | 🟢 | ✅ |
| **RQ05** | Habilitar el modo configuración (SET_UP) para cambio de contraseña únicamente tras una apertura exitosa. | 🟢 | 🟢 | ✅ |
| **RQ06** | Tras la validación exitosa, el sistema debe accionar un servomotor mediante PWM para liberar la traba física. | 🟢 | 🟢 | ✅ |
| **RQ07** | En modo armado, la apertura forzada de la puerta o la detección de luz interna (LDR) debe activar una alarma. | 🟢 | 🟢 | ✅ |
| **RQ08** | El sistema debe advertir mediante señales visuales y sonoras el ingreso de una combinación incorrecta. | 🟢 | 🟢 | ✅ |
| **RQ09** | El sistema debe registrar en la EEPROM la estampa de tiempo (RTC) tras cada intento de acceso. | 🟢 | 🟢 | ✅ |
| **RQ10** | El sistema debe recuperar de memoria y mostrar en el LCD el registro histórico de intentos. | 🟢 | 🟢 | ✅ |
| **RQ_BAT** | *(Preliminar)* Medir continuamente la tensión de la batería por ADC e informar mediante un LED si el nivel es bajo. | 🟡 | 🔴 | 🔴 |

<p align="center"><em>Tabla 4.5: Cumplimiento final de requisitos.</em></p>

**Leyenda:**
- 🟢 Implementado
- 🟡 Parcialmente cumplido / hardware disponible pero sin soporte de firmware
- 🔴 No implementado / descartado
- ✅ Cumplido

Los requisitos de la versión final (RQ01 a RQ10) se cumplieron en su totalidad, logrando un sistema funcional, robusto y con un manejo eficiente de la energía a través de la arquitectura *Event-Triggered*. 

Respecto a los requisitos no implementados:
* **RQ_BAT (Monitoreo de batería):** Esta funcionalidad, planteada en la etapa de planificacion, fue descartada debido a restricciones de tiempo durante la etapa final de integración. Se tomó la decisión de priorizar el desarrollo, la depuración y la estabilidad de las funciones críticas del sistema.

---

# Capítulo 5: Conclusiones

## 5.1 Resultados obtenidos

El prototipo cumple con la mayoría de los requisitos funcionales planteados. La integración del RTC DS3231 con la memoria EEPROM sobre un mismo bus I²C permitió construir un sistema de auditoría persistente, y el aislamiento de la alimentación del servomotor evitó que sus picos de consumo perturbaran el ADC del microcontrolador.

El aprendizaje más relevante del trabajo no estuvo en la funcionalidad sino en el **tiempo real**. La primera versión del firmware funcionaba correctamente desde el punto de vista del usuario, pero violaba sistemáticamente el presupuesto temporal: el `HAL_Delay(5)` de la EEPROM bloqueaba el micro cinco veces el período completo del ejecutivo, y la escritura del LCD lo hacía por varios milisegundos más. Que un sistema "ande" no implica que sea determinista. Reescribirlo como Event-Triggered System obligó a atacar cada fuente de bloqueo por separado (framebuffer para el display, cola diferida para la EEPROM, conversión en dos ticks para el ADC, árbitro de token para el bus compartido) y solo entonces el factor de uso se volvió una magnitud medible y acotada.

## 5.2 Próximos pasos

Si bien el prototipo actual alcanzó satisfactoriamente los objetivos funcionales y de eficiencia energética planteados para esta etapa académica, el desarrollo evidenció diversas oportunidades de mejora. De cara a una futura iteración orientada a aumentar la robustez y escalar el sistema hacia un producto comercial más completo, se proponen las siguientes líneas de trabajo:

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

[8] Repositorio de entregas: [https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02](https://github.com/Matias-J-Sanchez-Q/tdse-tf_2026-1erC_2-02)`
