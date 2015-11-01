/********************************************************************************
Includes
********************************************************************************/

#include <avr/interrupt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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

#define _NOP() do { __asm__ __volatile__ ("nop"); } while (0)

/********************************************************************************
	Macros and Defines
********************************************************************************/
#define CONSOLE_PREFIX "\natmega328>"

#define _in(bit,port)	port &= ~(1 << bit)
#define _out(bit,port)	port |= (1 << bit)
#define _on(bit,port)	port |= (1 << bit)
#define _off(bit,port)	port &= ~(1 << bit)

#define _to_uint64(x,y) ((uint64_t) x << 16) | y

#define GET_REG1_FLAG(bit) (1 << bit) & reg1_flags
#define SET_REG1_FLAG(bit) reg1_flags |= (1 << bit)
#define CLR_REG1_FLAG(bit) reg1_flags &= ~(1 << bit)

#define UART_CMD_RECEIVED  0
#define UNSUPPORTED_CMD_RECEIVED  1
#define RF24_RECEIVER_ENABLED  2

/********************************************************************************
	Function Prototypes
********************************************************************************/


/********************************************************************************
	Global Variables
********************************************************************************/

volatile uint8_t usart_cmd_buffer[255];
volatile uint8_t usart_cmd_buffer_count = 0;

volatile uint8_t reg1_flags = 0;

volatile uint64_t endJob1Cicles = 0;
volatile uint16_t job1Seconds = 60;

volatile uint64_t endJob2Cicles = 0;
volatile uint16_t job2Seconds = 10;

volatile uint8_t red_pwd = 1;
volatile uint8_t green_pwd = 1;
volatile uint8_t blue_pwd = 1;

/********************************************************************************
	Interrupt Service
********************************************************************************/

ISR(USART_RX_vect)
{
	uint8_t usart_data = UDR0;

	switch (usart_data) {
		case 00:
			SET_REG1_FLAG(UART_CMD_RECEIVED);
			break;

		default:
			usart_cmd_buffer[usart_cmd_buffer_count++] = usart_data;
	}
}

ISR(TIMER1_OVF_vect)
{
	incrementOvf();
}

ISR(TIMER0_OVF_vect)
{
	TCCR0A = 0
	    |(1<<COM0A1)    // Bits 7:6 – COM0A1:0: Set OC0A on Compare Match
	    |(1<<COM0A0)    //
	    |(1<<COM0B1)    // Bits 5:4 – COM0B1:0: Set OC0B on Compare Match
	    |(1<<COM0B0)
	    |(0<<WGM01)     // Bits 1:0 – WGM01:0: Normal
	    |(0<<WGM00)
	    ;

	TCNT0 = 0;

	_NOP();

	TCCR0B |= (1<<FOC0A)|(1<<FOC0B); // Force Output Compare

	TCCR0A = 0
	    |(1<<COM0A1)    // Bits 7:6 – COM0A1:0: Clear OC0A on Compare Match
	    |(0<<COM0A0)    //
	    |(1<<COM0B1)    // Bits 5:4 – COM0B1:0: Clear OC0B on Compare Match
	    |(0<<COM0B0)
	    |(0<<WGM01)     // Bits 1:0 – WGM01:0: Normal
	    |(0<<WGM00)
	    ;
}

ISR(TIMER2_OVF_vect)
{
	TCCR2A = 0
	    |(0<<COM2A1)    // Bits 7:6 – COM2A1:0: Normal port operation, OC2A disconnected.
	    |(0<<COM2A0)    //
	    |(1<<COM2B1)    // Bits 5:4 – COM2B1:0: Set OC2B on Compare Match
	    |(1<<COM2B0)
	    |(0<<WGM21)     // Bits 1:0 – WGM21:0: Normal
	    |(0<<WGM20)
	    ;

	TCNT2 = 0;

	TCCR2B |= (1<<FOC2A)|(1<<FOC2B); // Force Output Compare

	TCCR2A = 0
		|(0<<COM2A1)    // Bits 7:6 – COM2A1:0: Normal port operation, OC2A disconnected.
		|(0<<COM2A0)    //
	    |(1<<COM2B1)    // Bits 5:4 – COM2B1:0: Clear OC2B on Compare Match
	    |(0<<COM2B0)
	    |(0<<WGM21)     // Bits 1:0 – WGM21:0: Normal
	    |(0<<WGM20)
	    ;
}

/********************************************************************************
	Main
********************************************************************************/
int main(void) {
    // initialize code
	usart_init();

    // enable interrupts
    sei();

    // Init  IO
	_out(DDC0, DDRC); //
	_out(DDC1, DDRC); //
	_out(DDC2, DDRC); //

	_out(DDD3, DDRD); // OC2B
	_out(DDD5, DDRD); // OC0B
	_out(DDD6, DDRD); // OC0A

	_on(PD3, PORTD); // OC2B
	_on(PD5, PORTD); // OC0B
	_on(PD6, PORTD); // OC0A

    // Init timer1
    initTimer();

	// Output initialization log
    printf("Start ATMEGA328.");
    printf(CONSOLE_PREFIX);

    printf("Start NRF24L01P test...");

    // init job1
    //endJob1Cicles = convertSecondsToCicles(job1Seconds);
    endJob1Cicles = 0;

	// main loop
    while (1) {

    	/**
    	 * --------> USART Command received.
    	 */
    	if (GET_REG1_FLAG(UART_CMD_RECEIVED)) {
    		if (usart_cmd_buffer_count == 3) {
    			OCR0B = usart_cmd_buffer[0];
    			OCR2B = usart_cmd_buffer[1];
    			OCR0A = usart_cmd_buffer[2];
    		}

			usart_cmd_buffer_count = 0;
    		CLR_REG1_FLAG(UART_CMD_RECEIVED);
    	}

    	uint64_t currentTimeCicles = getCurrentTimeCicles();

    	/**
    	 * --------> Job 1 (Send data to server)
    	 */
    	if ((endJob1Cicles != 0) && (currentTimeCicles >= endJob1Cicles)) {
    		endJob1Cicles = convertSecondsToCicles(job1Seconds);
    		endJob2Cicles = convertSecondsToCicles(job2Seconds);

    		printf("\n job 1 execute");

    		SET_REG1_FLAG(RF24_RECEIVER_ENABLED);
    	}

    	/**
    	 * --------> Job 2 (Stop Receiving data from Server)
    	 */
    	if ((endJob2Cicles != 0) && (currentTimeCicles >= endJob2Cicles)) {
    		endJob2Cicles = 0;
    		CLR_REG1_FLAG(RF24_RECEIVER_ENABLED);

    		printf("\n job 2 execute");
    	}

    }
}
