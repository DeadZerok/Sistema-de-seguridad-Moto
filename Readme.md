# Sistema de Seguridad Inteligente para Motocicletas con ESP32
---

## 💡 Descripción del Proyecto

Este proyecto integral ofrece una solución avanzada de **seguridad y control de acceso para motocicletas** de bajo y mediano cilindraje, utilizando el versátil microcontrolador **ESP32**. Su objetivo principal es asegurar que el paso de corriente (para encendido y arranque del motor) solo se active bajo una **identidad completamente validada y autorizada**.

El sistema integra múltiples métodos de autenticación, incluyendo **lectura de huella digital**, **tarjetas RFID autorizadas** y **control remoto a través de un bot de Telegram**. Además de la autenticación, incorpora funciones de seguridad proactivas como un **sensor de vibración** para detectar intentos de manipulación, un **buzzer de alerta** y un sistema de **notificaciones en tiempo real vía Telegram**. Diseñado para ser robusto, el sistema puede operar en un **modo offline** en caso de pérdida de conectividad, manteniendo la seguridad local de la motocicleta.

---

## ✨ Características Principales

* **Autenticación Multifactor:** Permite el encendido solo con la validación de:
    * **Huella Digital:** Reconocimiento biométrico de usuarios autorizados.
    * **Tarjeta RFID:** Lectura de UIDs de tarjetas previamente autorizadas.
    * **Telegram Bot:** Control remoto seguro a través de comandos específicos.
* **Encendido Secuencial Controlado:** Activa relés específicos (encendido y arranque) solo tras una autenticación exitosa.
* **Detección de Vibración:** Un sensor monitorea movimientos o intentos de manipulación no autorizados.
* **Alertas Sonoras:** Buzzer integrado para emitir alarmas ante eventos de seguridad o intentos fallidos de acceso.
* **Interacción por Telegram:**
    * **Menú Interactivo:** Controla y monitorea el estado de la motocicleta de forma remota.
    * **Notificaciones de Seguridad:** Recibe alertas automáticas por vibración, intentos de acceso fallidos, etc.
    * **Contador de Intentos Fallidos:** Monitorea y registra los intentos de acceso no válidos.
* **Modo Offline Robusto:** Mantiene la funcionalidad de seguridad básica incluso sin conexión WiFi o a Telegram.
* **Gestión Flexible:** Soporte para múltiples tarjetas RFID y huellas digitales autorizadas.

---

## 🛠️ Hardware Necesario

* **ESP32:** Cualquier modelo de placa de desarrollo ESP32.
* **Sensor de Huellas Digitales:** Modelos compatibles con `Adafruit_Fingerprint` (ej., R307, ZFM, AS608, dy50).
* **Lector RFID RC522:** Módulo lector/escritor RFID.
* **2 Módulos de Relé:** Para controlar la corriente de encendido y arranque de la motocicleta. Se recomiendan relés de estado sólido para mayor durabilidad en entornos vehiculares.
    * **Relé de Encendido** permite el paso de coriente hacia el tablero y demas dispositivos.
    * **Relé de Arranque** activa el motor de arranque la motocicleta por unos instantes y ya encendida deja de activarse.
* **Buzzer:** Para las alertas sonoras.
* **Sensor de Vibración:** (Ej., SW-420 ) para detectar movimiento.
* **Tarjeta(s) RFID:** Una o más tarjetas Mifare 1K para autorizar el acceso.
* **Cables Jumper:** Para todas las interconexiones.
* **Fuente de Alimentación:** Adecuada para el ESP32 y los módulos (ej., 12V DC de la motocicleta con un regulador a 5V/3.3V).

### Conexiones de Hardware Típicas

* **Relé de Arranque** $\leftrightarrow$ **GPIO 21 (ESP32)**
* **Relé de Encendido** $\leftrightarrow$ **GPIO 25 (ESP32)**
* **Buzzer** $\leftrightarrow$ **GPIO 2 (ESP32)**
* **Sensor de Vibración** $\leftrightarrow$ **GPIO 4 (ESP32)**
* **Sensor de Huellas (RX del sensor)** $\leftrightarrow$ **GPIO 14 (ESP32)**
* **Sensor de Huellas (TX del sensor)** $\leftrightarrow$ **GPIO 27 (ESP32)**
* **Lector RFID RC522 (SPI):**
    * `SDA` (SS) $\leftrightarrow$ **GPIO 5 (ESP32)**
    * `RST` $\leftrightarrow$ **GPIO 22 (ESP32)**
    * `MOSI` $\leftrightarrow$ **GPIO 23 (ESP32)**
    * `MISO` $\leftrightarrow$ **GPIO 19 (ESP32)**
    * `SCK` $\leftrightarrow$ **GPIO 18 (ESP32)**

*(Asegúrate de conectar VCC y GND de todos los módulos a las fuentes de alimentación adecuadas del ESP32.)*

---

