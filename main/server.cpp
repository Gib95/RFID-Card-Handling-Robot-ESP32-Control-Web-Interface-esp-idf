/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string>
#include <sstream>
#include <iostream>
#include <charconv>  // Para std::from_chars
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"
#include <algorithm>  // Para std::transform

#include "server.h"
#include "esp_mac.h"

QueueHandle_t message_queue = NULL;  // Definición de la variable

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)
#define InterfaceControl "InterfaceControl"

#define MESSAGE         "Haz clic en un boton para posicionar una tarjeta."
#define ERROR_MESSAGE   "Debe quitar primero la tarjeta que esta posicionada"

#define MemoryKeyModoAutomatico "modoautomatico"

#define MemoryKeyBoton           "boton"
#define MemoryKeyVelocidad       "velocidad"
#define MemoryKeyTextButtonONOFF "nombreboton"
#define MemoryKeyTextButtonON    "nombreonboton"
#define MemoryKeyTextButtonOFF   "nombreoffboton"

#define MODO_AUTOMATICO_ON 0x00FF
#define MODO_AUTOMATICO_OFF 0x00FE
#define VELOCIDAD_MENSAJE       "Velocidad retardo seteada a "
#define MODO_AUTOMATICO_ON_STR  "Modo Automatico, la tarjeta pasa y sale sola"
#define MODO_AUTOMATICO_OFF_STR "Modo Manual, la tarjeta tiene que pasarse y sacarse manualmente"
#define BACKGROUND_MANUAL       "linear-gradient(135deg, #f1f2f6, #dfe6e9);"
#define BACKGROUND_AUTOMATIC    "linear-gradient(135deg,rgba(115, 187, 255, 0.8),rgba(14, 241, 241, 0.75));"

std::string message = MESSAGE;
std::string messageVelocity = VELOCIDAD_MENSAJE;
std::string messageMode = MODO_AUTOMATICO_OFF_STR;
std::string message_color = "color: green;";
std::string background_color =  BACKGROUND_MANUAL;
/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */
const char* HTTPServer = "HTTPServer";
bool automaticMode = false;

typedef struct {
    std::string color;           
    std::string label_off;     
    std::string label_on;      
    std::string state;        
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

std::string button_texts[NUM_BUTTONS];
std::string button_color[NUM_BUTTONS];
std::string button_state[NUM_BUTTONS];
std::string button_next_state[NUM_BUTTONS];


void save_button_name(std::string buttonKey, const std::string button_name) {
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &err);
    ESP_LOGW(HTTPServer, "Escribiendo valor para clave %s", button_name.c_str());
    err = handle->set_string(buttonKey.c_str(), button_name.c_str());
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

void save_int_value(const std::string valueKey, uint16_t value) {
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &err);
    ESP_LOGW(HTTPServer, "Escribiendo valor para clave %d", value);
    err = handle->set_item(valueKey.c_str(), value);
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

int load_button_name(const std::string button_name, std::string &button_name_found) {

    esp_err_t err;
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    printf("Buscando valor para clave %s", button_name.c_str());
    // Handle will automatically close when going out of scope or when it's reset.
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &err);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");

        char _button_name_found[64];
        const char* button_name_cstr = button_name.c_str();
        size_t len = sizeof(_button_name_found);
        
        // Obtén la cadena del NVS
        err = handle->get_string(button_name_cstr, _button_name_found, len);
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                ESP_LOGI(HTTPServer, "button_name_found = %s", _button_name_found);
                button_name_found =_button_name_found;
                return 0;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGW(HTTPServer, "The value is not initialized yet!");
                button_name_found = "error";
                return -1;
            default :
                ESP_LOGE(HTTPServer, "Error (%s) reading!", esp_err_to_name(err));
                button_name_found = "error";
                return -1;
            }
    }
    return -1;
}

