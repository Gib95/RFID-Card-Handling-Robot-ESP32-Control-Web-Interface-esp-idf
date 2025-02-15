#include "servoMotor.h"
#include <math.h>
#define ANGLE_MOTOR (180/10)
#define POSICION_ON 50
#define POSICION_OFF 0


typedef enum {
    MOTOR_0 = 0x0000,  // Motor 1 -> Bit 0 (1)
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



void ServoMotor::init_servo_pwm(int gpio) {
    ESP_LOGI(TAG, "Create timer and operator");
    
    // Configuración del temporizador
    mcpwm_timer_handle_t timer = NULL;

    mcpwm_timer_config_t timer_config = {
        .group_id = 0,                                // ID del grupo de MCPWM
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,       // Fuente de reloj por defecto
        .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ, // Resolución en Hz
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,      // Modo de conteo ascendente
        .period_ticks = SERVO_TIMEBASE_PERIOD,        // Periodo de 20 ms para 50 Hz
        .intr_priority = 1,                           // Prioridad de interrupción (puedes cambiarla)
        .flags = {                                    // Flags de configuración
            .update_period_on_empty = 1,              // Actualizar el periodo cuando el contador llegue a cero
            .update_period_on_sync = 0,               // No actualizar en eventos de sincronización
        }
    };

    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    // Configuración del operador
    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    ESP_LOGI(TAG, "Create comparator and generator from the operator");

    // Configuración del comparador
    mcpwm_comparator_config_t comparator_config = {
        .intr_priority = 1,                      // Prioridad de interrupción (ajustable)
        .flags = {
            .update_cmp_on_tez = true,           // Actualizar valor de comparación en TEZ (cuando el contador llega a cero)
            .update_cmp_on_tep = false,          // No actualizar en TEP (cuando el contador llega al pico)
            .update_cmp_on_sync = false,         // No actualizar en sincronización
        }
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

    // Configuración del generador
    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = gpio,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    // Configurar el valor inicial del comparador (centro)
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, angle_to_compare(0)));

    ESP_LOGI(TAG, "Set generator action on timer and compare event");
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

    ESP_LOGI(TAG, "Enable and start timer");
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}

// Constructor
ServoMotor::ServoMotor(int gpio, const char * _TAG){
    TAG = _TAG;
    init_servo_pwm(gpio);
}

// Destructor
ServoMotor::~ServoMotor() {
    // Aquí podrías liberar recursos si fuera necesario
}


void ServoMotor::move_to_target_angle(int angle) {
    // ESP_LOGI(TAG, "Moving servo to angle: %d", angle);
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, angle_to_compare(angle)));
}

void ServoMotor::move_servo(int target_angle) {
    int step = (target_angle > last_position) ? 1 : -1;  // Determina si incrementar o decrementar
    // Mueve gradualmente hasta llegar al ángulo objetivo
    while (last_position != target_angle) {
        last_position += step;
        move_to_target_angle(last_position);  // Mueve el servomotor al ángulo actual
        vTaskDelay(pdMS_TO_TICKS(speed));  // Controla la velocidad (delay entre movimientos)
    }
    ESP_LOGI(TAG, "Reached target angle: %d", target_angle);
}
void ServoMotor::setSpeed(int _speed) {
    speed = _speed;
}


// Función auxiliar para convertir el ángulo a un valor de comparación de PWM
uint32_t ServoMotor::angle_to_compare(int angle) {
    return (angle - SERVO_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}


const char* TAG = "move_servo_task";

ServoMotor servoA(SERVO_PULSE_GPIO_A, "servoA");
ServoMotor servoB(SERVO_PULSE_GPIO_B, "servoB");

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
    uint16_t message;    
    ESP_LOGW(TAG, "move_servo_task alive");
    int servoA_input = 0;
    int previous_position = 0;
    while (1) {      
        if (xQueueReceive(message_queue, &message, portMAX_DELAY) == pdPASS) {
            ESP_LOGW(TAG, "Mensaje recibido: %d", message);
            switch(message){
                case MOTOR_1:
                    servoA_input = 1;
                    servoA.move_servo((servoA_input-1)*ANGLE_MOTOR);
                    vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS); 
                    servoB.move_servo(POSICION_ON);
                    break;
                case MOTOR_2:
                    servoA_input = 2;
                    servoA.move_servo((servoA_input-1)*ANGLE_MOTOR);
                    vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS); 
                    servoB.move_servo(POSICION_ON);
                    break;
                case MOTOR_3:
                    servoA_input = 3;
                    servoA.move_servo((servoA_input-1)*ANGLE_MOTOR);
                    vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS); 
                    servoB.move_servo(POSICION_ON);
                    break;
                case MOTOR_4:
                    servoA_input = 4;
                    servoA.move_servo((servoA_input-1)*ANGLE_MOTOR);
                    vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  
                    servoB.move_servo(POSICION_ON);
                    break;
                case MOTOR_5:
                    servoA_input = 5;
                    servoA.move_servo((servoA_input-1)*ANGLE_MOTOR);
                    vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS); 
                    servoB.move_servo(POSICION_ON);
                    break;
                case MOTOR_6:
                    servoA_input = 6;
                    servoA.move_servo((servoA_input-1)*ANGLE_MOTOR);
                    vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS); 
                    servoB.move_servo(POSICION_ON);
                    break;
                case MOTOR_7:
                    servoA_input = 7;
                    servoA.move_servo((servoA_input-1)*ANGLE_MOTOR);
                    vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  
                    servoB.move_servo(POSICION_ON);
                    break;
                case MOTOR_8:
                    servoA_input = 8;
                    servoA.move_servo((servoA_input-1)*ANGLE_MOTOR);
                    vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  
                    servoB.move_servo(POSICION_ON);
                    break;
                case MOTOR_9:
                    servoA_input = 9;
                    servoA.move_servo((servoA_input-1)*ANGLE_MOTOR);
                    vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  
                    servoB.move_servo(POSICION_ON);
                    break;
                case MOTOR_10:
                    servoA_input = 10;
                    servoA.move_servo((servoA_input-1)*ANGLE_MOTOR);
                    vTaskDelay(calculate_delay(servoA_input, previous_position) / portTICK_PERIOD_MS);  
                    servoB.move_servo(POSICION_ON);
                    break;
                case MOTOR_0:
                    servoB.move_servo(POSICION_OFF);
                    break;
                default:
                    if (message & 0x8000){
                        uint8_t velocidad = message & 0x00FF;  
                        servoA.speed = velocidad;
                        servoB.speed = velocidad;
                        ESP_LOGW(TAG, "Velocidad de retardo actualizada a %d", velocidad);
                    }
                    else{
                        ESP_LOGE(TAG, "ERROR: no hay actuacion para el mensaje: %d", message);
                    }
                    break;
            }
            previous_position = servoA_input;
        }
    }
}
