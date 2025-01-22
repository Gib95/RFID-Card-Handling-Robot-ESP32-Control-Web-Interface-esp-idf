#include "servoMotor.h"

// Función para inicializar el PWM del servo
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


void ServoMotor::move_servo(int angle) {
    ESP_LOGI(TAG, "Moving servo to angle: %d", angle);

    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, angle_to_compare(angle)));

    vTaskDelay(pdMS_TO_TICKS(10)); // Espera un poco para el movimiento
}


// Función auxiliar para convertir el ángulo a un valor de comparación de PWM
uint32_t ServoMotor::angle_to_compare(int angle) {
    return (angle - SERVO_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}