int load_int_value(const std::string valueKey, uint16_t &value) {

    esp_err_t err;
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    printf("Buscando valor para clave %s", valueKey.c_str());
    // Handle will automatically close when going out of scope or when it's reset.
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &err);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
     
        // Obtén la cadena del NVS
        err = handle->get_item(valueKey.c_str(), value);
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
        "    <div class=\"button-container\">", background_color.c_str(), message_color.c_str(), messageMode.c_str(), messageVelocity.c_str(), message.c_str());

    // Generación de los botones de manera dinámica
    for (int i = 0; i < NUM_BUTTONS; i++) {
        offset += snprintf(response + offset, sizeof(response) - offset,
            "        <form action=\"/InterfaceControl\" method=\"GET\">"
            "            <button name=\"boton%d\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
            "        </form>", 
            i + 1, button_next_state[i].c_str(), button_color[i].c_str(), button_texts[i].c_str());
    }

    snprintf(response + offset, sizeof(response) - offset,  // Cierre del HTML
        "    </div>"
        "</body>"
        "</html>");
    
    valor = 0;
    if (button_next_state[indice] == "ON"){
        button_state[indice] ="OFF";
    }
    else{
        button_state[indice] ="ON";
        valor = indice+1;
    }
    if (move){
        servo_http_handler(valor);
    }

    // Enviar la respuesta HTML al cliente
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void update_button_state(std::string param, int index) {
    ESP_LOGW(HTTPServer, "update_button_state , param %s indice %d", param.c_str(), index);
    if (param == "ON") {
        button_texts[index] = buttons[index].label_off;
        button_color[index] = "rgb(69, 157, 160)";
        button_next_state[index] = "OFF";
    } else {
        button_texts[index] = buttons[index].label_on;
        button_color[index] = "#45a049";
        button_next_state[index] = "ON";
    }
}

static esp_err_t get_key_value(httpd_req_t *req, std::string &key, std::string &value) {
    std::string uri(req->uri);
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        std::string query = uri.substr(query_pos + 1);

        std::stringstream query_stream(query);
        std::string param;

        while (std::getline(query_stream, param, '&')) {
            size_t equal_pos = param.find('=');
            if (equal_pos != std::string::npos) {
                // Separar key y value
                key = param.substr(0, equal_pos);
                value = param.substr(equal_pos + 1);
                ESP_LOGI("Query", "Variable: %s, Valor: %s", key.c_str(), value.c_str());
            }
        }
    }
    return ESP_OK;
}

int control_botones(const std::string& param, int numero_boton) {
    ESP_LOGI(HTTPServer, "obtener_numero_boton = %d", numero_boton + 1);
    ESP_LOGI(HTTPServer, "boton%d state %s", numero_boton + 1 , button_state[numero_boton].c_str());
    message_color = "color: green;";
    message = MESSAGE;
    if(!automaticMode){
        if (param == "ON") {
            // Verificar si ya hay un botón encendido
            for (int j = 0; j < NUM_BUTTONS; j++) {
                if (j == numero_boton) continue;  // Si es el mismo botón, no hacer nada
                if (button_state[j] == "ON") {
                    message = ERROR_MESSAGE;
                    message_color = "color: red;";
                    return 1;
                }
            }
        }
    }

    update_button_state(param.c_str(), numero_boton);  // Necesitamos pasarlo como C string
    return 0;
}

int es_digito(char c) {
    return c >= '0' && c <= '9';
}

void url_decode(std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            // Convertir caracteres hexadecimales a ASCII
            char hex[3] = { str[i + 1], str[i + 2], '\0' };
            result += static_cast<char>(std::strtol(hex, nullptr, 16));
            i += 2; // Saltar los dos caracteres hexadecimales
        } else if (str[i] == '+') {
            // Los espacios son codificados como '+'
            result += ' ';
        } else {
            result += str[i];  // Caracter normal
        }
    }
    str = result;
} 

void obtenerPalabraYNumero(const std::string& str, std::string& palabra, int & numero) {
    // Buscar el último dígito que comienza un número
    size_t num_pos = str.find_last_of("0123456789");

    if (num_pos == std::string::npos) {
        palabra = str;
        numero = -1;
        return;
    }

    size_t start_pos = num_pos;
    while (start_pos > 0 && (isdigit(str[start_pos - 1]) || str[start_pos - 1] == '.')) {
        --start_pos;
    }

    palabra = str.substr(0, start_pos);

    std::string num_str = str.substr(start_pos);

    auto [ptr, ec] = std::from_chars(num_str.data(), num_str.data() + num_str.size(), numero);
    if (ec != std::errc()) {
        numero = -1;
    }
}

