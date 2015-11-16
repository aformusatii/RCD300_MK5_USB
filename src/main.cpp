/********************************************************************************
Includes
********************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "../atmega328/mtimer.h"

#ifdef	__cplusplus
extern "C" {
#endif
#include "../atmega328/usart.h"

#ifdef	__cplusplus
}
#endif

/********************************************************************************
	Macros and Defines
********************************************************************************/
#define BUTTON_SET_0_PRESSED_FLAG  0
#define BUTTON_SET_1_PRESSED_FLAG  1
#define MP3_ENABLED_FLAG           2
#define BUTTON_PRESSED_MAX_COUNT  10

#define SWITCH_ON_MP3 _on(PB0, PORTB)
#define SWITCH_OFF_MP3 _off(PB0, PORTB)

/********************************************************************************
	Function Prototypes
********************************************************************************/
uint16_t adc_read(uint8_t adcx);
void handle_button_set_0();
void handle_button_set_1();

/********************************************************************************
	Global Variables
********************************************************************************/
volatile uint8_t gen_flag_register = 0;

volatile uint8_t button_set_0_count = 0;
volatile uint32_t button_set_0_value = 0;

volatile uint8_t button_set_1_count = 0;
volatile uint32_t button_set_1_value = 0;

/********************************************************************************
	Interrupt Service
********************************************************************************/
ISR(USART_RX_vect)
{
	handle_usart_interrupt();
}

ISR(TIMER1_OVF_vect)
{
	incrementOvf();
}

ISR(INT0_vect)
{
	//printf("\n%i", index++);
}

/********************************************************************************
	Main
********************************************************************************/
int main(void) {

    // initialize code
	usart_init();

    // enable interrupts
    sei();

    // Init timer1
    initTimer();

    // Init GPIO
    _in(DDC0, DDRC); // Analog input 0
    _in(DDC1, DDRC); // Analog input 1
    _in(DDD2, DDRD); // INT0 input
    _out(DDB0, DDRB); // OUT Control 12V MP3

    _in(DDC2, DDRC); // Input
    _in(DDC3, DDRC); // Input
    _in(DDC4, DDRC); // Input
    _in(DDC5, DDRC); // Input

    PORTC = 0;

    SWITCH_OFF_MP3;

    // GPIO Interrupt
    EICRA = (0<<ISC11)|(0<<ISC10)|(1<<ISC01)|(1<<ISC00);
    EIMSK = (0<<INT1)|(1<<INT0);

    // Configure Sleep Mode
    SMCR = (0<<SM2)|(1<<SM1)|(0<<SM0)|(0<<SE);

    // Enable the ADC
    ADCSRA |= _BV(ADEN);

	// Output initialization log
    printf("Start ATMEGA328.");
    printf(CONSOLE_PREFIX);

	// main loop
    while (1) {
    	// main usart loop
    	usart_check_loop();

    	handle_button_set_0();

    	handle_button_set_1();

    	if ((GET_REG1_FLAG(PIND, PD2)) == 0) {
    		SWITCH_OFF_MP3;

    		debug_print("\n bye");
    		sleep_enable();
    		sleep_cpu();
    		sleep_disable();
    		debug_print("\n Hi!");

    		if (GET_REG1_FLAG(gen_flag_register, MP3_ENABLED_FLAG)) {
    			SWITCH_ON_MP3;
    		}
    	}

    }
}

/********************************************************************************
	Functions
********************************************************************************/
void handle_usart_cmd(char *cmd, char *args) {

	if (strcmp(cmd, "test") == 0) {
		printf("\n TEST [%s]", args);
	}

	else if (strcmp(cmd, "timer") == 0) {
		//uint16_t value = atoi(args);
	}

	else if (strcmp(cmd, "adc") == 0) {
		printf("\n ADS [%u]", adc_read(MUX0));
	}

	else if (strcmp(cmd, "sleep") == 0) {
		printf("\n bye");
		sleep_enable();
		sleep_cpu();
		sleep_disable();
		printf("\n Hi!");
	}

	else if (strcmp(cmd, "pb0_on") == 0) {
		SWITCH_ON_MP3;
		SET_REG1_FLAG(gen_flag_register, MP3_ENABLED_FLAG);
	}

	else if (strcmp(cmd, "pb0_off") == 0) {
		SWITCH_OFF_MP3;
		CLR_REG1_FLAG(gen_flag_register, MP3_ENABLED_FLAG);
	}

	else if (strcmp(cmd, "pb0") == 0) {
		if (GET_REG1_FLAG(PORTB, PB0)) {
    		SWITCH_OFF_MP3;
    		CLR_REG1_FLAG(gen_flag_register, MP3_ENABLED_FLAG);
		} else {
    		SWITCH_ON_MP3;
    		SET_REG1_FLAG(gen_flag_register, MP3_ENABLED_FLAG);
		}
	}

	else {
		printf("\n unknown [%s][%s]", cmd, args);
	}
}

