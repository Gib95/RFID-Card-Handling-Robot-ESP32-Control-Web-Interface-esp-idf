/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <charconv>  // Para std::from_chars
#include <sstream>
#include <iostream>
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"

#include "server.h"
#include "esp_mac.h"

QueueHandle_t message_queue = NULL;  // Definición de la variable

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)
#define InterfaceControl "InterfaceControl"

#define MESSAGE         "Haz clic en un boton para posicionar una tarjeta."
#define ERROR_MESSAGE   "Debe quitar primero la tarjeta que esta posicionada"

#define MemoryKeyModoAutomatico "modoautomatico"

#define MemoryKeyBoton           "boton%d"
#define MemoryKeyVelocidad       "velocidad"
#define MemoryKeyTextButtonONOFF "nombreboton%d"
#define MemoryKeyTextButtonON    "nombreonboton%d"
#define MemoryKeyTextButtonOFF   "nombreoffboton%d"

#define MemoryQueryBoton           "boton"
#define MemoryQueryVelocidad       "velocidad"
#define MemoryQueryTextButtonONOFF "nombreboton"
#define MemoryQueryTextButtonON    "nombreonboton"
#define MemoryQueryTextButtonOFF   "nombreoffboton"


#define MODO_AUTOMATICO_ON 0x00FF
#define MODO_AUTOMATICO_OFF 0x00FE
#define VELOCIDAD_MENSAJE       "Velocidad retardo seteada a %d"
#define MODO_AUTOMATICO_ON_STR  "Modo Automatico, la tarjeta pasa y sale sola"
#define MODO_AUTOMATICO_OFF_STR "Modo Manual, la tarjeta tiene que pasarse y sacarse manualmente"
#define BACKGROUND_MANUAL       "linear-gradient(135deg, #f1f2f6, #dfe6e9);"
#define BACKGROUND_AUTOMATIC    "linear-gradient(135deg,rgba(255, 223, 193, 0.72),rgb(252, 245, 182));"


#define MAX_LENGTH_VALUE 64
#define MAX_BACKGROUND_VALUE 128
#define MAX_QUERY_VALUE 16



char message[MAX_LENGTH_VALUE] = "Velocidad retardo seteada a %d"; // Aquí es donde se copia el literal
char messageVelocity[MAX_LENGTH_VALUE] = "Velocidad retardo seteada a %d";
char messageMode[MAX_LENGTH_VALUE] = MODO_AUTOMATICO_OFF_STR;
char message_color[MAX_LENGTH_VALUE] = "color: green;";
char background_color[MAX_BACKGROUND_VALUE] =  BACKGROUND_MANUAL;
/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */
const char* HTTPServer = "HTTPServer";
bool automaticMode = false;

