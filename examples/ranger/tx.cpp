#include <msp430fr5728.h>
#include "cc1101.h"
#include <string.h>

uint8_t tx_buffer[61]={0};
volatile uint8_t burstnum,i,flag;

void init_vlo() {
	CSCTL0_H = 0xA5;
	CSCTL1 = DCOFSEL_3; // Set to 8MHz DCO clock
	CSCTL2 = SELA_1 + SELS_3 + SELM_3;        // set ACLK = VLO; MCLK = DCO
	CSCTL3 = DIVA_0 + DIVS_3 + DIVM_3;        // set all dividers, Div by 8 for 1MHz clock speed
}

int main(void){
	//Stop watchdog timer
	WDTCTL = WDTPW + WDTHOLD;
	PJSEL0 &= ~(BIT5 + BIT4);
	PJSEL1 &= ~(BIT4 + BIT5);

	init_vlo();

	// Create a packet of data
	tx_buffer[0] = 12;
	tx_buffer[1] = 23;
	tx_buffer[2] = 34;	
	delay(1);

	Radio.Init();	
	__delay_cycles(1000000);
	flag = Radio.GetMARCState();
	Radio.SetDataRate(5); // Needs to be the same in Tx and Rx
	Radio.SetLogicalChannel(1); // Needs to be the same in Tx and Rx	
	Radio.SetTxPower(0);
       // Radio.Sleep();
	while(1) {		
		for(i=0;i<10;i++) {
			Radio.SendData(tx_buffer,10);
			delay(1);
			flag = Radio.GetMARCState();
			__no_operation();
		}
		delay(1);
	}
	return 1;
}