void handle_button_set_0() {
	uint16_t mux0Value = adc_read(MUX0);

	if (mux0Value > 220) {
		if (GET_REG1_FLAG(gen_flag_register, BUTTON_SET_0_PRESSED_FLAG)) {
	    	if (_between(button_set_0_value, 604)) { // 543 CD

	    		if (GET_REG1_FLAG(PORTB, PB0)) {
		    		SWITCH_OFF_MP3;
		    		CLR_REG1_FLAG(gen_flag_register, MP3_ENABLED_FLAG);
	    		} else {
		    		SWITCH_ON_MP3;
		    		SET_REG1_FLAG(gen_flag_register, MP3_ENABLED_FLAG);
	    		}

	    		button_set_0_value = 0;

	    		debug_print("\n PRESS 1 [%lu]", button_set_0_value);
	    	}

	    	else if (_between(button_set_0_value, 440)) { // Channel 1
	    		debug_print("\n PRESS 2 [%lu]", button_set_0_value);
	    	}

	    	else if (_between(button_set_0_value, 312)) { // Channel 2
	    		debug_print("\n PRESS 3 [%lu]", button_set_0_value);
	    	}

	    	else if (_between(button_set_0_value, 231)) { // Channel 3
	    		debug_print("\n PRESS 4 [%lu]", button_set_0_value);
	    	}

	    	else {
	    		debug_print("\n UNKNOWN [%lu]", button_set_0_value);
	    	}
		} else {
			if (button_set_0_count >= BUTTON_PRESSED_MAX_COUNT) {
				SET_REG1_FLAG(gen_flag_register, BUTTON_SET_0_PRESSED_FLAG);
				button_set_0_value = mux0Value;
				button_set_0_count = 0;
			} else {
				button_set_0_count++;
			}
		}
	} else if(GET_REG1_FLAG(gen_flag_register, BUTTON_SET_0_PRESSED_FLAG)) {
		CLR_REG1_FLAG(gen_flag_register, BUTTON_SET_0_PRESSED_FLAG);
		debug_print("\n -> END <-");
		button_set_0_value = 0;
	}
}

void handle_button_set_1() {
	uint16_t mux1Value = adc_read(MUX1);

	if (mux1Value > 100) {
		if (GET_REG1_FLAG(gen_flag_register, BUTTON_SET_1_PRESSED_FLAG)) {
			if (_between(button_set_1_value, 543)) { // 543 CD
				_out(DDC4, DDRC);
				debug_print("\n PRESS 1 [%lu]", button_set_1_value);
			}

			else if (_between(button_set_1_value, 352)) { // 352 PREV
				_out(DDC5, DDRC);
				debug_print("\n PRESS 2 [%lu]", button_set_1_value);
			}

			else if (_between(button_set_1_value, 271)) { // 271 NEXT
				_out(DDC3, DDRC);
				debug_print("\n PRESS 3 [%lu]", button_set_1_value);
			}

			else if (_between(button_set_1_value, 240)) { // 240 MENU
				_out(DDC2, DDRC);
				debug_print("\n PRESS 4 [%lu]", button_set_1_value);
			}

			else if (_between(button_set_1_value, 184)) { // 184 FM
				debug_print("\n PRESS 5 [%lu]", button_set_1_value);
			}

			else if (_between(button_set_1_value, 206)) { // 206 AM
				debug_print("\n PRESS 6 [%lu]", button_set_1_value);
			}

			else if (_between(button_set_1_value, 224)) { // 224 AS
				debug_print("\n PRESS 7 [%lu]", button_set_1_value);
			}

			else {
				debug_print("\n UNKNOWN [%lu]", button_set_1_value);
			}
		} else {
			if (button_set_1_count >= BUTTON_PRESSED_MAX_COUNT) {
				SET_REG1_FLAG(gen_flag_register, BUTTON_SET_1_PRESSED_FLAG);
				button_set_1_value = mux1Value;
				button_set_1_count = 0;
			} else {
				button_set_1_count++;
			}
		}
	} else if (GET_REG1_FLAG(gen_flag_register, BUTTON_SET_1_PRESSED_FLAG)) {
		CLR_REG1_FLAG(gen_flag_register, BUTTON_SET_1_PRESSED_FLAG);
		_in(DDC5, DDRC);
		_in(DDC4, DDRC);
		_in(DDC3, DDRC);
		_in(DDC2, DDRC);
		button_set_1_value = 0;

		debug_print("\n -> END <-");
	}
}

uint16_t adc_read(uint8_t adcx) {
	/* adcx is the analog pin we want to use.  ADMUX's first few bits are
	 * the binary representations of the numbers of the pins so we can
	 * just 'OR' the pin's number with ADMUX to select that pin.
	 * We first zero the four bits by setting ADMUX equal to its higher
	 * four bits. */
	ADMUX = adcx;
	ADMUX |= (0<<REFS1)|(1<<REFS0)|(0<<ADLAR);

	_delay_us(300);

	/* This starts the conversion. */
	ADCSRA |= _BV(ADSC);

	/* This is an idle loop that just wait around until the conversion
	 * is finished.  It constantly checks ADCSRA's ADSC bit, which we just
	 * set above, to see if it is still set.  This bit is automatically
	 * reset (zeroed) when the conversion is ready so if we do this in
	 * a loop the loop will just go until the conversion is ready. */
	while ( (ADCSRA & _BV(ADSC)) );

	/* Finally, we return the converted value to the calling function. */
	return ADC;
}
