#include "cc1101.h"

/****************************************************************/
#define 	WRITE_BURST     	0x40						//write burst
#define 	READ_SINGLE     	0x80						//read single
#define 	READ_BURST      	0xC0						//read burst
#define 	BYTES_IN_RXFIFO     0x7F  						//uint8_t number in RXfifo

/**
 * Type of register
 */
#define CC1101_CONFIG_REGISTER   READ_SINGLE
#define CC1101_STATUS_REGISTER   READ_BURST

/****************************************************************/

uint8_t PaTabel[8] = {0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0};

uint8_t mrfiRadioState = 0;
uint8_t rfPowerNdx = 0;
uint8_t dataRateNdx = 9;
uint8_t packetLength = 61;

static const uint8_t rfPowerTable[] = {
	/* 10 dBm */	0xC0, // 29.1 mA
	/* 7 dBm */		0xC8, // 24.2 mA
	/* 5 dBm */		0x84, // 19.4 mA
	/* 0 dBm */		0x60, // 15.9 mA
	/* -10 dBm */	0x34, // 14.4 mA
	/* -15 dBm */	0x1D, // 13.1 mA
	/* -20 dBm */	0x0E, // 12.4 mA
	/* -30 dBm */	0x12, // 11.9 mA
};

/** If clocking in a lot of data over SPI bus, clock speed of MCU needs to be higher */
/** Otherwise data will be lost */
// 0-10 data rates, 0-2 only use if in high speed mode on MCU
static const uint8_t rate_MDMCFG3[] = {
	0x3b, // 499.590 kBaud
	0x3b, // 249.795 kBaud
	0x83, // 153.252 kBaud
	0x83, //  76.626 kBaud /* This setting works well, and any data rates below it */
	0x83, //  38.313 kBaud
	0x8b, //  19.355 kBaud
	0x83, //   9.288 kBaud
	0x83, //   4.644 kBaud
	0x83, //   2.322 kBaud
	0x83, //   1.161 kBaud
	0x43  //   0.969 kBaud
};

static const uint8_t rate_MDMCFG4[] = {
	0x8e, // 499.590 kBaud
	0x8d, // 249.795 kBaud
	0x8c, // 153.252 kBaud
	0x8b, //  76.626 kBaud
	0x8a, //  38.313 kBaud
	0x89, //  19.355 kBaud
	0x88, //   9.288 kBaud
	0x87, //   4.644 kBaud
	0x86, //   2.322 kBaud
	0x85, //   1.161 kBaud
	0x85  //   0.969 kBaud
};

#define __mrfi_NUM_LOGICAL_CHANS__      25
static const uint8_t mrfiLogicalChanTable[] = // randomized version
{
   90, 105,  40,  45,  70,
   80, 100,   5,  60, 115,
   15, 125, 120,  50,  95,
   30,  75,  10,  25,  55,
  110,  65,  85,  35,  20
};

/****************************************************************
*FUNCTION NAME:SpiInit
*FUNCTION     :spi communication initialization
*INPUT        :none
*OUTPUT       :none
****************************************************************/

void CC1101Radio::SpiInit(void)
{
	// initialize the SPI pins
	
    /* configure all SPI related pins */
    SPI_CONFIG_CSN_PIN_AS_OUTPUT();
    SPI_CONFIG_SCLK_PIN_AS_OUTPUT();
    SPI_CONFIG_SI_PIN_AS_OUTPUT();
    SPI_CONFIG_SO_PIN_AS_INPUT();

    /* set CSn to default high level */
    SPI_DRIVE_CSN_HIGH();
  
    /* initialize the SPI registers */
	SPI_INIT();
}


/****************************************************************
*FUNCTION NAME:SpiTransfer
*FUNCTION     :spi transfer
*INPUT        :value: data to send
*OUTPUT       :data to receive / status byte
****************************************************************/
uint8_t CC1101Radio::SpiTransfer(uint8_t value)
{
	uint8_t statusByte;
	/* send the command strobe, wait for SPI access to complete */
	SPI_WRITE_BYTE(value);
	SPI_WAIT_DONE();

	/* read the readio status uint8_t returned by the command strobe */
	statusByte = SPI_READ_BYTE();
	//debug_stats[debug_index] = statusByte;
	//debug_index++;
	return statusByte;
}

