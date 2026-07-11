# Informe de Avances - Cerradura Electrónica de Alta Seguridad

**Autores:**
* Sanchez Q., Matias J. - Padrón 111060
* Laskowski, Marcos - Padrón 104028
* Fanzi, Francisco - Padrón 107510

**Fecha:** 14/06/2026
**Cuatrimestre:** 1er cuatrimestre 2026 (Curso-Grupo: 2-02)

A continuación se detalla el informe de avances del Trabajo Final a partir de los requerimientos planteados en la propuesta. Actualmente el proyecto se encuentra en su fase inicial organizativa y de adquisición de componentes.

| Estado | Descripción |
| :---: | :--- |
| 🟢 | Ya implementado |
| 🟡 | En proceso |
| 🔴 | Pendiente |

### 1. Hardware y Armado

| Req ID | Descripción | Estado |
| :---: | :--- | :---: |
| 1.1 | Adquisición de la placa NUCLEO-F103RB. | 🟢 |
| 1.2 | Adquisición de la totalidad de los sensores (dial, LDR, MC-38), módulos (RTC, EEPROM) y pantalla LCD. | 🟢 |
| 1.3 | Armado final en placa experimental con componentes soldados. | 🔴 |

### 2. Gestión de Energía

| Req ID | Descripción | Estado |
| :---: | :--- | :---: |
| 2.1 | El sistema operará en bajo consumo (REPOSO), activando el LCD y el escrutinio del potenciómetro solo tras una interrupción de usuario. | 🔴 |

### 3. Auditoría de Sistema

| Req ID | Descripción | Estado |
| :---: | :--- | :---: |
| 3.1 | El sistema medirá continuamente la tensión de la batería por ADC. | 🔴 |
| 3.2 | El sistema informará mediante el LED amarillo y un mensaje en el display si el nivel de batería es bajo. | 🔴 |

### 4. Seguridad y Acceso

| Req ID | Descripción | Estado |
| :---: | :--- | :---: |
| 4.1 | El sistema validará el ingreso de la combinación mediante un dial analógico posicional. | 🔴 |
| 4.2 | Se validará la contraseña actual antes de habilitar la sobrescritura de una nueva combinación en el modo SET_UP. | 🔴 |
| 4.3 | En modo REPOSO, la apertura no validada de la puerta (MC-38) activará la alarma visual y sonora. | 🔴 |
| 4.4 | En modo REPOSO, la detección de luz interna (LDR anti-sabotaje) activará la alarma visual y sonora. | 🔴 |

### 5. Almacenamiento y Reloj (I2C)

| Req ID | Descripción | Estado |
| :---: | :--- | :---: |
| 5.1 | El sistema registrará en la EEPROM la estampa de tiempo provista por el RTC tras cada apertura exitosa. | 🔴 |
| 5.2 | Al pulsar el botón de Historial, se mostrará el último evento de apertura en la pantalla. | 🔴 |

### 6. Interfaz Visual

| Req ID | Descripción | Estado |
| :---: | :--- | :---: |
| 6.1 | El sistema empleará una pantalla LCD 16x2 vía I2C como interfaz visual (reemplazando al módulo HM-10 según lo acordado con los docentes). | 🔴 |