## 📚 Librerías Requeridas

Para compilar este proyecto, necesitarás instalar las siguientes librerías desde el **Administrador de Librerías** del Arduino IDE:

* `Adafruit_Fingerprint`
* `MFRC522` by GithubCommunity
* `CTBot` (para la integración con Telegram)
* `WiFi` (generalmente viene con el soporte de placa ESP32)
* `SPI` (generalmente viene incluida)
* `ArduinoJson` (puede ser requerida por `CTBot` para el manejo de mensajes JSON, versión 5 ).

---

## 📦 Estructura de Archivos

Este proyecto podría tener una estructura de archivos similar a la siguiente (la cual podría variar ligeramente):

* `main.ino` o `Moto_Seguridad.ino`: El archivo principal con la lógica central del sistema.
* `token.h`: **¡Archivo crucial y no público!** Contiene tus credenciales Wi-Fi, el token de tu bot de Telegram, y posiblemente IDs de usuarios/chats autorizados.
* `rfid_manager.h` / `fingerprint_manager.h`: (Opcional, si el código está modularizado) Archivos para encapsular la lógica de RFID y huella digital.
* `.gitignore`: Para asegurar que `token.h` y otros archivos sensibles no se suban al repositorio.

---

## 🚀 Cómo Usar e Implementar

1.  **Clona el Repositorio:** Descarga el código a tu máquina local.
2.  **Configura un Bot de Telegram:**
    * Habla con [@BotFather](https://t.me/botfather) en Telegram para crear un nuevo bot y obtener su **token HTTP API**.
    * Para obtener tu `chat_id`, envía un mensaje a tu nuevo bot y luego visita `https://api.telegram.org/bot<TU_BOT_TOKEN>/getUpdates` (reemplazando `<TU_BOT_TOKEN>` con el token de tu bot) en tu navegador, o usa servicios como [@userinfobot](https://t.me/userinfobot).
3.  **Crea `token.h`:**
    * En la misma carpeta que el archivo `.ino` principal, crea un nuevo archivo llamado **`token.h`**.
    * Dentro de este archivo, define tus credenciales Wi-Fi y los tokens de Telegram. Un ejemplo básico podría ser:
        ```cpp
        // token.h
        #ifndef TOKEN_H
        #define TOKEN_H

        const char* ssid = "Tu_SSID_WiFi";          // Reemplaza con el nombre de tu red WiFi
        const char* password = "Tu_Contraseña_WiFi";   // Reemplaza con la contraseña de tu red WiFi
        const char* telegramBotToken = "TU_BOT_TOKEN_AQUI"; // Reemplaza con el token de tu bot de Telegram
        const long ownerChatID = TU_CHAT_ID_AQUI;      // Reemplaza con tu Chat ID de Telegram (para notificaciones directas)

        #endif // TOKEN_H
        ```
4.  **Abre en Arduino IDE:** Abre el archivo `.ino` principal del proyecto en el Arduino IDE.
5.  **Instala Librerías:** Asegúrate de que todas las librerías mencionadas en la sección "Librerías Requeridas" estén instaladas.
6.  **Conecta el Hardware:** Realiza todas las conexiones de los sensores, relés, buzzer y lector RFID al ESP32 según el esquema.
7.  **Registra Huellas y RFID:**
    * Utiliza los ejemplos de las librerías `Adafruit_Fingerprint` y `MFRC522` para registrar huellas digitales y obtener los UIDs de las tarjetas RFID que deseas autorizar en el sistema. Estos UIDs y IDs de huella deberán ser configurados en el código principal o en archivos de configuración específicos.
8.  **Compila y Sube:** Selecciona tu placa ESP32 y el puerto COM correcto, luego compila y sube el código a tu ESP32.
9.  **Prueba el Sistema:**
    * Abre el **Monitor Serial** (a 115200 baudios) para ver el estado de la conexión y los mensajes de depuración.
    * Prueba los diferentes métodos de autenticación (huella, RFID, comandos de Telegram).
    * Simula vibraciones y observa las notificaciones en Telegram y las alertas del buzzer.

---

## 🔐 Consideraciones de Seguridad

* **Protección de Credenciales:** Es fundamental que el archivo `token.h` nunca se suba a repositorios públicos. Utiliza siempre `.gitignore` para excluirlo.
* **Gestión de IDs:** Para una mayor seguridad, se recomienda implementar un sistema para añadir y eliminar huellas/UIDs de forma segura, idealmente sin recompilar el código.
* **Alarma y Bloqueo:** El sistema incluye funciones de bloqueo temporal tras múltiples intentos fallidos y alarmas sonoras para disuadir a intrusos.
* **Protección Física:** Asegura que el ESP32 y los componentes sensibles estén protegidos físicamente dentro de la motocicleta para evitar manipulaciones.
* **Fiabilidad de los Relés:** Utiliza relés adecuados para las corrientes de la motocicleta y considera protecciones adicionales.

---