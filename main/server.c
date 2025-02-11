/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include "server.h"
#include "URIHandler.h"
QueueHandle_t message_queue = NULL;  // Definición de la variable

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)
#define InterfaceControl "InterfaceControl"

#define MESSAGE "Haz clic en un boton para cambiar su texto."
#define ERROR_MESSAGE "Debe quitar primero la tarjeta que esta posicionada"
char * message = MESSAGE;

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */
const char* HTTPServer = "HTTPServer";

typedef struct {
    const char* color;                   // Puntero a la variable de estado que cambia
    const char* label_off;         // Texto cuando el botón está desactivado
    const char* label_on;          // Texto cuando el botón está activado
    const char* state;          // Texto cuando el botón está activado
} Button;

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

Button buttons[] = {
    {"#4CAF50", "Sacar Tarjeta 1", "Pasar Tarjeta 1", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 2", "Pasar Tarjeta 2", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 3", "Pasar Tarjeta 3", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 4", "Pasar Tarjeta 4", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 5", "Pasar Tarjeta 5", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 6", "Pasar Tarjeta 6", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 7", "Pasar Tarjeta 7", "OFF"},
    {"#4CAF50", "Sacar Tarjeta 8", "Pasar Tarjeta 8", "OFF"}
};

#define NUM_BUTTONS (sizeof(buttons) / sizeof(buttons[0]))

const char* button_texts[NUM_BUTTONS];
const char* button_color[NUM_BUTTONS];
const char* button_state[NUM_BUTTONS];
const char* button_next_state[NUM_BUTTONS];

bool no_actualizar = true;

static esp_err_t servo_http_handler(uint16_t num_motor)
{
    ESP_LOGW(HTTPServer, "servo_http_handler ");
    if (num_motor==0)
    {
        xQueueSend(message_queue, &num_motor, portMAX_DELAY);
        return ESP_OK;
    }
    xQueueSend(message_queue, &motorBits[num_motor], portMAX_DELAY);

    return ESP_OK;
}

