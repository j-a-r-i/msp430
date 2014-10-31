/**
 * Copyright 2014 Jari Ojanen
 */
#include "hw.h"
#include <stdio.h>
#include "main.h"
#include "config.h"

unsigned int g_event = 0;

callback cb_timer1 = NULL;
callback cb_timer2 = NULL;
callback cb_timer3 = NULL;

unsigned char g_rtc_wday = 0;
unsigned char g_rtc_hour = 22;
unsigned char g_rtc_min = 16;
unsigned char g_rtc_sec = 0;

#ifdef GCC
__attribute__ ((__interrupt__(TIMER0_A0_VECTOR)))
static void timer1_isr()
#else
#pragma vector=TIMER0_A0_VECTOR
__interrupt void timer1_isr(void)
#endif
{
	g_event |= EV_TIMER1;
	__bic_SR_register_on_exit(LPM0_bits);
} 

#ifdef GCC
__attribute__ ((__interrupt__(TIMER1_A0_VECTOR)))
static void timer2_isr()
#else
#pragma vector=TIMER1_A0_VECTOR
__interrupt void timer2_isr(void)
#endif
{
	g_event |= EV_TIMER2;
	__bic_SR_register_on_exit(LPM0_bits);
} 

#ifdef GCC
__attribute__ ((__interrupt__(WDT_VECTOR)))
static void timer3_isr()
#else
#pragma vector=WDT_VECTOR
__interrupt void timer3_isr(void)
#endif
{
	g_rtc_sec++;
	if (g_rtc_sec > 59) {
		g_rtc_sec = 0;
		g_rtc_min++;
		if (g_rtc_min > 59) {
			g_rtc_min = 0;
			g_rtc_hour++;
			if (g_rtc_hour > 23) {
				g_rtc_hour = 0;
				g_rtc_wday++;
				if (g_rtc_wday > 6) {
					g_rtc_wday = 0;
				}
			}
		}
	}
	//toggle_LED_RED;
	g_event |= EV_TIMER3;
	__bic_SR_register_on_exit(LPM0_bits);
} 

#ifdef GCC
__attribute__((__interrupt__(PORT1_VECTOR)))
static void port1_isr()
#else
#pragma vector=PORT1_VECTOR
__interrupt void port1_isr(void)
#endif
{
	g_event |= EV_PORT1;
	__bic_SR_register_on_exit(LPM0_bits);
} 

#ifdef GCC
__attribute__((__interrupt__(PORT2_VECTOR)))
static void port2_isr()
#else
#pragma vector=PORT2_VECTOR
__interrupt void port2_isr(void)
#endif
{
	g_event |= EV_PORT2;
	__bic_SR_register_on_exit(LPM0_bits);
} 

#ifdef GCC
__attribute__((__interrupt__(USCIAB0TX_VECTOR)))
static void usci_tx_isr()
#else
#pragma vector=USCIAB0TX_VECTOR
__interrupt void usci_tx_isr(void)
#endif
{
	g_event |= EV_TX;
	__bic_SR_register_on_exit(LPM0_bits);
} 

#ifdef GCC
__attribute__((__interrupt__(USCIAB0RX_VECTOR)))
static void usci_rx_isr()
#else
#pragma vector=USCIAB0RX_VECTOR
__interrupt void usci_rx_isr(void)
#endif
{
	g_event |= EV_RX;
	__bic_SR_register_on_exit(LPM0_bits);
} 

void uart_init()
{
#define TX BIT2
#define RX BIT1

	P1SEL  |= RX + TX;
	P1SEL2 |= RX + TX;

	UCA0CTL1 |= UCSSEL_2; // SMCLK
	UCA0BR0 = 0x08;
	UCA0BR1 = 0x00;
	UCA0MCTL = UCBRS2 + UCBRS0; // modulation
	UCA0CTL1 &= ~UCSWRST;
	UC0IE |= UCA0RXIE;
}

void uart_ch(char ch)
{
	while ( !(IFG2 & UCA0TXIFG));
	UCA0TXBUF = ch;
}

void uart_str(char *str)
{
    while(*str) {
		uart_ch(*str);
        str++;
    }
}

void uart_num(uint8_t num)
{
	char digit0, digit1, digit2='0';
	digit0 = '0'+(num % 10);
	num /= 10;
	digit1 = '0'+(num % 10);
	if (num > 10) {
		num /= 10;
		digit2 = '0'+(num % 10);
		uart_ch(digit2);
	}
	uart_ch(digit1);
	uart_ch(digit0);
}

//------------------------------------------------------------------------------
int main()
{
	WDTCTL = WDTPW + WDTHOLD;              // Stop watchdog timer

	BCSCTL1 = CALBC1_1MHZ;                 // Set DCO to calibrated 1 MHz.
	DCOCTL = CALDCO_1MHZ;

	BCSCTL3 = XT2S_0 | LFXT1S_0 | XCAP_2;

	// Wait xtal to be stable
	//
	do {
		IFG1 &= ~OFIFG;      // Clear OSC fault flag
	    __delay_cycles(50);  // 50us delay
	} while (IFG1 & OFIFG);


	// Init timer1
	//
	TA0CCR0   = 0x7FFF;
	TA0CCTL0  = CCIE;
	TA0CTL = TASSEL_1 + MC_1 + ID_0; // SMCLK, up to CCR0, divider /8

	// Init timer2
	//
	TA1CCR0   = 0x3FFF;
	TA1CCTL0  = CCIE;
	TA1CTL = TASSEL_1 + MC_1 + ID_0; // SMCLK, up to CCR0, divider /8

	// Init timer3
	//
	WDTCTL = WDT_ADLY_1000;
	IE1 |= WDTIE;

	uart_init();

	// main loop
	//
	config_port_init();
	app_init();


	uart_str("main\n");

	//__enable_interrupt();
	while (1) {
		if (g_event == 0)
			__bis_SR_register(LPM0_bits + GIE);

		if (g_event & EV_TIMER1) {
			if (cb_timer1)
				cb_timer1();
			g_event &= ~EV_TIMER1;
		}
		if (g_event & EV_TIMER2) {
			if (cb_timer2)
				cb_timer2();
			g_event &= ~EV_TIMER2;
		}
		if (g_event & EV_TIMER3) {
			if (cb_timer3)
				cb_timer3();
			g_event &= ~EV_TIMER3;
		}
			
	}
	return 0;
}
