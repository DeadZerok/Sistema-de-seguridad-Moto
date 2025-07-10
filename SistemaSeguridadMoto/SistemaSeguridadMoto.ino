
// Declaracion de librerias 
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include "CTBot.h"
#include "token.h"
#include <WiFi.h>

// Definición de pines
#define RXD2 14        // Pin RX para sensor de huellas
#define TXD2 27        // Pin TX para sensor de huellas
#define SS_PIN 5       // Pin SS (SDA) para RFID
#define RST_PIN 22     // Pin RST para RFID
#define R_ENCENDIDO 25 // Pin para relé de encendido
#define R_ARRANQUE 21  // Pin para relé de arranque
#define BUZZER_PIN 2   // Pin para el buzzer
#define VIBRA_PIN 4    // Pin para el sensor de vibración


// Inicialización del bot de Telegram
CTBot miBot;
CTBotInlineKeyboard miTeclado;

// UID de la tarjeta autorizada
const byte TARJETAS_AUTORIZADAS[][4] = {
    {0x64, 0xCA, 0xE8, 0x2B},  // Tarjeta 1
    {0x23, 0xB1, 0x05, 0x2D}   // Tarjeta 2
};

const byte NUMERO_TARJETAS = sizeof(TARJETAS_AUTORIZADAS) / sizeof(TARJETAS_AUTORIZADAS[0]);

// Constantes de tiempo
const unsigned long TIEMPO_RELE = 2000;     // Tiempo de activación de relés (2 segundos)
const unsigned long TIEMPO_ESPERA = 100;    // Tiempo entre lecturas de tarjeta
const unsigned long TIEMPO_ALARMA = 5000;   // Tiempo de alarma larga (5 segundos)
const unsigned long TIEMPO_PITIDO = 1000;   // Tiempo de pitido corto (1 segundo)

// Constantes de tiempo de bot 
const unsigned long BOT_CHECK_INTERVAL = 100; // Intervalo para revisar mensajes de Telegram (100ms)
unsigned long lastBotCheck = 0; // Variable para controlar el tiempo de la última verificación

// Constantes de contadores
unsigned long contadorVibraciones = 0;

// Variables globales
MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_Fingerprint huella(&Serial2);
unsigned long ultimaLectura = 0;
unsigned long tiempoInicioAlarma = 0;
bool sistemaActivado = false;
bool alarmaActiva = false;
bool modoOffline = false;

// Contadores de intentos fallidos
int intentosFallidosHuella = 0;
int intentosFallidosRFID = 0;
const int MAX_INTENTOS = 3;

// Para formatear mensajes en Serial
template<class T> inline Print &operator <<(Print &obj, T arg) {
    obj.print(arg);
    return obj;
}

void inicializarTeclado() {
    // Primera fila de botones
    miTeclado.addButton("Encender Sistema", "encender", CTBotKeyboardButtonQuery);
    miTeclado.addButton("Apagar Sistema", "apagar", CTBotKeyboardButtonQuery);
    
    // Segunda fila
    miTeclado.addRow();
    miTeclado.addButton("Ver Estado", "estado", CTBotKeyboardButtonQuery);
    
    // Tercera fila
    miTeclado.addRow();
    miTeclado.addButton("Ver Estadísticas", "stats", CTBotKeyboardButtonQuery);
}

void enviarMenuPrincipal(int64_t chat_id) {
    miBot.sendMessage(chat_id, "🔐 Menu Principal del Sistema de Seguridad\n\nSeleccione una opción:", miTeclado);
}


// Función para mostrar el UID de una tarjeta
void mostrarUID(byte *buffer, byte bufferSize) {
    Serial.print("UID detectado: ");
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
    Serial.println();
}

// Función de inicialización de relés
void inicializarReles() {
    pinMode(R_ENCENDIDO, OUTPUT);
    pinMode(R_ARRANQUE, OUTPUT);
    digitalWrite(R_ENCENDIDO, LOW);
    digitalWrite(R_ARRANQUE, LOW);
}