esp_err_t refresh_web(httpd_req_t *req, bool no_actualizar, int indice){
    char response[2048*2];
    uint16_t valor = 0;
    snprintf(response, sizeof(response),
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
        "            background: linear-gradient(135deg, #f1f2f6, #dfe6e9);"
        "            margin: 0;"
        "            height: 100vh;"
        "        }"
        "        h1 {"
        "            color: #2d3436;"
        "            font-size: 3em;"
        "            margin-bottom: 20px;"
        "        }"
        "        p {"
        "            color: #636e72;"
        "            font-size: 1.2em;"
        "            margin-bottom: 40px;"
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
        "            background-color: #45a049;"  /* Color ligeramente más oscuro al pasar el ratón */
        "            transform: translateY(-2px); /* Efecto de elevación */"
        "            box-shadow: 0 8px 12px rgba(0, 0, 0, 0.2);"
        "        }"
        "        button:active {"
        "            background-color: #388e3c;"  /* Color más oscuro al presionar */
        "            transform: translateY(2px);" /* Efecto de presionar el botón */
        "            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.15);"
        "        }"
        "    </style>"
        "</head>"
        "<body>"
        "    <h1>Interfaz de Botones</h1>"
        "    <p>\"%s\"</p>"
        "    <div class=\"button-container\">"
        "        <form action=\"/InterfaceControl\" method=\"GET\">"
        "            <!-- Botón 1 que envía 'boton=0' al servidor -->"
        "            <button name=\"boton1\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"
        "        <form action=\"/InterfaceControl\" method=\"GET\">"
        "            <!-- Botón 2 que envía 'boton=1' al servidor -->"
        "            <button name=\"boton2\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"
        "        <form action=\"/InterfaceControl\" method=\"GET\">"
        "            <!-- Botón 3 que envía 'boton=2' al servidor -->"
        "            <button name=\"boton3\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"      
        "        <form action=\"/InterfaceControl\" method=\"GET\">"
        "            <!-- Botón 1 que envía 'boton=0' al servidor -->"
        "            <button name=\"boton4\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"
        "        <form action=\"/InterfaceControl\" method=\"GET\">"
        "            <!-- Botón 2 que envía 'boton=1' al servidor -->"
        "            <button name=\"boton5\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"
        "        <form action=\"/InterfaceControl\" method=\"GET\">"
        "            <!-- Botón 3 que envía 'boton=2' al servidor -->"
        "            <button name=\"boton6\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"       
        "        <form action=\"/InterfaceControl\" method=\"GET\">"
        "            <!-- Botón 1 que envía 'boton=0' al servidor -->"
        "            <button name=\"boton7\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"
        "        <form action=\"/InterfaceControl\" method=\"GET\">"
        "            <!-- Botón 2 que envía 'boton=1' al servidor -->"
        "            <button name=\"boton8\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"
        "    </div>"
        "</body>"
        "</html>",
        message,
        button_next_state[0], button_color[0], button_texts[0], 
        button_next_state[1], button_color[1], button_texts[1], 
        button_next_state[2], button_color[2], button_texts[2],
        button_next_state[3], button_color[3], button_texts[3], 
        button_next_state[4], button_color[4], button_texts[4], 
        button_next_state[5], button_color[5], button_texts[5],
        button_next_state[6], button_color[6], button_texts[6], 
        button_next_state[7], button_color[7], button_texts[7]);
    
    valor = 0;
    if (!no_actualizar){
        char query_name[26];  // Nombre del parámetro como "boton1", "boton2", etc.
        if (strcmp(button_next_state[indice], "ON") == 0) {
            button_state[indice] ="OFF";
        }
        else{
            button_state[indice] ="ON";
            valor = indice+1;
        }
        ESP_LOGW(HTTPServer, "Actualizando indice de tarjeta %d", indice);
        ESP_LOGI(HTTPServer, "button_state[j] %s , button_next_state indice %s ",button_state[indice], button_next_state[indice]);
        ESP_LOGI(HTTPServer, "%d", indice);
    }
    servo_http_handler(valor);

    // Enviar la respuesta HTML al cliente
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void update_button_state(const char* param, int index) {
    ESP_LOGW(HTTPServer, "update_button_state , param %s", param);
    if (strcmp(param, "ON") == 0) {
            button_texts[index] = buttons[index].label_off;
            button_color[index] = "rgb(69, 157, 160)";
            button_next_state[index] = "OFF";
    } else {
        button_texts[index] = buttons[index].label_on;
        button_color[index] = "#45a049";
        button_next_state[index] = "ON";
    }
}

static esp_err_t any_handler(httpd_req_t *req) {
    char* buf;
    size_t buf_len;
    ESP_LOGW(HTTPServer, "Found URL query => %s", req->uri);
    int indice_boton = 0;
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, HTTPServer, "buffer alloc failed");

        // Obtener la cadena de la URL
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTPServer, "Found URL query => %s", buf);

            // Para cada botón (boton1, boton2, boton3, etc.), actualizamos su estado
            for (int i = 0; i < NUM_BUTTONS; i++) {  // Ajusta el límite del bucle según el número de botones
                char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN], dec_param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};
                char query_name[10];  // Nombre del parámetro como "boton1", "boton2", etc.
                snprintf(query_name, sizeof(query_name), "boton%d", i + 1);
                ESP_LOGI(HTTPServer, "boton%d state %s", i , button_state[i]);

                if (httpd_query_key_value(buf, query_name, param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(HTTPServer, "Found URL query parameter => %s=%s", query_name, param);
                    if (strcmp(param, "ON") == 0) {
                        for (int j = 0; j < NUM_BUTTONS; j++){
                            // ESP_LOGI(HTTPServer, "button_state[j] %s i %d  j %d", button_state[j],i,j);

                            if (j == i) continue;
                            // ESP_LOGI(HTTPServer, "button_state[j] %s i %d  j %d", button_state[j],i,j);

                            if (strcmp(button_state[j], "ON") == 0){
                                // ESP_LOGE(HTTPServer, "ERROR_MESSAGE %s ", ERROR_MESSAGE);

                                message = ERROR_MESSAGE;
                                no_actualizar = true;
                                break;
                            }
                            no_actualizar = false;
                        }
                        if (!no_actualizar){
                            message = MESSAGE;
                            update_button_state(param, i);  // Actualiza el estado del botón usando la función
                            example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                            ESP_LOGW(HTTPServer, "se actualiza el boton => %s", dec_param);
                        }
                    }
                    else{
                        message = MESSAGE;
                        update_button_state(param, i);  // Actualiza el estado del botón usando la función
                        example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                        ESP_LOGW(HTTPServer, "desactivando el boton => %s", dec_param);
                        no_actualizar = false;

                    }
                    break;
                }
                indice_boton++;
            }
        }
        free(buf);
    }
    refresh_web(req, no_actualizar, indice_boton);
    return ESP_OK;
}
static const httpd_uri_t Interface = {
    .uri       = "/InterfaceControl",
    .method    = HTTP_GET,
    .handler   = any_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static const httpd_uri_t index_ = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = any_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL  
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    // Definir el tamaño de la pila para la tarea httpd
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;  // Ajusta este valor según sea necesario, por ejemplo, 8192 bytes
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
