/*
 * File:   main.c
 * Author: karim
 *
 * Created on July 3, 2020, 2:54 PM
 */

#include "config_877A.h"
#include <xc.h>
#include "adc.h"
#include "eeprom_ext.h"
#include "display7s.h"
#include "i2c.h"


#define POWER  (1<<0)
#define UP  (1<<1)
#define DOWN  (1<<2)
#define ADDR 10
#define LED (1<<4)


char tempMode = 0; // initially off
char powerMode = 0; //initially off
char heatMode = 0; //initially off
char coolMode = 0; // initially off
char mask7S = 0xFF;
char maskLED = 0xFF;
char delay7S = 0;
char delayLED = 0;
char i; // for loop variable
unsigned int sum = 0;
char segFlag = 0;
unsigned char tempAvg;
unsigned char tempSet = 60; // set temperature
unsigned char tempCurr; //current temperature
unsigned char tempHist [10] = {60, 60, 60, 60, 60, 60, 60, 60, 60, 60};
unsigned char firstDigit;
unsigned char secondDigit;
char index = 0;
char notPressed = 0;

void interrupt ISR() {

    if (INTF) {

        // turn the power on or off
        if (!powerMode) {
            powerMode = 1;
            if (e2pext_r(11) == 0xFB) {
                tempSet = (unsigned char) e2pext_r(ADDR);
            }
        } else {

            powerMode = 0;

        }
        INTF = 0;


    }
    if (TMR2IF && powerMode) {

        // display current temperature on 7S
        if (segFlag) {
            PORTA = 0x10;
            if (tempMode) {
                firstDigit = tempSet / 10;

            } else {
                firstDigit = tempCurr / 10;

            }
            //            firstDigit = tempSet / 10;

            PORTD = display7s(firstDigit) & mask7S;

        } else {
            PORTA = 0x20;
            if (tempMode) {
                secondDigit = tempSet % 10;

            } else {
                secondDigit = tempCurr % 10;

            }
            //            secondDigit = tempSet % 10;

            PORTD = display7s(secondDigit) & mask7S;
        }
        segFlag = !segFlag;
        TMR2IF = 0;
        TMR2ON = 1;

    }
    if (TMR0IF && powerMode) {

        // get current temperature
        if (tempMode) {
            delay7S += 1;
            if (delay7S == 2) {
                mask7S = mask7S == 0xff ? 0x00 : 0xff;
                delay7S = 0;
            }
        }

        if (heatMode) {
            delayLED += 1;
            if (delayLED == 2) {
                maskLED = maskLED == 0xff ? 0x00 : 0xff;
                delayLED = 0;
            }
        }


        tempCurr = adc_amostra(2);
        tempCurr /= 2;

        if (index == 10) {
            index = 0;
            tempHist[index] = tempCurr;
        } else {
            tempHist[index] = tempCurr;
            index++;
        }

        TMR0IF = 0;
    }
    if (TMR1IF && powerMode) {

        if (tempMode && notPressed < 5) {
            // check 5 seconds limit
            notPressed += 1;

            // blink 7S 
            //            blink7S = 1;


        }
        if (notPressed == 5) {
            tempMode = 0;
            notPressed = 0;
            mask7S = 0xFF;
            e2pext_w(ADDR, (unsigned char) tempSet);
            e2pext_w(11, 0xFB);


        }
        if (heatMode) {

            //            blinkLED = 1;

        }
        TMR1IF = 0;
        TMR1ON = 1;

    }



}

void init() {

    // PORTS init
    TRISB = POWER | UP | DOWN;
    TRISA = 0x07;
    TRISD = 0x00;
    TRISC = 0x00;
    PORTB &= ~LED;
    // INTERUPT FOR POWER BUTTON
    OPTION_REG = 0x00;

    GIE = 1;
    PEIE = 1;
    INTE = 1;



}

void timer_100ms_init() {
    TMR0IE = 1;
    OPTION_REG = 0x07;
    TMR0 = 60;
}

void timer_10ms_init() {
    TMR2 = 1;
    T2CON = 0x07;
    TMR2IE = 1;
}

void timer_1s_init() {
    TMR1 = 3036;
    T1CON = (1 << 5) | (1 << 6) | (1 << 0);
    TMR1IE = 1;
}

void main(void) {

    i2c_init();

    init();

    timer_100ms_init();
    timer_10ms_init();
    timer_1s_init();
    adc_init();
    // e2pext_w(ADDR, tempSet);



    while (1) {

        if (powerMode) {
            //
            //            if (blink7S) {
            //                // blink 7S 
            //                PORTD = 0;
            //                atraso_ms(250);
            //                blink7S = 0;
            //            }
            //            if (blinkLED) {
            //                // blink  LED
            //                PORTB &= ~LED;
            //                atraso_ms(250);
            //                PORTB |= LED;
            //                blinkLED = 0;
            //            }

            if (!(PORTB & UP)) {
                notPressed = 0;
                while (!(PORTB & UP));
                if (!tempMode) {
                    tempMode = 1;
                } else {
                    if (tempSet < 75) {
                        GIE = 0;
                        tempSet += 5;
                        GIE = 1;
                    }
                }
            }

            if (!(PORTB & DOWN)) {
                notPressed = 0;
                while (!(PORTB & DOWN));
                if (!tempMode) {
                    tempMode = 1;
                } else {
                    if (tempSet > 35) {
                        GIE = 0;
                        tempSet -= 5;
                        GIE = 1;
                    }
                }
            }
            // checking for up and down buttons
            //            if (!((PORTB & UP) | (PORTB & DOWN))) {
            //                atraso_ms(30);
            //                if (!((PORTB & UP) | (PORTB & DOWN))) {
            //
            //                    if (!tempMode) {
            //
            //                        tempMode = 1;
            //                    } else {
            //                        notPressed = 0;
            //                        if ((!(PORTB & UP)) &&(tempSet < 75)) {
            //                            atraso_ms(30);
            //                            if ((!(PORTB & UP)) &&(tempSet < 75))
            //                                tempSet += 5;
            //                        } else
            //                            if (tempSet > 35)
            //                            tempSet -= 5;
            //                    }
            //                }
            //            }
            // average of last 10 temperatures
            GIE = 0;
            for (i = 0; i < 9; i++) {
                sum += tempHist[i];
            }
            tempAvg = sum / 10;
            GIE = 1;
            //            tempAvg = tempCurr;

            sum = 0;
            // choose cool mode or heat mode

            if (tempSet - tempAvg >= 5) {

                heatMode = 1;
                coolMode = 0;
            } else if (tempSet - tempAvg <= -5) {

                heatMode = 0;
                coolMode = 1;
            }

            // turn on LED
            if (heatMode || coolMode) {
                PORTBbits.RB4 = maskLED ? 1 : 0;
                if (heatMode) {

                    PORTCbits.RC5 = 1;
                    PORTCbits.RC2 = 0;

                } else if (coolMode) {

                    PORTCbits.RC5 = 0;
                    PORTCbits.RC2 = 1;


                }
            } else {
                PORTB &= ~LED;
            }



        } else {
            PORTCbits.RC5 = 0;
            PORTCbits.RC2 = 0;
            PORTD = 0x00;
        }
    }

    return;
}