typedef struct {
    char color[20];           
    char label_off[MAX_LENGTH_VALUE];     
    char label_on[MAX_LENGTH_VALUE];      
    char state[5];        
} Button;
Button buttons[] = {
    {"#4CAF50", "Sacar Tarjeta 1", "Pasar Tarjeta 1", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 2", "Pasar Tarjeta 2", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 3", "Pasar Tarjeta 3", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 4", "Pasar Tarjeta 4", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 5", "Pasar Tarjeta 5", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 6", "Pasar Tarjeta 6", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 7", "Pasar Tarjeta 7", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 8", "Pasar Tarjeta 8", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 9", "Pasar Tarjeta 9", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 10", "Pasar Tarjeta 10", "OFF"}
};

typedef enum {
    MOTOR_0 = 0x0001,  // Motor 1 -> Bit 0 (1)
    MOTOR_1 = 0x0001,  // Motor 1 -> Bit 0 (1)
    MOTOR_2 = 0x0002,  // Motor 2 -> Bit 1 (2)
    MOTOR_3 = 0x0004,  // Motor 3 -> Bit 2 (4)
    MOTOR_4 = 0x0008,  // Motor 4 -> Bit 3 (8)
    MOTOR_5 = 0x0010,  // Motor 5 -> Bit 4 (16)
    MOTOR_6 = 0x0020,  // Motor 6 -> Bit 5 (32)
    MOTOR_7 = 0x0040,  // Motor 7 -> Bit 6 (64)
    MOTOR_8 = 0x0080,  // Motor 8 -> Bit 7 (128)
    MOTOR_9 = 0x0100,  // Motor 9 -> Bit 8 (256)
    MOTOR_10 = 0x0200  // Motor 10 -> Bit 9 (512)
} MotorBit;

// Array que asocia el número del motor con su valor de bit
const uint16_t motorBits[] = {
    MOTOR_0,  // Motor 1 -> 0x0001
    MOTOR_1,  // Motor 1 -> 0x0001
    MOTOR_2,  // Motor 2 -> 0x0002
    MOTOR_3,  // Motor 3 -> 0x0004
    MOTOR_4,  // Motor 4 -> 0x0008
    MOTOR_5,  // Motor 5 -> 0x0010
    MOTOR_6,  // Motor 6 -> 0x0020
    MOTOR_7,  // Motor 7 -> 0x0040
    MOTOR_8,  // Motor 8 -> 0x0080
    MOTOR_9,  // Motor 9 -> 0x0100
    MOTOR_10  // Motor 10 -> 0x0200
};


#define NUM_BUTTONS (sizeof(buttons) / sizeof(buttons[0]))

char button_texts[NUM_BUTTONS][MAX_LENGTH_VALUE];  // Cambié a un arreglo de 2 dimensiones
char button_color[NUM_BUTTONS][MAX_LENGTH_VALUE];
char button_state[NUM_BUTTONS][MAX_LENGTH_VALUE];
char button_next_state[NUM_BUTTONS][MAX_LENGTH_VALUE];


void save_button_name(char * buttonKey, char * button_name) {
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &err);
    ESP_LOGW(HTTPServer, "Escribiendo valor para clave %s", buttonKey);
    err = handle->set_string(buttonKey, button_name);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    printf("Committing updates in NVS ... ");
    err = handle->commit();
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    printf("\n");
    fflush(stdout);
}

void save_int_value(char * valueKey, uint16_t value) {
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &err);
    ESP_LOGW(HTTPServer, "Escribiendo valor para clave %d", value);
    err = handle->set_item(valueKey, value);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    printf("Committing updates in NVS ... ");
    err = handle->commit();
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    printf("\n");
    fflush(stdout);
}

int load_button_name(char* button_name, char * button_name_found) {

    esp_err_t err;
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    printf("Buscando valor para clave %s", button_name);
    // Handle will automatically close when going out of scope or when it's reset.
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &err);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf(" Done\n");

        // Obtén la cadena del NVS
        err = handle->get_string(button_name, button_name_found, MAX_LENGTH_VALUE);
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                ESP_LOGI(HTTPServer, "button_name_found = %s", button_name_found);
                return 0;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGW(HTTPServer, "The value is not initialized yet!");
                strcpy(button_name_found,"error");
                return -1;
            default :
                ESP_LOGE(HTTPServer, "Error (%s) reading!", esp_err_to_name(err));
                strcpy(button_name_found,"error");
                return -1;
            }
    }
    return -1;
}

int load_int_value(char * valueKey, uint16_t &value) {

    esp_err_t err;
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    printf("Buscando valor para clave %s", valueKey);
    // Handle will automatically close when going out of scope or when it's reset.
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &err);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
     
        // Obtén la cadena del NVS
        err = handle->get_item(valueKey, value);
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                ESP_LOGI(HTTPServer, "value found = %d", value);
                return 0;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGW(HTTPServer, "The value is not initialized yet!");
                return -1;
            default :
                ESP_LOGE(HTTPServer, "Error (%s) reading!", esp_err_to_name(err));
                return -1;
            }
    }
    return -1;
}