void convertirAMinusculas(std::string &str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

bool contieneSoloNumerosYCaracteres(const std::string& str) {
    // Recorremos el string y verificamos que no tenga letras
    for (char c : str) {
        if (std::isalpha(c)) {  // Si es una letra
            return false;  // No es permitido
        }
        if (!std::isalnum(c)) {  // Si no es alfanumérico
            return false;  // No es permitido
        }
    }
    return true;  // Solo tiene caracteres alfanuméricos y no tiene letras
}
static esp_err_t any_handler(httpd_req_t *req) {
    ESP_LOGW(HTTPServer, "Found URL query => %s", req->uri);
    bool move = false;
    std::string claveFiltro;
    std::string clave;  
    int numclave = 0;
    std::string valor; 
    std::string ButtonKey = "";
    background_color = BACKGROUND_MANUAL;
    if(automaticMode){
        background_color = BACKGROUND_AUTOMATIC;
    }
    get_key_value(req, clave, valor);
    url_decode(valor);
    obtenerPalabraYNumero(clave, claveFiltro, numclave);
    convertirAMinusculas(clave);
    convertirAMinusculas(claveFiltro);
    ESP_LOGW(HTTPServer, "claveFiltro => %s", claveFiltro.c_str());
    if (numclave!=-1){
        std::string num_clave_str = std::to_string(numclave);
        if (claveFiltro == MemoryKeyBoton) {
            if ((valor == "ON") || (valor == "OFF")){
                if(control_botones(valor, numclave - 1)==0){
                    move = true;
                }
            }
            else{
                ESP_LOGE(HTTPServer, " el parametro no es valido => %s", valor.c_str());
                message_color = "color: red;";
                message = valor + " no es un parametro valido";
            }
        }

        else if (claveFiltro == MemoryKeyTextButtonON)   {
            buttons[numclave - 1].label_on = valor;
            ESP_LOGW(HTTPServer, "nombre ON del boton%d cambiado a => %s", numclave, buttons[numclave - 1].label_on.c_str());
            message = "Nombre de Pasar boton" + num_clave_str + " cambiado a Pasar " + valor;
            ButtonKey = MemoryKeyTextButtonON +  num_clave_str;
            save_button_name(ButtonKey, valor);
            button_texts[numclave - 1] = valor;
        } 
        else if (claveFiltro == MemoryKeyTextButtonOFF) {
            buttons[numclave - 1].label_off = valor;
            ESP_LOGW(HTTPServer, "nombre OFF del boton%d cambiado a => %s", numclave, buttons[numclave - 1].label_off.c_str());
            message = "Nombre de Sacar boton" + num_clave_str + " cambiado a Pasar " + valor;
            ButtonKey = MemoryKeyTextButtonOFF + num_clave_str;
            save_button_name(ButtonKey, valor);
            button_texts[numclave - 1] = valor;
        } 
        else if (claveFiltro == MemoryKeyTextButtonONOFF) {
            buttons[numclave - 1].label_off = "Sacar " + valor;
            buttons[numclave - 1].label_on = "Pasar " + valor;
            ESP_LOGW(HTTPServer, "nombre del boton%d off cambiado a => %s", numclave, buttons[numclave - 1].label_off.c_str());
            ESP_LOGW(HTTPServer, "nombre del boton%d on cambiado a => %s", numclave, buttons[numclave - 1].label_on.c_str());
            message = "Nombre del boton " + num_clave_str + " cambiado a " + valor;
            ButtonKey = MemoryKeyTextButtonON + num_clave_str;
            save_button_name(ButtonKey, buttons[numclave - 1].label_on);
            ButtonKey = MemoryKeyTextButtonOFF + num_clave_str;
            save_button_name(ButtonKey, buttons[numclave - 1].label_off);
            button_texts[numclave - 1] = valor;
        } 
        else{
            ESP_LOGE(HTTPServer, " el parametro no es valido => %s", claveFiltro.c_str());
            message_color = "color: red;";
            message = valor + " no es un parametro valido";
            ESP_LOGW(HTTPServer, "%s", message.c_str());
        }
        
    }
    else if (clave == MemoryKeyVelocidad) {
        obtenerPalabraYNumero(valor, claveFiltro, numclave);
        if(contieneSoloNumerosYCaracteres(valor)){
            if (numclave!=-1)
            {
                uint16_t parametro_velocidad = 0;  
                parametro_velocidad |= (1 << 15);  // Establece el bit más significativo a 1
                ESP_LOGI(HTTPServer, "MASCARA 2 BYTES CON BIT MAS SIGNIFICATIVO A 1 %d", parametro_velocidad);
                parametro_velocidad += static_cast<uint16_t>(std::stoi(valor));
                ESP_LOGI(HTTPServer, "suma de la velocidad de entrada %d", parametro_velocidad);
                message_color = "color: rgb(145, 146, 53);";
                messageVelocity = VELOCIDAD_MENSAJE + valor;
                save_int_value(MemoryKeyVelocidad, parametro_velocidad);
                servo_http_handler(parametro_velocidad);
            }
            else{
                ESP_LOGE(HTTPServer, " el parametro tiene que tener un numero => %s", claveFiltro.c_str());
                message = valor + " no es un parametro valido";
                message_color = "color: red;";
                ESP_LOGW(HTTPServer, "%s", message.c_str());
            }
        }
        else{
            ESP_LOGE(HTTPServer, " el parametro tiene que tener un numero => %s", claveFiltro.c_str());
            message = valor + " no es un parametro valido";
            message_color = "color: red;";
            ESP_LOGW(HTTPServer, "%s", message.c_str());
        }
    }
    else if(claveFiltro == MemoryKeyModoAutomatico){
        if(valor == "ON") {
            save_int_value(MemoryKeyModoAutomatico, 1);
            message = "En modo automatico";
            automaticMode = true;
            messageMode = MODO_AUTOMATICO_ON_STR;
            servo_http_handler(MODO_AUTOMATICO_ON);
            background_color =  BACKGROUND_AUTOMATIC;

        }            
        else if(valor == "OFF") {
            message = "Fuera de modo automatico";
            save_int_value(MemoryKeyModoAutomatico, 0);
            automaticMode = false;
            messageMode = MODO_AUTOMATICO_OFF_STR;
            servo_http_handler(MODO_AUTOMATICO_OFF);
            background_color =  BACKGROUND_MANUAL;

        }
        else{
            ESP_LOGE(HTTPServer, " el valor tiene que ser ON o OFF => %s", valor.c_str());
            message = valor + " no es un parametro valido";
            message_color = "color: red;";
            ESP_LOGW(HTTPServer, "%s", message.c_str());
        }
    }
    else{
        ESP_LOGE(HTTPServer, " el parametro tiene que tener un numero => %s", claveFiltro.c_str());
        message = clave + " no es un parametro valido";
        message_color = "color: red;";
        ESP_LOGW(HTTPServer, "%s", message.c_str());
    }
    refresh_web(req, numclave-1, move);
    if (automaticMode){
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

    std::string button_name_memoryOn = "";
    std::string button_name_memoryOff = "";
    uint16_t velocidad;
    uint16_t modo;

    if(load_int_value(MemoryKeyVelocidad, velocidad)==0){
        servo_http_handler(velocidad);
        messageVelocity = VELOCIDAD_MENSAJE + std::to_string(velocidad & 0x00FF);
    }
    else{
        messageVelocity = "";
    }
    if(load_int_value(MemoryKeyModoAutomatico, modo)==0){
        if (modo){
            message = "En modo automatico";
            automaticMode = true;
            messageMode = MODO_AUTOMATICO_ON_STR;
            servo_http_handler(MODO_AUTOMATICO_ON);
        }            
        else{
            message = "Fuera de modo automatico";
            messageMode = MODO_AUTOMATICO_OFF_STR;
            servo_http_handler(MODO_AUTOMATICO_OFF);
        }
    }
    for (int i = 0; i < NUM_BUTTONS; i++) {
        std::string botonON = MemoryKeyTextButtonON;
        std::string botonOFF = MemoryKeyTextButtonOFF;
        botonON += std::to_string(i + 1);
        botonOFF += std::to_string(i + 1);
        if(load_button_name(botonON, button_name_memoryOn)==0){
            if (button_name_memoryOn != "error"){
                buttons[i].label_on = button_name_memoryOn;
            }        
        }

        if(load_button_name(botonOFF, button_name_memoryOff)==0){
            if (button_name_memoryOff != "error"){
                buttons[i].label_off = button_name_memoryOff;
            }
        }
    }
    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_state[i] = "OFF";  
        button_next_state[i] = "ON";  
        button_texts[i] = buttons[i].label_on;
        button_color[i] = buttons[i].color;
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
#endif // !CONFIG_IDF_TARGET_LINUX

void start_server_task(void *pvParameters)
{
    static httpd_handle_t server = NULL;
    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
#if !CONFIG_IDF_TARGET_LINUX
#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_WIFI
#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_ETHERNET
#endif // !CONFIG_IDF_TARGET_LINUX

    /* Start the server for the first time */
    server = start_webserver();

    while (server) {
        sleep(5);
    }
}


