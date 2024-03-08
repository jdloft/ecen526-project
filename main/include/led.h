#ifndef __LED_H__
#define __LED_H__

enum led_state {
    LED_OFF,
    LED_READY,
    LED_RECV,
    LED_FAULT
};
extern enum led_state current_led_state;


void configure_led(void);
void led_reset();
void led_set_ready();
void led_set_active();
void led_set_fault();
void blink_led();

#endif // __LED_H__