// Inicializar buzzer y sensor de vibración
void inicializarSensores() {
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(VIBRA_PIN, INPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

// Función de inicialización del RFID
void inicializarRFID() {
    SPI.begin();
    rfid.PCD_Init();
}


// Función para intentar conectar a Telegram
bool conectarTelegram() {
    Serial.println("Intentando conectar a WiFi y Telegram...");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // Intenta conectar al WiFi con timeout
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nNo se pudo conectar al WiFi");
        return false;
    }
    
    Serial.println("\nWiFi conectado");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
    
    miBot.setTelegramToken(token);
    
    if (miBot.testConnection()) {
        Serial.println("Conexión con Telegram establecida");
        return true;
    }
    
    Serial.println("No se pudo conectar a Telegram");
    return false;
}


// Función de inicialización del sensor de huellas
bool inicializarHuella() {
    Serial2.begin(57600, SERIAL_8N1, RXD2, TXD2);
    huella.begin(57600);
    
    if (!huella.verifyPassword()) {
        Serial.println("Error: Sensor de huellas no conectado");
        return false;
    }
    
    Serial.print("Huellas almacenadas: ");
    Serial.println(huella.templateCount);
    return true;
}

// Función para activar el buzzer
void activarBuzzer(unsigned long duracion) {
    digitalWrite(BUZZER_PIN, HIGH);
    tiempoInicioAlarma = millis();
    alarmaActiva = true;
    
    // Esperar la duración especificada
    while (millis() - tiempoInicioAlarma < duracion) {
        delay(10);
    }
    
    digitalWrite(BUZZER_PIN, LOW);
    alarmaActiva = false;
}

// Función modificada para enviar mensaje considerando el modo offline
void enviarMensajeTelegram(const String& mensaje, int64_t chat_id = IDchat) {
    if (!modoOffline) {
        miBot.sendMessage(chat_id, mensaje);
    }
}

// Verificar vibración cuando el sistema está desactivado
void verificarVibracion() {
    static unsigned long ultimaVibracion = 0;
    const unsigned long TIEMPO_MINIMO_ENTRE_VIBRACIONES = 1000; // 1 segundo entre detecciones
    
    if (!sistemaActivado && digitalRead(VIBRA_PIN) == HIGH) {
        // Verificar si es una nueva vibración (para no contar múltiples veces la misma)
        if (millis() - ultimaVibracion > TIEMPO_MINIMO_ENTRE_VIBRACIONES) {
            contadorVibraciones++;
            Serial.println("¡Vibración detectada!");
            Serial.print("Número de activaciones: ");
            Serial.println(contadorVibraciones);
            
            String mensaje = "🚨 ¡ALERTA DE SEGURIDAD! #" + String(contadorVibraciones) + "\n\n";
            mensaje += "Se ha detectado vibración en el sistema.\n";
            mensaje += "Estado del sistema: " + String(sistemaActivado ? "Activado ✅" : "Desactivado ❌") + "\n";
            mensaje += "Tiempo: " + String(millis() / 1000 / 60) + " minutos desde el inicio\n";
            mensaje += "Total activaciones: " + String(contadorVibraciones);
            
            // Enviar notificación al usuario
            enviarMensajeTelegram(mensaje);

            // Actualizar tiempo de última vibración
            ultimaVibracion = millis();
            
            activarBuzzer(TIEMPO_ALARMA);
            Serial.println("Buzzer activado 5 segundos!");
        }
    }
}

// Manejar intentos fallidos
void manejarIntentoFallido(bool esHuella) {
    if (esHuella) {
        intentosFallidosHuella++;
        Serial.print("Intentos fallidos de huella: ");
        Serial.println(intentosFallidosHuella);
        
        if (intentosFallidosHuella >= MAX_INTENTOS) {
            if (intentosFallidosHuella == MAX_INTENTOS) {
                activarBuzzer(TIEMPO_PITIDO);
                Serial.println("buzzer activado un segundo");
            } else {
                activarBuzzer(TIEMPO_ALARMA);
                Serial.println("buzzer activado Cinco segundos");
            }
        }
    } else {
        intentosFallidosRFID++;
        Serial.print("Intentos fallidos de RFID: ");
        Serial.println(intentosFallidosRFID);
        
        if (intentosFallidosRFID >= MAX_INTENTOS) {
            if (intentosFallidosRFID == MAX_INTENTOS) {
                activarBuzzer(TIEMPO_PITIDO);
                Serial.println("buzzer activado un segundo");
            } else {
                activarBuzzer(TIEMPO_ALARMA);
                Serial.println("buzzer activado Cinco segundos");
            }
        }
    }
}

// Restablecer contadores de intentos
void restablecerContadores() {
    intentosFallidosHuella = 0;
    intentosFallidosRFID = 0;
}

// Función modificada para activar relés
void activarReles(int64_t chat_id = 0) {
    sistemaActivado = true;
    digitalWrite(R_ENCENDIDO, HIGH);
    delay(TIEMPO_RELE);
    
    digitalWrite(R_ARRANQUE, HIGH);
    delay(TIEMPO_RELE);
    digitalWrite(R_ARRANQUE, LOW);

    if (chat_id != 0 && !modoOffline) {
        miBot.sendMessage(chat_id, "Sistema activado exitosamente");
    }
}

// Función para desactivar relés por chatbot
void desactivarReles(int64_t chat_id = 0) {
    sistemaActivado = false;
    digitalWrite(R_ENCENDIDO, LOW);
    digitalWrite(R_ARRANQUE, LOW);

    // Si se proporcionó un chat_id, enviamos notificación
    if (chat_id != 0) {
        miBot.sendMessage(chat_id, "Sistema desactivado exitosamente");
    }
}

// Verificar tarjeta RFID
bool verificarRFID() {
    if (millis() - ultimaLectura < TIEMPO_ESPERA) {
        return false;
    }
    ultimaLectura = millis();

    if (!rfid.PICC_IsNewCardPresent()) {
        return false;
    }
    
    if (!rfid.PICC_ReadCardSerial()) {
        return false;
    }

    Serial.println("\n=== Tarjeta Detectada ===");
    mostrarUID(rfid.uid.uidByte, rfid.uid.size);

    bool tarjetaValida = false;

    // Verificar contra todas las tarjetas autorizadas
    for (byte j = 0; j < NUMERO_TARJETAS; j++) {
        if (rfid.uid.size == sizeof(TARJETAS_AUTORIZADAS[j])) {
            tarjetaValida = true;
            for (byte i = 0; i < rfid.uid.size; i++) {
                if (rfid.uid.uidByte[i] != TARJETAS_AUTORIZADAS[j][i]) {
                    tarjetaValida = false;
                    break;
                }
            }
            
            if (tarjetaValida) {
                break;
            }
        }
    }

    if (tarjetaValida) {
        Serial.println("Estado: Tarjeta AUTORIZADA ✓");
        restablecerContadores();
    } else {
        Serial.println("Estado: Tarjeta NO autorizada ✗");
        manejarIntentoFallido(false);
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    
    return tarjetaValida;
}

// Verificar huella digital
bool verificarHuella() {
    uint8_t p = huella.getImage();
    if (p != FINGERPRINT_NOFINGER) {
        if (p != FINGERPRINT_OK) {
            Serial.println("\nError al capturar huella");
            manejarIntentoFallido(true);
            return false;
        }

        p = huella.image2Tz();
        if (p != FINGERPRINT_OK) {
            Serial.println("Error: Huella de baja calidad");
            manejarIntentoFallido(true);
            return false;
        }

        p = huella.fingerFastSearch();
        if (p != FINGERPRINT_OK) {
            Serial.println("Huella NO autorizada ✗");
            manejarIntentoFallido(true);
            return false;
        }

        Serial.println("\n=== Huella Detectada ===");
        Serial.println("Estado: Huella AUTORIZADA ✓");
        Serial.print("ID #");
        Serial.println(huella.fingerID);
        restablecerContadores();
        return true;
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println("\nIniciando sistema...");

    inicializarReles();
    inicializarRFID();
    inicializarSensores();

        if (!inicializarHuella()) {
        Serial.println("ERROR: Sistema detenido");
        while (1) {
            digitalWrite(R_ENCENDIDO, !digitalRead(R_ENCENDIDO));
            delay(500);
        }
    }

  // Intentar conexión a Telegram
    if (!conectarTelegram()) {
        modoOffline = true;
        Serial.println("Sistema funcionando en modo offline");
    } else {
        inicializarTeclado();
        Serial.println("Sistema funcionando en modo online con Telegram");
    }
    
    Serial.println("Sistema listo");
}

void loop() {
  
  unsigned long currentMillis = millis();
    // Verificar mensajes de Telegram cada 100ms
    if (!modoOffline && currentMillis - lastBotCheck >= BOT_CHECK_INTERVAL) {
        TBMessage msg;
        if (miBot.getNewMessage(msg)) {
            if (msg.messageType == CTBotMessageText) {
                Serial << "Mensaje: " << msg.sender.firstName << " - " << msg.text << "\n";
                
                if (msg.text.equalsIgnoreCase("/start") ||
                    msg.text.equalsIgnoreCase("/comenzar") || 
                    msg.text.equalsIgnoreCase("menu") || 
                    msg.text.equalsIgnoreCase("opciones")) {
                    enviarMenuPrincipal(msg.sender.id);
                } else {
                    miBot.sendMessage(msg.sender.id, "Usa /comenzar o 'menu' para ver las opciones disponibles");
                }
            }
            else if (msg.messageType == CTBotMessageQuery) {
                Serial << "Acción seleccionada por " << msg.sender.firstName << ": " << msg.callbackQueryData << "\n";
                
                if (msg.callbackQueryData.equals("encender")) {
                    if (!sistemaActivado) {
                        activarReles(msg.sender.id);
                        miBot.endQuery(msg.callbackQueryID, "✅ Sistema activado exitosamente", true);
                    } else {
                        miBot.endQuery(msg.callbackQueryID, "⚠️ El sistema ya está activado", true);
                    }
                }
                else if (msg.callbackQueryData.equals("apagar")) {
                    if (sistemaActivado) {
                        desactivarReles(msg.sender.id);
                        miBot.endQuery(msg.callbackQueryID, "✅ Sistema desactivado exitosamente", true);
                    } else {
                        miBot.endQuery(msg.callbackQueryID, "⚠️ El sistema ya está desactivado", true);
                    }
                }
                else if (msg.callbackQueryData.equals("estado")) {
                    String estado = sistemaActivado ? "ACTIVADO ✅" : "DESACTIVADO ❌";
                    String mensaje = "Estado del sistema: " + estado + "\n";
                    mensaje += "Intentos fallidos RFID: " + String(intentosFallidosRFID) + "\n";
                    mensaje += "Intentos fallidos Huella: " + String(intentosFallidosHuella);
                    miBot.endQuery(msg.callbackQueryID, mensaje, true);
                }
                else if (msg.callbackQueryData.equals("stats")) {
                    String stats = "📊 Estadísticas del Sistema\n\n";
                    stats += "• Tiempo activo: " + String(millis() / 1000 / 60) + " minutos\n";
                    stats += "• Estado: " + String(sistemaActivado ? "Activado" : "Desactivado") + "\n";
                    stats += "• Alarma: " + String(alarmaActiva ? "Desctiva" : "Activada") + "\n";
                    miBot.endQuery(msg.callbackQueryID, stats, true);
                    
                    enviarMenuPrincipal(msg.sender.id);
                }
            }
        }
        lastBotCheck = currentMillis;
    }

    if (!alarmaActiva) {
        verificarVibracion();
        
        if (verificarRFID() || verificarHuella()) {
            Serial.println("¡Acceso autorizado!");
            activarReles();
            delay(1000);
        }
    }
    
    // Desactivar sistema después de un tiempo
    if (sistemaActivado && (millis() - ultimaLectura > 60000)) { // 1 minuto
        sistemaActivado = false;
    }
    
    delay(10);
}