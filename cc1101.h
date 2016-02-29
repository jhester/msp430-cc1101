#ifndef CC1101_h
#define CC1101_h

#include <stdint.h>
#include <msp430.h>
#include "macros.h"
#include "registers.h"
#include "pins.h"


/* Helper functions */
#define SPI_TURN_CHIP_SELECT_ON()        SPI_DRIVE_CSN_LOW()
#define SPI_TURN_CHIP_SELECT_OFF()       SPI_DRIVE_CSN_HIGH()
#define SPI_CHIP_SELECT_IS_OFF()         SPI_CSN_IS_HIGH()

/* Radio States */
#define RADIO_STATE_UNKNOWN  0
#define RADIO_STATE_OFF      1
#define RADIO_STATE_IDLE     2
#define RADIO_STATE_RX       3

class CC1101Radio {
	private:
		void SpiInit(void);
		uint8_t SpiTransfer(uint8_t value);
		void GDO_Set (void);
		void Reset (void);
		void SpiWriteReg(uint8_t addr, uint8_t value);
		void SpiWriteBurstReg(uint8_t addr, uint8_t *buffer, uint8_t num);
		uint8_t SpiStrobe(uint8_t strobe);
		uint8_t SpiReadReg(uint8_t addr);
		void SpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t num);
		uint8_t SpiReadStatus(uint8_t addr);
		void RegConfigSettings(void);
		void RxModeOff(void);
		uint8_t SpiReadStatusReg(uint8_t addr);
	public:
		void Init(void);
		void RxOn(void);
		void Idle(void);		
		void Sleep(void);
		void Wakeup(void);
		void SendData(uint8_t *txBuffer, uint8_t size);
		void SendDataNoWait(uint8_t *txBuffer, uint8_t size);		
		void SetDataRate(uint8_t rate_ndx);
		void SetTxPower(uint8_t powrset_ndx);
		void SetLogicalChannel(uint8_t channel);
		void SetMaxPacketLength(uint8_t pkt_length);
		uint8_t GetMARCState();
		uint8_t CheckReceiveFlag(void);
		uint8_t ReceiveData(uint8_t *rxBuffer);
		uint8_t GetState(void);
		int8_t Rssi(void);
};

extern CC1101Radio Radio;

#endif