/****************************************************************
*FUNCTION NAME: GDO_Set()
*FUNCTION     : set GDO0,GDO2 pin
*INPUT        : none
*OUTPUT       : none
****************************************************************/
void CC1101Radio::GDO_Set (void)
{
	CONFIG_GDO0_PIN_AS_INPUT();
	CONFIG_GDO2_PIN_AS_INPUT();
}

/****************************************************************
*FUNCTION NAME:Reset
*FUNCTION     :CC1101 reset //details refer datasheet of CC1101/CC1100//
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void CC1101Radio::Reset (void)
{
	SPI_DRIVE_CSN_LOW();
	delay(1);
	SPI_DRIVE_CSN_HIGH();
	delay(4);
	SPI_DRIVE_CSN_LOW();
	while (SPI_SO_IS_HIGH());
	SpiTransfer(CC1101_SRES);
	while (SPI_SO_IS_HIGH());
	SPI_DRIVE_CSN_HIGH();
}

/****************************************************************
*FUNCTION NAME:Init
*FUNCTION     :CC1101 initialization
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void CC1101Radio::Init(void)
{
	SpiInit();										//spi initialization
	GDO_Set();										//GDO set
	Reset();										//CC1101 reset
	RegConfigSettings();							//CC1101 register config
	SpiWriteBurstReg(CC1101_PATABLE,PaTabel,8);		//CC1101 PATABLE config
	mrfiRadioState = RADIO_STATE_IDLE;
}


/****************************************************************
*FUNCTION NAME:SpiWriteReg
*FUNCTION     :CC1101 write data to register
*INPUT        :addr: register address; value: register value
*OUTPUT       :none
****************************************************************/
void CC1101Radio::SpiWriteReg(uint8_t addr, uint8_t value)
{
	SPI_DRIVE_CSN_LOW();
	while (SPI_SO_IS_HIGH());
	SpiTransfer(addr);
	SpiTransfer(value);
	SPI_DRIVE_CSN_HIGH();
}

/****************************************************************
*FUNCTION NAME:SpiWriteBurstReg
*FUNCTION     :CC1101 write burst data to register
*INPUT        :addr: register address; buffer:register value array; num:number to write
*OUTPUT       :none
****************************************************************/
void CC1101Radio::SpiWriteBurstReg(uint8_t addr, uint8_t *buffer, uint8_t num)
{
	uint8_t i, temp;
	temp = addr | WRITE_BURST;
	
	SPI_TURN_CHIP_SELECT_OFF();
	SPI_TURN_CHIP_SELECT_ON();
	
	while (SPI_SO_IS_HIGH());
	SpiTransfer(temp);
	for (i = 0; i < num; i++)
	{
	    SpiTransfer(buffer[i]);
	}
	SPI_TURN_CHIP_SELECT_OFF();
}

/****************************************************************
*FUNCTION NAME:SpiStrobe
*FUNCTION     :CC1101 Strobe
*INPUT        :strobe: command; //refer define in CC1101.h//
*OUTPUT       :status byte
****************************************************************/
uint8_t CC1101Radio::SpiStrobe(uint8_t strobe)
{
	SPI_TURN_CHIP_SELECT_ON();
	while (SPI_SO_IS_HIGH());
	uint8_t statusByte = SpiTransfer(strobe);
	SPI_TURN_CHIP_SELECT_OFF();
	return statusByte;
}

/****************************************************************
*FUNCTION NAME:SpiReadReg
*FUNCTION     :CC1101 read data from register
*INPUT        :addr: register address
*OUTPUT       :register value
****************************************************************/
uint8_t CC1101Radio::SpiReadReg(uint8_t addr) 
{
	uint8_t temp, value;

    temp = addr|READ_SINGLE;
	SPI_DRIVE_CSN_LOW();
	while (SPI_SO_IS_HIGH());
	SpiTransfer(temp);
	value=SpiTransfer(0);
	SPI_DRIVE_CSN_HIGH();

	return value;
}