static esp_err_t servo_http_handler(uint16_t num_motor)
{
    ESP_LOGW(HTTPServer, "servo_http_handler num motor %d ",num_motor);
    if ((num_motor==0) || (num_motor & 0x8000))
    {
        xQueueSend(message_queue, &num_motor, portMAX_DELAY);
        return ESP_OK;
    }
    else if((num_motor == MODO_AUTOMATICO_ON) || (num_motor == MODO_AUTOMATICO_OFF)){
        xQueueSend(message_queue, &num_motor, portMAX_DELAY);
        return ESP_OK;
    }
    if( NUM_BUTTONS<num_motor){
        ESP_LOGE(HTTPServer, "Posicion num motor %d no esta dentro de la lista",num_motor);
        return ESP_OK;

    }
    xQueueSend(message_queue, &motorBits[num_motor], portMAX_DELAY);
    return ESP_OK;
}

esp_err_t refresh_web(httpd_req_t *req, int indice, bool move){
    char response[2048*2];
    uint16_t valor = 0;
    int offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
        "<!DOCTYPE html>"
        "<html lang=\"es\">"
        "<head>"
        "    <meta charset=\"UTF-8\">"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "    <title>Botones Interactivos</title>"
        "    <style>"
        "        body {"
        "            font-family: 'Arial', sans-serif;"
        "            text-align: center;"
        "            padding: 50px;"
        "            background: %s"
        "            margin: 0;"
        "            height: 100vh;"
        "        }"
        "        h1 {"
        "            color: #2d3436;"
        "            font-size: 3em;"
        "            margin-bottom: 20px;"
        "        }"
        "        p {"
        "            %s"
        "            font-size: 1.2em;"
        "            margin-bottom: 20px;"
        "        }"
        "        .button-container {"
        "            display: flex;"
        "            justify-content: center;"
        "            gap: 20px;"
        "            flex-wrap: wrap;"
        "        }"
        "        button {"
        "            background-color: #4CAF50; /* Color verde suave */"
        "            color: white;"
        "            font-size: 18px;"
        "            padding: 15px 40px;"
        "            border: none;"
        "            border-radius: 8px;"
        "            cursor: pointer;"
        "            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);"
        "            transition: background-color 0.3s, transform 0.1s, box-shadow 0.3s;"
        "            max-width: 220px;"
        "            font-weight: bold;"
        "        }"
        "        button:hover {"
        "            background-color: #45a049;"
        "            transform: translateY(-2px);"
        "            box-shadow: 0 8px 12px rgba(0, 0, 0, 0.2);"
        "        }"
        "        button:active {"
        "            background-color: #388e3c;"
        "            transform: translateY(2px);"
        "            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.15);"
        "        }"
        "    </style>"
        "</head>"
        "<body>"
        "    <h1>Interfaz de Tarjetas</h1>"
        "    <p>%s</p>"
        "    <p>%s</p>"
        "    <p>%s</p>"
        "    <div class=\"button-container\">", background_color, message_color, messageMode, messageVelocity, message);

    // Generación de los botones de manera dinámica
    for (int i = 0; i < NUM_BUTTONS; i++) {
        offset += snprintf(response + offset, sizeof(response) - offset,
            "        <form action=\"/InterfaceControl\" method=\"GET\">"
            "            <button name=\"boton%d\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
            "        </form>", 
            i + 1, button_next_state[i], button_color[i], button_texts[i]);
    }

    snprintf(response + offset, sizeof(response) - offset,  // Cierre del HTML
        "    </div>"
        "</body>"
        "</html>");
    
    valor = 0;
    if (strcmp(button_next_state[indice], "ON") == 0){
        strcpy(button_state[indice],"OFF");
    }
    else{
        strcpy(button_state[indice],"ON");
        valor = indice+1;
    }
    if (move){
        servo_http_handler(valor);
    }

    // Enviar la respuesta HTML al cliente
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void update_button_state(char * param, int index) {
    // ESP_LOGW(HTTPServer, "update_button_state , param %s indice %d", param, index);
    
    if (strcmp(param, "ON") == 0) {
        // Asignamos valores a los arreglos de char[] usando strcpy
        strcpy(button_texts[index], buttons[index].label_off);
        strcpy(button_color[index], "rgb(69, 157, 160)");
        strcpy(button_next_state[index], "OFF");
    } else {
        strcpy(button_texts[index], buttons[index].label_on);
        strcpy(button_color[index], "#45a049");
        strcpy(button_next_state[index], "ON");
    }
}

