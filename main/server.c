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

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */
const char* HTTPServer = "HTTPServer";

typedef struct {
    const char* color;                   // Puntero a la variable de estado que cambia
    const char* label_off;         // Texto cuando el botón está desactivado
    const char* label_on;          // Texto cuando el botón está activado
} Button;


Button buttons[] = {
    {"#4CAF50", "Sacar Tarjeta 1", "Pasar Tarjeta 1"},
    {"#4CAF50", "Sacar Tarjeta 2", "Pasar Tarjeta 2"},
    {"#4CAF50", "Sacar Tarjeta 3", "Pasar Tarjeta 3"}
};

#define NUM_BUTTONS (sizeof(buttons) / sizeof(buttons[0]))

const char* button_texts[NUM_BUTTONS];
const char* button_color[NUM_BUTTONS];
const char* button_next_state[NUM_BUTTONS];
const char* uri_text[NUM_BUTTONS];

static esp_err_t servo_on_http_handler()
{
    // Definir el mensaje por defecto
    const char* move_message = "DEFAULT_MOVE";
    move_message = "MOVE_SERVO1";  // Mensaje para cardOn
    xQueueSend(message_queue, &move_message, portMAX_DELAY);
    return ESP_OK;
}

static esp_err_t servo_off_http_handler(){
    const char* move_message = "DEFAULT_MOVE";
    move_message = "MOVE_SERVO2";  // Mensaje para cardOff
    xQueueSend(message_queue, &move_message, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t any_handler(httpd_req_t *req)
{

    ESP_LOGW(HTTPServer, "URI -> %s", req->uri);
    int posicion = 0;
    char*  buf;
    size_t buf_len;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, HTTPServer, "buffer alloc failed");
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTPServer, "Found URL query => %s", buf);
            char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN], dec_param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "boton1", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTPServer, "Found URL query parameter => boton1=%s", param);
                if (strcmp(param, "1") == 0){
                    button_texts[0] = buttons[0].label_off;
                    button_color[0] = "rgb(69, 157, 160)";
                    button_next_state[0] = "0";

                }
                else{
                    button_texts[0] = buttons[0].label_on;
                    button_color[0] = "#45a049";
                    button_next_state[0] = "1";

                }
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(HTTPServer, "Decoded query parameter => %s", dec_param);
            }
            else if (httpd_query_key_value(buf, "boton2", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTPServer, "Found URL query parameter => boton2=%s", param);

                if (strcmp(param, "1") == 0){
                    button_texts[1] = buttons[1].label_off;
                    button_color[1] = "rgb(69, 157, 160)";
                    button_next_state[1] = "0";
                }
                else{
                    button_texts[1] = buttons[1].label_on;
                    button_color[1] = "#45a049";
                    button_next_state[1] = "1";
                }
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(HTTPServer, "Decoded query parameter => %s", dec_param);
            }
            else if (httpd_query_key_value(buf, "boton3", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTPServer, "Found URL query parameter => boton3=%s", param);

                if (strcmp(param, "1") == 0){
                    button_texts[2] = buttons[2].label_off;
                    button_color[2] = "rgb(69, 157, 160)";
                    button_next_state[2] = "0";

                }
                else{
                    button_texts[2] = buttons[2].label_on;
                    button_color[2] = "#45a049";
                    button_next_state[2] = "1";

                }
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(HTTPServer, "Decoded query parameter => %s", dec_param);
            }
        }
        free(buf);
    }

    char response[2048*2];
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
        "    <p>Haz clic en un botón para cambiar su texto.</p>"
        "    <div class=\"button-container\">"
        "        <form action=\"/cardOn1\" method=\"GET\">"
        "            <!-- Botón 1 que envía 'boton=0' al servidor -->"
        "            <button name=\"boton1\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"
        "        <form action=\"/cardOn1\" method=\"GET\">"
        "            <!-- Botón 2 que envía 'boton=1' al servidor -->"
        "            <button name=\"boton2\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"
        "        <form action=\"/cardOn1\" method=\"GET\">"
        "            <!-- Botón 3 que envía 'boton=2' al servidor -->"
        "            <button name=\"boton3\" value=\"%s\" style=\"background-color:%s;\">%s</button>"
        "        </form>"
        "    </div>"
        "</body>"
        "</html>",

        button_next_state[0], button_color[0], button_texts[0], 
        button_next_state[1], button_color[1], button_texts[1], 
        button_next_state[2], button_color[2], button_texts[2]);

    // Enviar la respuesta HTML al cliente
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}