/****************************************************************
*FUNCTION NAME:SpiReadStatusReg
*FUNCTION     :CC1101 read data from status register
*INPUT        :addr: register address
*OUTPUT       :register value
****************************************************************/
uint8_t CC1101Radio::SpiReadStatusReg(uint8_t addr) 
{
	uint8_t temp, value;

    temp = addr|READ_BURST;
	SPI_DRIVE_CSN_LOW();
	while (SPI_SO_IS_HIGH());
	SpiTransfer(temp);
	value=SpiTransfer(0);
	SPI_DRIVE_CSN_HIGH();

	return value;
}

/****************************************************************
*FUNCTION NAME:SpiReadBurstReg
*FUNCTION     :CC1101 read burst data from register
*INPUT        :addr: register address; buffer:array to store register value; num: number to read
*OUTPUT       :none
****************************************************************/
void CC1101Radio::SpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t num)
{
	uint8_t i,temp;

	temp = addr | READ_BURST;
	SPI_DRIVE_CSN_LOW();
	while (SPI_SO_IS_HIGH());
	SpiTransfer(temp);
	for(i=0;i<num;i++)
	{
		buffer[i]=SpiTransfer(0);
	}
	SPI_DRIVE_CSN_HIGH();
}

/****************************************************************
*FUNCTION NAME:SpiReadStatus
*FUNCTION     :CC1101 read status register
*INPUT        :addr: register address
*OUTPUT       :status value
****************************************************************/
uint8_t CC1101Radio::SpiReadStatus(uint8_t addr) 
{
	uint8_t value,temp;

	temp = addr | READ_BURST;
	SPI_DRIVE_CSN_LOW();
	while (SPI_SO_IS_HIGH());
	SpiTransfer(temp);
	value=SpiTransfer(0);
	SPI_DRIVE_CSN_HIGH();

	return value;
}

/****************************************************************
*FUNCTION NAME:RegConfigSettings
*FUNCTION     :CC1101 register config //details refer datasheet of CC1101/CC1100//
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void CC1101Radio::RegConfigSettings(void) 
{
    SpiWriteReg(CC1101_FSCTRL1,  0x08);
    SpiWriteReg(CC1101_FSCTRL0,  0x00);
    SpiWriteReg(CC1101_FREQ2,    0x10);
    SpiWriteReg(CC1101_FREQ1,    0xA7);
    SpiWriteReg(CC1101_FREQ0,    0x62);
    SpiWriteReg(CC1101_MDMCFG4,  rate_MDMCFG4[dataRateNdx]); // CHANBW_E[1:0], CHANBW_M[1:0], DRATE_E[3:0], Reset is B10001100, 0x56 is 1.5kBaud, 0x55 is around 0.6kBaud
    SpiWriteReg(CC1101_MDMCFG3,  rate_MDMCFG3[dataRateNdx]); // DRATE_M[7:0], Reset is 0x22, 0x00 with above setting is 1.5kBaud
    SpiWriteReg(CC1101_MDMCFG2,  0x03);
    SpiWriteReg(CC1101_MDMCFG1,  0x22);
    SpiWriteReg(CC1101_MDMCFG0,  0xF8);
    SpiWriteReg(CC1101_CHANNR,   0x00);
    SpiWriteReg(CC1101_DEVIATN,  0x47);
    SpiWriteReg(CC1101_FREND1,   0xB6);
    SpiWriteReg(CC1101_FREND0,   0x10);
    SpiWriteReg(CC1101_MCSM0 ,   0x18);
    SpiWriteReg(CC1101_FOCCFG,   0x1D);
    SpiWriteReg(CC1101_BSCFG,    0x1C);
    SpiWriteReg(CC1101_AGCCTRL2, 0xC7);
	SpiWriteReg(CC1101_AGCCTRL1, 0x00);
    SpiWriteReg(CC1101_AGCCTRL0, 0xB2);
    SpiWriteReg(CC1101_FSCAL3,   0xEA);
	SpiWriteReg(CC1101_FSCAL2,   0x2A);
	SpiWriteReg(CC1101_FSCAL1,   0x00);
    SpiWriteReg(CC1101_FSCAL0,   0x11);
    SpiWriteReg(CC1101_FSTEST,   0x59);
    SpiWriteReg(CC1101_TEST2,    0x81);
    SpiWriteReg(CC1101_TEST1,    0x35);
    SpiWriteReg(CC1101_TEST0,    0x09);
    SpiWriteReg(CC1101_IOCFG2,   0x0B); 	//serial clock.synchronous to the data in synchronous serial mode
    SpiWriteReg(CC1101_IOCFG0,   0x06);  	//asserts when sync word has been sent/received, and de-asserts at the end of the packet 
    SpiWriteReg(CC1101_PKTCTRL1, 0x04);		//two status uint8_ts will be appended to the payload of the packet,including RSSI LQI and CRC OK, no address check
    SpiWriteReg(CC1101_PKTCTRL0, 0x05);		//whitening off;CRC Enable£»fixed length packets set by PKTLEN reg
    SpiWriteReg(CC1101_ADDR,     0x00);		//address used for packet filtration.
    SpiWriteReg(CC1101_PKTLEN,   0x3D); 	//61 uint8_ts max length
}

/****************************************************************
*FUNCTION NAME:SendData
*FUNCTION     :use CC1101 send data
*INPUT        :txBuffer: data array to send; size: number of data to send, no more than packet length
*OUTPUT       :none
****************************************************************/
void CC1101Radio::SendData(uint8_t *txBuffer,uint8_t size)
{
	SpiWriteReg(CC1101_TXFIFO,size); // Send size first in variable length mode (always be in var length mode)
	SpiWriteBurstReg(CC1101_TXFIFO,txBuffer,size);			//write data to send
	SpiStrobe(CC1101_STX);									//start send	
	while (!GDO0_PIN_IS_HIGH());
	while (GDO0_PIN_IS_HIGH());	
	SpiStrobe(CC1101_SFTX);									//flush TXfifo
}