static esp_err_t get_key_value(httpd_req_t *req, char *key, char *value) {
    // Obtener la URI del request
    const char *uri = req->uri;

    // Buscar el carácter '?' que marca el comienzo de los parámetros en la URI
    char *query_pos = strchr(uri, '?');
    if (query_pos != nullptr) {
        // El primer parámetro comienza justo después del '?'
        query_pos++;  // Avanzamos un caracter para saltarnos el '?'

        // Usamos strtok para separar los parámetros por '&'
        char *param = strtok(query_pos, "&");
        
        while (param != nullptr) {
            // Buscamos el signo '=' que separa la clave y el valor
            char *equal_pos = strchr(param, '=');
            if (equal_pos != nullptr) {
                // Separamos la clave y el valor
                *equal_pos = '\0';  // Terminamos la clave con '\0'
                strcpy(key, param);  // Copiamos la clave al argumento 'key'
                strcpy(value, equal_pos + 1);  // Copiamos el valor al argumento 'value'
                
                ESP_LOGI("Query", "Variable: %s, Valor: %s", key, value);
            }
            // Continuamos con el siguiente parámetro
            param = strtok(nullptr, "&");
        }
    }

    return ESP_OK;
}

int control_botones(char * param, int numero_boton) {
    ESP_LOGI(HTTPServer, "obtener_numero_boton = %d", numero_boton + 1);
    ESP_LOGI(HTTPServer, "boton%d state %s", numero_boton + 1 , button_state[numero_boton]);
    sprintf(message, MESSAGE);
    sprintf(message_color, "color: green;");
    if(!automaticMode){
        if (strcmp(param,"ON")==0) {
            // Verificar si ya hay un botón encendido
            for (int j = 0; j < NUM_BUTTONS; j++) {
                if (j == numero_boton) continue;  // Si es el mismo botón, no hacer nada
                if (strcmp(button_state[j], "ON")==0) {
                    sprintf(message, ERROR_MESSAGE);
                    sprintf(message_color, "color: red;");
                    return 1;
                }
            }
        }
    }

    update_button_state(param, numero_boton);  // Necesitamos pasarlo como C string
    return 0;
}

// Función para verificar si un carácter es un dígito
int es_digito(char c) {
    return c >= '0' && c <= '9';
}

// Función para realizar el decodificado de una URL (usando char*)
void url_decode(char* str) {
    char result[1024];  // Usamos un buffer para almacenar el resultado
    int j = 0;  // Índice para el resultado
    
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] == '%' && str[i+1] != '\0' && str[i+2] != '\0') {
            // Convertir los dos siguientes caracteres hexadecimales a ASCII
            char hex[3] = {str[i+1], str[i+2], '\0'};
            result[j++] = static_cast<char>(std::strtol(hex, nullptr, 16));
            i += 2;  // Saltamos los dos caracteres hexadecimales
        } else if (str[i] == '+') {
            result[j++] = ' ';  // Convertimos '+' en espacio
        } else {
            result[j++] = str[i];  // Caracter normal
        }
    }
    
    result[j] = '\0';  // Añadimos el terminador de cadena
    strcpy(str, result);  // Copiamos el resultado decodificado de vuelta en str
}

