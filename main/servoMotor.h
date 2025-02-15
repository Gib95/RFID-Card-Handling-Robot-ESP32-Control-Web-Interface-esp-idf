#ifndef SERVOMOTOR_H
#define SERVOMOTOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm_prelude.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>


#define SERVO_PIN 13
#define SERVO_MIN_DEGREE 0
#define SERVO_MAX_DEGREE 180
#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2500
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000
#define SERVO_TIMEBASE_PERIOD 20000

#define SERVO_PULSE_GPIO_A      13
#define SERVO_PULSE_GPIO_B      12
#define DELAYTIME      500

#define MAX_POSITION 8
#define MIN_POSITION 0

extern QueueHandle_t message_queue;  // Declaración externa de message_queue

void move_servo_task(void *pvParameters);

class ServoMotor {
public:
    // Constructor
    ServoMotor(int gpio, const char * _TAG);

    // Destructor
    ~ServoMotor();
    void move_servo(int angle);
    void init_servo_pwm(int gpio);
    void setSpeed(int _speed);
    int speed = 0;

private:
    void move_to_target_angle(int angle);
    uint32_t angle_to_compare(int angle);
    int last_position = 0;
    // Miembro privado: el comparador que usaremos en el PWM
    mcpwm_cmpr_handle_t comparator;
    const char* TAG = "";

};

#endif // SERVOMOTOR_H
