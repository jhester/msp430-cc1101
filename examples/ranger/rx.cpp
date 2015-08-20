#include <msp430fr5728.h>
#include "cc1101.h"

uint8_t RX_buffer[61]={0};
volatile uint8_t sizerx,i,flag;

int main(void){
	//Stop watchdog timer
	WDTCTL = WDTPW + WDTHOLD;
	P1DIR = 0; 
	P1OUT = 0; 
	P1REN = 0xFF;

	P2DIR = 0; 
	P2OUT = 0; 
	P2REN = 0xFF;

	PJDIR = 0xFF;
	PJOUT = 0;

	// Turn off Temp sensor
	REFCTL0 |= REFTCOFF; 
	REFCTL0 &= ~REFON;

	//PJDIR |= BIT0; // Set LED direction, as OUTPUT (Set bit 0 to 1 in PJ Register, (LED))
	//PJOUT &= ~BIT0; // Set output to LOW (Send 0 over PIN PJ)
	
	delay(1); // waits 1000 cycles
	Radio.Init(); // start radio (entirely resets radio)
	Radio.SetDataRate(5); // Needs to be the same in Tx and Rx
	Radio.SetLogicalChannel(1); // Needs to be the same in Tx and Rx	
	
	Radio.RxOn(); // receive mode active
	while(1) {		
		if(Radio.CheckReceiveFlag()) {	// if buffer has contents then flag returns true.	
			//PJOUT ^= BIT0;	// Toggle light LED
			sizerx=Radio.ReceiveData(RX_buffer); // put contents into RX buffer
			Radio.RxOn();
			__no_operation();
		}
		flag = Radio.GetMARCState();
		__no_operation();
	}
	return 1;
}
