#include "server.h"
#include "servoMotor.h"
// Declaramos la cola global para compartir entre tareas
extern QueueHandle_t message_queue;  // Declaración externa de message_queue

#define SERVO_PULSE_GPIO_A      13
#define SERVO_PULSE_GPIO_B      12

const char* TAG = "MAIN";

// Objetos ServoMotor globales
ServoMotor servoA(SERVO_PULSE_GPIO_A, "servoA");
ServoMotor servoB(SERVO_PULSE_GPIO_B, "servoB");

// Función para inicializar Wi-Fi
void init_wifi(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());  // Inicializa el almacenamiento no volátil
    ESP_ERROR_CHECK(esp_netif_init());  // Inicializa la red
    ESP_ERROR_CHECK(esp_event_loop_create_default());  // Crea el loop de eventos
    ESP_ERROR_CHECK(example_connect());  // Conecta Wi-Fi o Ethernet, según la configuración
}

void move_servo_task(void *pvParameters) {
    char* message;
    ESP_LOGI(TAG, "move_servo_task alive");

    while (1) {
        // Espera hasta que se reciba un mensaje en la cola
        if (xQueueReceive(message_queue, &message, portMAX_DELAY) == pdPASS) {
            ESP_LOGW(TAG, "Mensaje recibido: %s", message);

            // Mueve el servo A (servoA) a 0 grados si el mensaje es "MOVE_SERVO1"
            if (strcmp(message, "MOVE_SERVO1") == 0) {
                servoA.move_servo(SERVO_MIN_DEGREE);
                servoB.move_servo(SERVO_MIN_DEGREE);

            // Mueve el servo B (servoB) a 180 grados si el mensaje es "MOVE_SERVO2"
            } else if (strcmp(message, "MOVE_SERVO2") == 0) {
                servoA.move_servo(SERVO_MAX_DEGREE);
                servoB.move_servo(SERVO_MAX_DEGREE);
            }
        }
    }
}

extern "C" void app_main(void)
{
    // Inicializa la red
    init_wifi();

    // Crear la cola para los mensajes (con espacio para 10 elementos)
    message_queue = xQueueCreate(10, sizeof(char*));

    // Crea una tarea para iniciar el servidor HTTP
    // xTaskCreate(start_server_task, "start_server_task", 8192, NULL, 5, NULL);
    start_server_task();
    // Crea una tarea para mover el servo según los mensajes recibidos
    xTaskCreate(move_servo_task, "move_servo_task", 2048, NULL, 5, NULL);
}