// Función para separar la palabra y el número en una cadena (usando char*)
void obtenerPalabraYNumero(char* str, char* palabra, int &numero) {
    int num_pos = -1;
    
    // Buscamos el último dígito que podría comenzar un número
    for (int i = 0; str[i] != '\0'; ++i) {
        if (es_digito(str[i])) {
            num_pos = i;
            break;  // Encontramos el primer dígito, no seguimos buscando
        }
    }
    
    if (num_pos == -1) {
        // No se encuentra un número, así que toda la cadena es la palabra
        strcpy(palabra, str);
        numero = -1;  // Indicamos que no se encontró un número
        return;
    }

    // Extraemos la parte de la palabra antes del número
    strncpy(palabra, str, num_pos);
    palabra[num_pos] = '\0';  // Terminamos la palabra con '\0'
    
    // Ahora extraemos el número (max 2 dígitos)
    char num_str[3];  // Sólo necesitamos dos caracteres + el '\0'
    int i = 0;
    
    // Copiamos los primeros dos dígitos
    for (int j = num_pos; str[j] != '\0' && i < 2; ++j, ++i) {
        num_str[i] = str[j];
    }
    num_str[i] = '\0';  // Terminamos la cadena numérica

    // Convertimos el número en la cadena numérica (solo dos dígitos)
    if (sscanf(num_str, "%d", &numero) != 1) {
        numero = -1;  // Si no es un número válido
    }
}

// Función para convertir una cadena en minúsculas (usando char*)
void convertirAMinusculas(char* str) {
    for (int i = 0; str[i] != '\0'; ++i) {
        str[i] = static_cast<char>(tolower(str[i]));
    }
}

// Función para verificar si una cadena contiene solo números y caracteres alfanuméricos (sin letras)
bool contieneSoloNumerosYCaracteres(char* str) {
    for (int i = 0; str[i] != '\0'; ++i) {
        if (isalpha(str[i])) {  // Si es una letra, no es válido
            return false;
        }
        if (!isalnum(str[i])) {  // Si no es alfanumérico, no es válido
            return false;
        }
    }
    return true;
}


// Función auxiliar para asignar el mensaje y el color
void set_message(char* msg, char* color) {
    strcpy(message, msg);
    strcpy(message_color, color);
    ESP_LOGW(HTTPServer, "%s", message);
}

// Función para actualizar las etiquetas del botón
void update_button_label(int numclave, char* label, bool is_on) {
    char num_clave_str[MAX_LENGTH_VALUE];
    sprintf(num_clave_str, "%d", numclave);

    if (is_on) {
        strcpy(buttons[numclave - 1].label_on, label);
        strcpy(button_texts[numclave - 1], label);
        ESP_LOGW(HTTPServer, "Nombre ON del boton %d cambiado a => %s", numclave, label);
    } else {
        strcpy(buttons[numclave - 1].label_off, label);
        strcpy(button_texts[numclave - 1], label);
        ESP_LOGW(HTTPServer, "Nombre OFF del boton %d cambiado a => %s", numclave, label);
    }
    
    char ButtonKey[MAX_LENGTH_VALUE];
    sprintf(ButtonKey, is_on ? MemoryKeyTextButtonON : MemoryKeyTextButtonOFF, numclave);
    save_button_name(ButtonKey, label);
}

// Función genérica para manejar cambios de parámetros
void handle_key_value_change(char* claveFiltro, char* valor, int numclave, bool &move) {
   
    if (strcmp(claveFiltro, MemoryQueryBoton) == 0) {
        if (strcmp(valor, "ON") == 0 || strcmp(valor, "OFF") == 0) {
            if (control_botones(valor, numclave - 1) == 0) {
                move = true;
            }
        } else {
            ESP_LOGE(HTTPServer, "El parametro no es valido => %s", valor);
            set_message(valor, "color: red;");
        }
    }
    else if (strcmp(claveFiltro, MemoryQueryTextButtonON) == 0) {
        update_button_label(numclave, valor, true);
        sprintf(message, "Nombre de Pasar boton %d cambiado a Pasar %s", numclave, valor);
        set_message(message, "color: green;");
    } 
    else if (strcmp(claveFiltro, MemoryQueryTextButtonOFF) == 0) {
        update_button_label(numclave, valor, false);
        sprintf(message, "Nombre de Sacar boton %d cambiado a Pasar %s", numclave, valor);
        set_message(message, "color: green;");
    } 
    else if (strcmp(claveFiltro, MemoryQueryTextButtonONOFF) == 0) {
        char full_label[MAX_LENGTH_VALUE];
        sprintf(full_label, "Sacar %s", valor);
        update_button_label(numclave, full_label, false);
        sprintf(full_label, "Pasar %s", valor);
        update_button_label(numclave, full_label, true);

        sprintf(message, "Nombre del boton %d cambiado a %s", numclave, valor);
        set_message(message, "color: green;");
    } 
    else {
        ESP_LOGE(HTTPServer, "El parametro no es valido => %s", claveFiltro);
        set_message(valor, "color: red;");
    }
}

