#include "server.h"
#include "servoMotor.h"
#include <math.h>
extern QueueHandle_t message_queue;  // Declaración externa de message_queue

#define SERVO_PULSE_GPIO_A      13
#define SERVO_PULSE_GPIO_B      12
#define DELAYTIME      500

// Definición de valores máximos y mínimos de entrada para los servos
#define MAX_POSITION 8
#define MIN_POSITION 0
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



// Función para calcular el delay según la diferencia de los números de entrada
int calculate_delay(int current_position, int previous_position) {
    ESP_LOGI(TAG, "calculate_delay");

    int difference = abs(current_position - previous_position);
        if (difference == 8) {
        return 700; // 700 ms para la máxima diferencia
    }
    // Si la diferencia es mínima (por ejemplo, de 1 a 2, o de 7 a 8)
    else if (difference == 1) {
        return 100; // 100 ms para la mínima diferencia
    } 
    else {
        float delay = 100 + (difference / 8.0) * (DELAYTIME - 80);
        return (int) delay;
    }
}

void move_servo_task(void *pvParameters) {
    char *message;    
    char *message_copy = (char *)malloc(100 * sizeof(char)); 

    ESP_LOGW(TAG, "move_servo_task alive");
    int servoA_input = 0;
    int previous_position = 0;
    while (1) {      
        if (xQueueReceive(message_queue, &message, portMAX_DELAY) == pdPASS) {
            memcpy(message_copy, message, 26);
            ESP_LOGW(TAG, "Mensaje recibido: %s", message_copy);
            
            if (strcmp(message_copy, "MOVE_SERVO_ON1") == 0) {
                servoA_input = 1;
                servoA.move_servo(0);  // Mover servo A a 0
                vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  // Espera según la diferencia con la posición anterior
                servoB.move_servo(0);  // Mover servo B
            } 
            else if (strcmp(message_copy, "MOVE_SERVO_OFF1") == 0) {
                servoB.move_servo(20);
            }                        
            else if (strcmp(message_copy, "MOVE_SERVO_ON2") == 0) {
                servoA_input = 2;
                servoA.move_servo(20);
                vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  // Espera según la diferencia con la posición anterior
                servoB.move_servo(0);
            }            
            else if (strcmp(message_copy, "MOVE_SERVO_OFF2") == 0) {
                servoB.move_servo(20);
            } 
            else if (strcmp(message_copy, "MOVE_SERVO_ON3") == 0) {
                servoA_input = 3;
                servoA.move_servo(36);
                vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  // Espera según la diferencia con la posición anterior
                servoB.move_servo(0);
            }             
            else if (strcmp(message_copy, "MOVE_SERVO_OFF3") == 0) {
                servoB.move_servo(20);
            }  
            else if (strcmp(message_copy, "MOVE_SERVO_ON4") == 0) {
                servoA_input = 4;
                servoA.move_servo(50);
                vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  // Espera según la diferencia con la posición anterior
                servoB.move_servo(0);
            }  
            else if (strcmp(message_copy, "MOVE_SERVO_OFF4") == 0) {
                servoB.move_servo(20);
            }  
            else if (strcmp(message_copy, "MOVE_SERVO_ON5") == 0) {
                servoA_input = 5;
                servoA.move_servo(68);
                vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  // Espera según la diferencia con la posición anterior
                servoB.move_servo(0);
            }  
            else if (strcmp(message_copy, "MOVE_SERVO_OFF5") == 0) {
                servoB.move_servo(20);
            }  
            else if (strcmp(message_copy, "MOVE_SERVO_ON6") == 0) {
                servoA_input = 6;
                servoA.move_servo(84);
                vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  // Espera según la diferencia con la posición anterior
                servoB.move_servo(0);
            }  
            else if (strcmp(message_copy, "MOVE_SERVO_OFF6") == 0) {
                servoB.move_servo(20);
            }  
            else if (strcmp(message_copy, "MOVE_SERVO_ON7") == 0) {
                servoA_input = 7;
                servoA.move_servo(100);
                vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  // Espera según la diferencia con la posición anterior
                servoB.move_servo(0);
            }  
            else if (strcmp(message_copy, "MOVE_SERVO_OFF7") == 0) {
                servoB.move_servo(20);
            }  
            else if (strcmp(message_copy, "MOVE_SERVO_ON8") == 0) {
                servoA_input = 8;
                servoA.move_servo(115);
                vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  // Espera según la diferencia con la posición anterior
                servoB.move_servo(0);
            }  
            else if (strcmp(message_copy, "MOVE_SERVO_OFF8") == 0) {
                servoB.move_servo(20);
            }
            else{ 
                ESP_LOGE(TAG, "ERROR: no hay actuacion para el mensaje: %s", message_copy);
            }
            previous_position = servoA_input;  // Actualizar la posición anterior
        }
    }
}

extern "C" void app_main(void)
{
    // Inicializa la red
    init_wifi();

    // Crear la cola para los mensajes (con espacio para 10 elementos)
    message_queue = xQueueCreate(20, sizeof(char*));

    // Crea una tarea para iniciar el servidor HTTP
    // xTaskCreate(start_server_task, "start_server_task", 8192, NULL, 5, NULL);
    xTaskCreate(start_server_task, "move_servo_task", 2048, NULL, 5, NULL);

    // Crea una tarea para mover el servo según los mensajes recibidos
    xTaskCreate(move_servo_task, "move_servo_task", 2048, NULL, 5, NULL);
        while (1) {   
            vTaskDelay(10000/ portTICK_PERIOD_MS);  // Espera según la diferencia con la posición anterior
        }   
}
