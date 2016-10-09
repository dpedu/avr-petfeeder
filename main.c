#define __DEBUG__ 1

#include <avr/io.h>
#include <util/delay.h>
#include <libs/serialdebug.h>
#include <libs/macros.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

// User settings
#define HOUR 3600
// I wanted each block of time to be 45 mins, but my chip's clock is slow
// 40 should roughly scale to 45
#define BLOCK_MINS 40
// How long to pour for
#define DISPENSE_MS 12000

/*


                  vcc [1   14] gnd
                      [2   13] input speed_0
                      [3   12] input speed_1
                  rst [4   11] input speed_2
INT/input feed button [5   10] input speed_3
                motor [6    9]
         debug serial [7    8]
*/

// Ports

#define FEED_DDR DDRA
#define FEED_PORT PORTA
#define FEED_BIT PA7

#define PWRON_DDR DDRA
#define PWRON_PORT PORTA
#define PWRON_BIT PA5

#define FORCE_DDR DDRB
#define FORCE_PORT PINB
#define FORCE_BIT PB2

#define NIBBLE0_DDR  DDRA
#define NIBBLE0_PORT PINA
#define NIBBLE0_BIT  PA0

#define NIBBLE1_DDR  DDRA
#define NIBBLE1_PORT PINA
#define NIBBLE1_BIT  PA1

#define NIBBLE2_DDR  DDRA
#define NIBBLE2_PORT PINA
#define NIBBLE2_BIT  PA2

#define NIBBLE3_DDR  DDRA
#define NIBBLE3_PORT PINA
#define NIBBLE3_BIT  PA3


void motor_on(void) {
    // Start turning the motor
    SET_BIT(FEED_PORT, FEED_BIT);
}

void motor_off(void) {
    // Stop turning the motor
    CLEAR_BIT(FEED_PORT, FEED_BIT);
}

void dispense(int ms) {
    // Activate motor for X ms
    motor_on();
    while(ms > 0) {
        _delay_ms(15);
        ms -= 15;
    }
    motor_off();
}

void pwr_on(void) {
    // Enable power to the motor control and input dial
    SET_BIT(PWRON_PORT, PWRON_BIT);
}

void pwr_off(void) {
    // Disable power to the motor control and input dial
    CLEAR_BIT(PWRON_PORT, PWRON_BIT);
}

#define WAKEUP_PULSE 100
#define WAKEUP_BETWEEN 75

void bootpulse(void) {
    // Pulse the motor quickly
    motor_on();
    _delay_ms(WAKEUP_PULSE);
    motor_off();
    _delay_ms(WAKEUP_BETWEEN);
}

void wakeup(void) {
    // Perform
    motor_on();
    for(int i=0;i<6;i++) bootpulse();
    motor_off();
}

void delay_s(int seconds) {
    while(seconds-- > 0) {
        for (int i = 0; i < 10; i++)
            _delay_ms(100);
        #ifdef __DEBUG__
            dbg_putstring("Wait...\n");
        #endif
    }
}

uint8_t get_speed() {
    // Get speed setting, 0-15 from 4 pins
    uint8_t speed = 0;
    speed |= (IS_SET(NIBBLE3_PORT,NIBBLE3_BIT));
    speed |= (IS_SET(NIBBLE2_PORT,NIBBLE2_BIT) << 1);
    speed |= (IS_SET(NIBBLE1_PORT,NIBBLE1_BIT) << 2);
    speed |= (IS_SET(NIBBLE0_PORT,NIBBLE0_BIT) << 3);
    return speed;
}

void setup() {
    // PART 1: setup io pins
    // Set motor pin as output
    SET_BIT(FEED_DDR, FEED_BIT);
    // Set button pin as input
    CLEAR_BIT(FORCE_DDR, FORCE_BIT);
    // Set speed control pins as input
    CLEAR_BIT(NIBBLE0_DDR, NIBBLE0_BIT);
    CLEAR_BIT(NIBBLE1_DDR, NIBBLE1_BIT);
    CLEAR_BIT(NIBBLE2_DDR, NIBBLE2_BIT);
    CLEAR_BIT(NIBBLE3_DDR, NIBBLE3_BIT);

    // PART 2: Set up interupts
    cli(); //disable global interrupts

    // Set ISC01= 1 and ISC00 = 0 to generate an external interrupt request
    // on any change of INT0
    SET_BIT(MCUCR,ISC00);
    SET_BIT(MCUCR,ISC01);
    //enable external interrupt request 0
    SET_BIT(GIMSK,INT0);

    // disable ADC (saves power)
    ADCSRA = 0;

    // configure sleep mode
    WDTCSR |= (_BV(WDCE) | _BV(WDE));   // Enable the WD Change Bit
    WDTCSR =   _BV(WDIE) |              // Enable WDT Interrupt
    _BV(WDP1) | _BV(WDP2);   // Set Timeout to 1 seconds

    //enable global interrupts
    sei();
}

void gotosleep(void) {
    // Put the MCU to sleep
    sleep_bod_disable();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
}

// Use to track timing
long seconds_since_last_spin = 0;

// watchdog interrupt
ISR (WDT_vect) {
    // break out of sleep mode
    sleep_disable();
    // Increment time counter
    seconds_since_last_spin += 1;
}

//external interrupt ISR (for INT0 pin)
ISR(INT0_vect) {
    #ifdef __DEBUG__
        dbg_putstring("AYE MATEY\n");
    #endif
    pwr_on();
    while(IS_SET(FORCE_PORT, FORCE_BIT)) {
        motor_on();
    }
    motor_off();
    seconds_since_last_spin = 0;
}

int main(void) {
    // Reset all pin configs
    DDRA = 0b00000000;
    DDRB = 0b00000000;

    #ifdef __DEBUG__
        serial_configure();
        dbg_putstring("\n\n");
    #endif

    // Configure the mcu
    setup();

    // Wiggle so the user knows we're alive
    pwr_on();
    wakeup();
    pwr_off();

    delay_s(1);

    while (1) {
        if (seconds_since_last_spin % 10 == 0) {
            pwr_on();
            _delay_ms(1); // Let input pins warm up

            // speed can be 0-15
            // delay = 2hrs + speed * .75h
            // range of 1hr to 12hrs25m
            uint8_t speed = get_speed();
            long sleep_max = HOUR + (BLOCK_MINS * 60 * speed); // 45 mins per unit of speed

            long to_go = sleep_max - seconds_since_last_spin;

            #ifdef __DEBUG__
                dbg_putstring("Mins to sleep: ");
                dbg_putint((unsigned int)sleep_max/60);
                dbg_putstring("\n");
                dbg_putstring("Mins to go:    ");
                dbg_putint((unsigned int)to_go/60);
                dbg_putstring("\n\n");
            #endif

            // If it's been long enough, do the thing
            if(to_go <= 0) {
                dispense(DISPENSE_MS);
                seconds_since_last_spin = 0;
            }
        }

        // Sleep and do it all again tomorrow
        pwr_off();
        gotosleep();
    }

    return 0;

}