// Función genérica para manejar la velocidad
void handle_velocity_change(char* valor) {
    int numclave = 0;
    char claveFiltro[MAX_LENGTH_VALUE] = {0};
    obtenerPalabraYNumero(valor, claveFiltro, numclave);

    if (contieneSoloNumerosYCaracteres(valor)) {
        if (numclave != -1) {
            uint16_t parametro_velocidad = 0;  
            parametro_velocidad |= (1 << 15);  // Establece el bit más significativo a 1
            parametro_velocidad += static_cast<uint16_t>(atoi(valor));
            sprintf(messageVelocity, VELOCIDAD_MENSAJE, static_cast<uint16_t>(atoi(valor)));
            save_int_value(MemoryKeyVelocidad, parametro_velocidad);
            servo_http_handler(parametro_velocidad);
            set_message(messageVelocity, "color: rgb(145, 146, 53);");
        }
    } else {
        ESP_LOGE(HTTPServer, "El parametro tiene que ser un numero => %s", claveFiltro);
        set_message(valor, "color: red;");
    }
}

// Función genérica para manejar el modo automático
void handle_automatic_mode_change(char* valor) {
    if (strcmp(valor, "ON") == 0) {
        save_int_value(MemoryKeyModoAutomatico, 1);
        automaticMode = true;
        servo_http_handler(MODO_AUTOMATICO_ON);
        strcpy(messageMode, MODO_AUTOMATICO_ON_STR);
        strcpy(background_color, BACKGROUND_AUTOMATIC);
        set_message("En modo automatico", "color: green;");
    }            
    else if (strcmp(valor, "OFF") == 0) {
        save_int_value(MemoryKeyModoAutomatico, 0);
        automaticMode = false;
        servo_http_handler(MODO_AUTOMATICO_OFF);
        strcpy(messageMode, MODO_AUTOMATICO_OFF_STR);
        strcpy(background_color, BACKGROUND_MANUAL);
        set_message("Fuera de modo automatico", "color: green;");
    }
    else {
        ESP_LOGE(HTTPServer, "El valor tiene que ser ON o OFF => %s", valor);
        set_message(valor, "color: red;");
    }
}

static esp_err_t any_handler(httpd_req_t *req) {
    ESP_LOGW(HTTPServer, "Found URL query => %s", req->uri);
    bool move = false;
    char claveFiltro[MAX_LENGTH_VALUE] = {0};
    char clave[MAX_QUERY_VALUE] = {0};  
    int numclave = 0;
    char valor[MAX_QUERY_VALUE] = {0}; 
    strcpy(background_color, BACKGROUND_MANUAL);

    if(automaticMode) {
        strcpy(background_color, BACKGROUND_AUTOMATIC);
    }

    get_key_value(req, clave, valor);
    url_decode(valor);
    obtenerPalabraYNumero(clave, claveFiltro, numclave);
    ESP_LOGI(HTTPServer, "Palabra: %s, Número: %d", claveFiltro, numclave);

    convertirAMinusculas(clave);
    convertirAMinusculas(claveFiltro);
    ESP_LOGW(HTTPServer, "claveFiltro => %s", claveFiltro);

    if (numclave != -1) {
        handle_key_value_change(claveFiltro, valor, numclave, move);
    }
    else if (strcmp(clave, MemoryKeyVelocidad) == 0) {
        handle_velocity_change(valor);
    }
    else if (strcmp(claveFiltro, MemoryKeyModoAutomatico) == 0) {
        handle_automatic_mode_change(valor);
    } else {
        ESP_LOGE(HTTPServer, "El parametro tiene que tener un numero => %s", claveFiltro);
        sprintf(message, "%s no es un parametro valido", clave);
        set_message(message, "color: red;");
    }

    refresh_web(req, numclave - 1, move);

    if (automaticMode) {
        for (int indice_lista = 0; indice_lista < NUM_BUTTONS; indice_lista++) {
            update_button_state("OFF", indice_lista);
        }
    }

    return ESP_OK;
}

