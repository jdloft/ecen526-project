#ifndef __LED_H__
#define __LED_H__

typedef enum {
    LED_OFF,
    LED_INIT,
    LED_READY,
    LED_ACTIVE,
    LED_FAULT
} wirelesscomm_led_state_t;
extern wirelesscomm_led_state_t current_led_state;


void configure_led(void);
void led_reset();
void led_set(wirelesscomm_led_state_t state);

#endif // __LED_H__