/* An HTTP GET handler */
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, HTTPServer, "buffer alloc failed");
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTPServer, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, HTTPServer, "buffer alloc failed");
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTPServer, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, HTTPServer, "buffer alloc failed");
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTPServer, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, HTTPServer, "buffer alloc failed");
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTPServer, "Found URL query => %s", buf);
            char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN], dec_param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTPServer, "Found URL query parameter => query1=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(HTTPServer, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTPServer, "Found URL query parameter => query3=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(HTTPServer, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTPServer, "Found URL query parameter => query2=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(HTTPServer, "Decoded query parameter => %s", dec_param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(HTTPServer, "Request headers lost");
    }
    return ESP_OK;
}

static const httpd_uri_t hello = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};

/* An HTTP POST handler */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        ESP_LOGI(HTTPServer, "=========== RECEIVED DATA ==========");
        ESP_LOGI(HTTPServer, "%.*s", ret, buf);
        ESP_LOGI(HTTPServer, "====================================");
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t echo = {
    .uri       = "/echo",
    .method    = HTTP_POST,
    .handler   = echo_post_handler,
    .user_ctx  = NULL
};

// /* An HTTP_ANY handler */
// static esp_err_t any_handler(httpd_req_t *req)
// {
//     /* Send response with body set as the
//      * string passed in user context*/
//     const char* resp_str = (const char*) req->user_ctx;
//     httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

//     // End response
//     httpd_resp_send_chunk(req, NULL, 0);
//     return ESP_OK;
// }

static const httpd_uri_t any = {
    .uri       = "/any",
    .method    = HTTP_ANY,
    .handler   = any_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};


static const httpd_uri_t cardOn1 = {
    .uri       = "/cardOn1",
    .method    = HTTP_ANY,
    .handler   = any_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

// static const httpd_uri_t cardOff1 = {
//     .uri       = "/cardOff1",
//     .method    = HTTP_ANY,
//     .handler   = any_handler,
//     /* Let's pass response string in user
//      * context to demonstrate it's usage */
//     .user_ctx  = NULL
// };

// static const httpd_uri_t cardOn2 = {
//     .uri       = "/cardOn2",
//     .method    = HTTP_ANY,
//     .handler   = any_handler,
//     /* Let's pass response string in user
//      * context to demonstrate it's usage */
//     .user_ctx  = NULL
// };

// static const httpd_uri_t cardOff2 = {
//     .uri       = "/cardOff2",
//     .method    = HTTP_ANY,
//     .handler   = any_handler,
//     /* Let's pass response string in user
//      * context to demonstrate it's usage */
//     .user_ctx  = NULL
// };static const httpd_uri_t cardOn3 = {
//     .uri       = "/cardOn3",
//     .method    = HTTP_ANY,
//     .handler   = any_handler,
//     /* Let's pass response string in user
//      * context to demonstrate it's usage */
//     .user_ctx  = NULL
// };

// static const httpd_uri_t cardOff3 = {
//     .uri       = "/cardOff3",
//     .method    = HTTP_ANY,
//     .handler   = any_handler,
//     /* Let's pass response string in user
//      * context to demonstrate it's usage */
//     .user_ctx  = NULL
// };

static const httpd_uri_t index_ = {
    .uri       = "/",
    .method    = HTTP_ANY,
    .handler   = any_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL  
};


/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/echo", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */
static esp_err_t ctrl_put_handler(httpd_req_t *req)
{
    char buf;
    int ret;

    if ((ret = httpd_req_recv(req, &buf, 1)) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    if (buf == '0') {
        /* URI handlers can be unregistered using the uri string */
        ESP_LOGI(HTTPServer, "Unregistering /hello and /echo URIs");
        httpd_unregister_uri(req->handle, "/hello");
        httpd_unregister_uri(req->handle, "/echo");
        /* Register the custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else {
        ESP_LOGI(HTTPServer, "Registering /hello and /echo URIs");
        httpd_register_uri_handler(req->handle, &hello);
        httpd_register_uri_handler(req->handle, &echo);
        /* Unregister custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, NULL);
    }

    /* Respond with empty body */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t ctrl = {
    .uri       = "/ctrl",
    .method    = HTTP_PUT,
    .handler   = ctrl_put_handler,
    .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    // Definir el tamaño de la pila para la tarea httpd
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;  // Ajusta este valor según sea necesario, por ejemplo, 8192 bytes
    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_next_state[i] = "1";  // Establecer como activado
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
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &index_);
        httpd_register_uri_handler(server, &cardOn1);
        // httpd_register_uri_handler(server, &cardOff1);        
        // httpd_register_uri_handler(server, &cardOn2);
        // httpd_register_uri_handler(server, &cardOff2);        
        // httpd_register_uri_handler(server, &cardOn3);
        // httpd_register_uri_handler(server, &cardOff3);

        httpd_register_uri_handler(server, &echo);
        httpd_register_uri_handler(server, &ctrl);
        httpd_register_uri_handler(server, &any);
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

void start_server_task(void *pvParameter)
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
