#ifndef SERVOMOTOR_H
#define SERVOMOTOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm_prelude.h"
#include "esp_log.h"

#define SERVO_PIN 13
#define SERVO_MIN_DEGREE 0
#define SERVO_MAX_DEGREE 180
#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2500
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000
#define SERVO_TIMEBASE_PERIOD 20000

class ServoMotor {
public:
    // Constructor
    ServoMotor(int gpio, const char * _TAG);

    // Destructor
    ~ServoMotor();

    // Método para mover el servo a un ángulo específico
    void move_servo(int angle);

    // Función para inicializar el PWM
    void init_servo_pwm(int gpio);

private:
    // Función para convertir el ángulo a valor de comparación
    uint32_t angle_to_compare(int angle);

    // Miembro privado: el comparador que usaremos en el PWM
    mcpwm_cmpr_handle_t comparator;
    const char* TAG = "";

};

#endif // SERVOMOTOR_H
