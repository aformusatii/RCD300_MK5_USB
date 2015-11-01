/********************************************************************************
Includes
********************************************************************************/

#include "mtimer.h"

volatile uint32_t timer1_ovf_count = 0;

/**
 * Initialize timer.
 */
void initTimer() {
	// Reset all timers
	TCNT0 = 0;
	TCNT1 = 0;
	TCNT2 = 80;

    //TCCR1B = (1<<CS12)|(0<<CS11)|(1<<CS10);
	//_on(TOIE1, TIMSK1);

	// Setup Timer/Counter0
	TCCR0A = 0
		|(1<<COM0A1)    // Bits 7:6 – COM0A1:0: Clear OC0A on Compare Match
		|(0<<COM0A0)    //
		|(1<<COM0B1)    // Bits 5:4 – COM0B1:0: Clear OC0B on Compare Match
		|(0<<COM0B0)
	    |(0<<WGM01)     // Bits 1:0 – WGM01:0: Normal
	    |(0<<WGM00)
	    ;

	TCCR0B = 0
	    |(0<<FOC0A)     // Force Output Compare A
	    |(0<<FOC0B)     // Force Output Compare B
	    |(0<<WGM02)     // Bit 3 – WGM02: Waveform Generation Mode
	    |(0<<CS02)      // Bits 2:0 – CS02:0: Clock Select
	    |(1<<CS01)
	    |(1<<CS00)      // 010 = clock/8
	    ;

	OCR0A = 0;
	OCR0B = 0;

	_on(TOIE0, TIMSK0);

	// Setup Timer/Counter2
	TCCR2A = 0
	    |(0<<COM2A1)    // Bits 7:6 – COM2A1:0: Normal port operation, OC2A disconnected.
	    |(0<<COM2A0)    //
	    |(1<<COM2B1)    // Bits 5:4 – COM2B1:0: Clear OC2B on Compare Match
	    |(0<<COM2B0)
	    |(0<<WGM21)     // Bits 1:0 – WGM21:0: Normal
	    |(0<<WGM20)
	    ;

	TCCR2B = 0
	    |(0<<FOC2A)     // Force Output Compare A
	    |(0<<FOC2B)     // Force Output Compare B
	    |(0<<WGM22)     // Bit 3 – WGM22: Normal
	    |(0<<CS22)      // Bits 2:0 – CS22:0: Clock Select
	    |(1<<CS21)
	    |(1<<CS20)      // 010 = clock/8
	    ;

	OCR2B = 0;

	_on(TOIE2, TIMSK2);
}

void incrementOvf() {
	timer1_ovf_count++;
}

/**
 * Returns elapsed time in milliseconds starting with given startTime (in cpu cicles).
 * Figure valid for clkI/O/1024 (From prescaler) and clk = 8 Mhz
 */
uint32_t getElapsedMilliseconds(uint64_t startTime) {
	uint64_t endTime = getCurrentTimeCicles();
	uint64_t diff = (endTime - startTime);

	return (uint32_t) ((diff * 10000) / 78125);
}

/**
 * Returns cpu cicles needed to reach given period of seconds.
 */
uint64_t convertSecondsToCicles(uint16_t value) {
	return (value == 0) ? 0: getCurrentTimeCicles() + ((value * 78125) / 10);
}

/**
 * Returns current cpu cicles counted so far by Timer 1 (48 bits - 7812.5 per second, max value ~1142 years, should be enough).
 * Figure valid for clkI/O/1024 (From prescaler) and clk = 8 Mhz
 */
uint64_t getCurrentTimeCicles() {
	uint32_t highValue = timer1_ovf_count;
	uint16_t lowValue = TCNT1;

	if (lowValue < 4) {
		highValue = timer1_ovf_count;
	}

	return _to_uint64(highValue, lowValue);
}
