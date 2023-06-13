#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


volatile int counter = 0; // Keeps track of time
volatile int step = 0; // Walks through all steps of the sensor cycle
volatile int sleep = 0; // Wait after cycle completion in order to not spam the timeout
volatile int timeout = 0; // Tracks if the sensor has not sent data back if this happens the cycle is restarted
volatile int startTime = 0; // Track when the rising edge of the data signal happens
volatile int stopTime = 0; // Track when the falling edge of the data signal happens
volatile int deltaTime = 0; // Time between rising and falling edge of the data signal
volatile int distance = 0; // Distance measured by the sensor

/**
 * Setzt die LED's bassierend auf der Distanz
 * @param distance Distanzwert int cm
 */
void  setLed(int distance) {

    PORTA = 0x00; // Reset all LED's

    float scale = distance / 400.0; // Scale the distance to fit within the LED range (0 >= scale >= 1)

    // Set LED to high if the condition is met
    if(scale >= 0.5 / 8.0) PORTA |= (1<<PORTA0);
    if(scale >= 1.5 / 8.0) PORTA |= (1<<PORTA1);
    if(scale >= 2.5 / 8.0) PORTA |= (1<<PORTA2);
    if(scale >= 3.5 / 8.0) PORTA |= (1<<PORTA3);
    if(scale >= 4.5 / 8.0) PORTA |= (1<<PORTA4);
    if(scale >= 5.5 / 8.0) PORTA |= (1<<PORTA5);
    if(scale >= 6.5 / 8.0) PORTA |= (1<<PORTA6);
    if(scale >= 7.5 / 8.0) PORTA |= (1<<PORTA7);

}


// Runs every 10us
// Timer0 Compare A  				_VECTOR(21)
ISR(TIMER0_COMPA_vect) {

    counter++; // Increment time keeping variable in order to track time

    // Walk through all steps
    switch (step) {

        // Step 1
        case 0: // TRG ON
            PORTC |= (1 << PORTC0); // set PC0 high


            if(sleep >= 4) {
                sleep = 0;
                step++;
            } else {
                sleep++;
            }
            break;

        // Step 2
        case 1: // TRG OFF
            PORTC &= ~(1 << PORTC0); // set PC0 low
            step++;
            break;

        // Step 3
        case 2: //WAIT ECHO ON
            if(PINC & (1 << PINC1)) {
                startTime = counter; // Start Time of Impuls
                step++;
            }

            timeout++;
            if(timeout >= 1000) {
                step = 0;
                timeout = 0;
            }
            break;

        // Step 4
        case 3: //WAIT ECHO OFF
            if(!(PINC & (1 << PINC1))) {

                // Calculate Time of Travel and Distance
                stopTime = counter;
                deltaTime = stopTime - startTime;
                distance = deltaTime * 10 / 58;
                setLed(distance);

                step++;
            }
            break;

        // Step 5
        case 4: // DELAY AFTER ECHO
            if(sleep >= 1000) {
                step++;
            } else {
                sleep++;
            }
            break;

        // Step 6
        default:
            counter = 0;
            step = 0;
            sleep = 0;
            break;
    }
}



int main() {
    // PIN MODES
    DDRA = 0xff; // Output for LED's

    DDRC |= (1 << DDC0); // Output for TRIGGER signal

    DDRC &= ~(1 << DDC1); // Input for DATA signal
    PORTC |= (1 << PORTC1); // Enable pull-up


    // Setup timer in order to trigger the "on compare A match" interrupt every 10us
    TCCR0A = (1 << WGM01); // set timer mode to CTC
    TCCR0B = (1 << CS01); // set presaler to 8
    OCR0A = 19; // set the output compare match value
    TIMSK0 = (1 << OCIE0A); // enable timer compare match A interrupt


    sei(); // Enable Interrupt

    while (1) { };
}
