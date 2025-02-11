#include "server.h"
#include "servoMotor.h"

extern QueueHandle_t message_queue;  // Declaración externa de message_queue

void init_wifi(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());  // Inicializa el almacenamiento no volátil
    ESP_ERROR_CHECK(esp_netif_init());  // Inicializa la red
    ESP_ERROR_CHECK(esp_event_loop_create_default());  // Crea el loop de eventos
    ESP_ERROR_CHECK(example_connect());  // Conecta Wi-Fi o Ethernet, según la configuración
}

extern "C" void app_main(void)
{
    init_wifi();
    message_queue = xQueueCreate(20, sizeof(uint16_t));
    xTaskCreate(start_server_task, "move_servo_task", 2048, NULL, 5, NULL);
    xTaskCreate(move_servo_task, "move_servo_task", 2048, NULL, 5, NULL);
        while (1) {   
            vTaskDelay(10000/ portTICK_PERIOD_MS);
        }   
}