static const httpd_uri_t Interface = {
    .uri       = "/InterfaceControl",
    .method    = HTTP_GET,
    .handler   = any_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t index_ = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = any_handler,
    .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    // Definir el tamaño de la pila para la tarea httpd
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192; 

    char button_name_memoryOn[MAX_LENGTH_VALUE] = {0};
    char button_name_memoryOff[MAX_LENGTH_VALUE] = {0};
    uint16_t velocidad;
    uint16_t modo;

    if(load_int_value(MemoryKeyVelocidad, velocidad)==0){
        servo_http_handler(velocidad);
        sprintf(messageVelocity, VELOCIDAD_MENSAJE, (velocidad & 0x00FF));
    }
    else{
        strcpy(messageVelocity, " ");
    }
    if(load_int_value(MemoryKeyModoAutomatico, modo)==0){
        if (modo){
            strcpy(message, "En modo automatico");
            strcpy(messageMode, MODO_AUTOMATICO_ON_STR);
            automaticMode = true;
            servo_http_handler(MODO_AUTOMATICO_ON);
        }            
        else{
            strcpy(message, "Fuera de modo automatico");
            strcpy(messageMode, MODO_AUTOMATICO_OFF_STR);
            servo_http_handler(MODO_AUTOMATICO_OFF);
        }
    }
    for (int i = 0; i < NUM_BUTTONS; i++) {
        char botonON[20];
        char botonOFF[20];
        sprintf(botonON, MemoryKeyTextButtonON, (i + 1));
        sprintf(botonOFF, MemoryKeyTextButtonOFF, (i + 1));
        if(load_button_name(botonON, button_name_memoryOn)==0){
            if (strcmp(button_name_memoryOn, "error") !=0){
                strcpy(buttons[i].label_on, button_name_memoryOn);
            }        
        }

        if(load_button_name(botonOFF, button_name_memoryOff)==0){
            if (strcmp(button_name_memoryOff, "error") !=0){
                strcpy(buttons[i].label_off, button_name_memoryOff);
            }
        }
    }
    for (int i = 0; i < NUM_BUTTONS; i++) {
        strcpy(button_state[i], "OFF");
        strcpy(button_next_state[i], "ON");
        strcpy(button_texts[i], buttons[i].label_on);
        strcpy(button_color[i], buttons[i].color);
    }

#if CONFIG_IDF_TARGET_LINUX
    // Setting port as 8001 when building for Linux. Port 80 can be used only by a privileged user in linux.
    // So when a unprivileged user tries to run the application, it throws bind error and the server is not started.
    // Port 8001 can be used by an unprivileged user as well. So the application will not throw bind error and the
    // server will be started.
    config.server_port = 8001;
#endif // !CONFIG_IDF_TARGET_LINUX
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(HTTPServer, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(HTTPServer, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_);
        httpd_register_uri_handler(server, &Interface);
        return server;
    }

    ESP_LOGI(HTTPServer, "Error starting server!");
    return NULL;
}

#if !CONFIG_IDF_TARGET_LINUX
static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(HTTPServer, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(HTTPServer, "Failed to stop http server");
        }
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(HTTPServer, "Starting webserver");
        *server = start_webserver();
    }
}
#endif     

void start_server_task(void *pvParameters)
{
    static httpd_handle_t server = NULL;
    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    server = start_webserver();

    while (server) {
        sleep(5);
    }
}