/****************************************************************
*FUNCTION NAME:SendDataNoWait
*FUNCTION     :use CC1101 send data but don't wait for the GDO0 pins
*INPUT        :txBuffer: data array to send; size: number of data to send, no more than packet length
*OUTPUT       :none
****************************************************************/
void CC1101Radio::SendDataNoWait(uint8_t *txBuffer,uint8_t size)
{
	SpiWriteReg(CC1101_TXFIFO,size); // Send size first in variable length mode (always be in var length mode)
	SpiWriteBurstReg(CC1101_TXFIFO,txBuffer,size);			//write data to send
	SpiStrobe(CC1101_STX);									//start send	
	__delay_cycles(1000);
	SpiStrobe(CC1101_SFTX);									//flush TXfifo
}

/****************************************************************
*FUNCTION NAME:SetReceive
*FUNCTION     :set CC1101 to receive state
*INPUT        :none
*OUTPUT       :none
****************************************************************/
void CC1101Radio::RxOn(void)
{
	SpiStrobe(CC1101_SRX);
	mrfiRadioState = RADIO_STATE_RX;
}

/****************************************************************
*FUNCTION NAME:CheckReceiveFlag
*FUNCTION     :check receive data or not
*INPUT        :none
*OUTPUT       :flag: 0 no data; 1 receive data 
****************************************************************/
uint8_t CC1101Radio::CheckReceiveFlag(void)
{
	if(GDO0_PIN_IS_HIGH())			//receive data
	{
		while (GDO0_PIN_IS_HIGH());
		return 1;
	}
	else							// no data
	{
		return 0;
	}
}


/****************************************************************
*FUNCTION NAME:ReceiveData
*FUNCTION     :read data received from RXfifo
*INPUT        :rxBuffer: buffer to store data
*OUTPUT       :size of data received
****************************************************************/
uint8_t CC1101Radio::ReceiveData(uint8_t *rxBuffer)
{
	uint8_t size;
	uint8_t status[2];

	if(SpiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO)
	{
		size=SpiReadReg(CC1101_RXFIFO);
		SpiReadBurstReg(CC1101_RXFIFO,rxBuffer,size);
		SpiReadBurstReg(CC1101_RXFIFO,status,2);
		SpiStrobe(CC1101_SFRX);
		//RxOn(); // ???? I this bad????
		mrfiRadioState = RADIO_STATE_IDLE;
		return size;
	}
	else
	{
		SpiStrobe(CC1101_SFRX);
		//RxOn(); // ???? I this bad????
		mrfiRadioState = RADIO_STATE_IDLE;
		return 0;
	}
}

/****************************************************************
* ReceiveData
* FUNCTION     :set the data rate, i.e. how fast we send data in terms of bytes (baud rate)
* INPUT        :rate_ndx: the ndx in the MDMCFG registers to set to, refer to rate_MDMCFGX arrays
* OUTPUT       :none
****************************************************************/
void CC1101Radio::SetDataRate(uint8_t rate_ndx) {
	dataRateNdx = rate_ndx;
	RxModeOff();
	SpiWriteReg(CC1101_MDMCFG4, rate_MDMCFG4[rate_ndx]);
	SpiWriteReg(CC1101_MDMCFG3, rate_MDMCFG3[rate_ndx]);
	if(mrfiRadioState == RADIO_STATE_RX) {
		RxOn();
	}
}

// Sets ALL harmonics to the same power, which could be undesirable
void CC1101Radio::SetTxPower(uint8_t powrset) {
	rfPowerNdx = powrset;
	RxModeOff();
	for(uint8_t i=0;i<8;i++)
		PaTabel[i] = rfPowerTable[powrset];
	SpiWriteBurstReg(CC1101_PATABLE,PaTabel,8);		//CC1101 PATABLE config
	if(mrfiRadioState == RADIO_STATE_RX) {
		RxOn();
	}
}

// This idles but does not change here
void CC1101Radio::RxModeOff() {
	SpiStrobe(CC1101_SIDLE);
	while (SpiStrobe(CC1101_SNOP) & 0xF0);
	SpiStrobe(CC1101_SFRX);
}

// Idle mode probably should be avoided, as 1mA draw
// Only voltage regulator to digital part and crystal oscillator running 
void CC1101Radio::Idle() {
	//if(mrfiRadioState == RADIO_STATE_RX) {
		RxModeOff();
		mrfiRadioState = RADIO_STATE_IDLE;
	//}
}

// Voltage regulator to digital part off, register values retained (SLEEP state). 
// All GDO pins programmed to 0x2F (HW to 0)
// Lowest power state for radio, should draw ~200nA
void CC1101Radio::Sleep() {
	Idle();
	delay(1);
	SpiStrobe(CC1101_SPWD);	
	mrfiRadioState = RADIO_STATE_OFF;
}

void CC1101Radio::Wakeup() {
	/* if radio is already awake, just ignore wakeup request */
	if(mrfiRadioState != RADIO_STATE_OFF) {
		return;
	}
	
    /* drive CSn low to initiate wakeup */
    SPI_DRIVE_CSN_LOW();

    /* wait for MISO to go high indicating the oscillator is stable */
    while (SPI_SO_IS_HIGH());

    /* wakeup is complete, drive CSn high and continue */
    SPI_DRIVE_CSN_HIGH();
	
	/*
	*  The test registers must be restored after sleep for the CC1100 and CC2500 radios.
	*  This is not required for the CC1101 radio.
	*/
	// #ifndef MRFI_CC1101/
	//  mrfiSpiWriteReg( TEST2, SMARTRF_SETTING_TEST2 );
	//  mrfiSpiWriteReg( TEST1, SMARTRF_SETTING_TEST1 );
	//  mrfiSpiWriteReg( TEST0, SMARTRF_SETTING_TEST0 );
	// #endif

	 /* enter idle mode */
	 mrfiRadioState = RADIO_STATE_IDLE;
}

uint8_t CC1101Radio::GetState() {
	return	mrfiRadioState;
}

uint8_t CC1101Radio::GetMARCState() {
	//return	SpiReadReg(CC1101_MARCSTATE);
	//return SpiStrobe(0xF5);
	return SpiReadStatusReg(CC1101_MARCSTATE) & 0x1F;
}

void CC1101Radio::SetLogicalChannel(uint8_t channel) {
    /* logical channel is not valid? */
   if (channel >= __mrfi_NUM_LOGICAL_CHANS__) return;

    /* make sure radio is off before changing channels */
    RxModeOff();
	
	SpiWriteReg(CC1101_CHANNR, mrfiLogicalChanTable[channel]);
    /* turn radio back on if it was on before channel change */
	
	if(mrfiRadioState == RADIO_STATE_RX) {
		RxOn();
	}
}

void CC1101Radio::SetMaxPacketLength(uint8_t pkt_length) {
	packetLength = pkt_length;
	SpiWriteReg(CC1101_PKTLEN, pkt_length);
}

CC1101Radio Radio;