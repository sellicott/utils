#ifndef _CODEC_H_
#define _CODEC_H_

#include <stm32f4xx.h>
#include <stm32f4xx_i2c.h>
#include <stm32f4xx_spi.h>
#include <sys_tick.h>

#define RESET_PIN	(1<<4);

//define codec registers
#define CODEC_POWER_1	0x02
#define CODEC_POWER_2	0x04

//define codec register values
#define POWER_ON	0x9E
#define POWER_OFF	0x01

//define power2 register values
#define PDN_HPB1	7
#define PDN_HPB0	6
#define PDN_HPA1	5
#define PDN_HPA0	4
#define PDN_SPKB1	3
#define PDN_SPKB0	2
#define PDN_SPKA1	1
#define PDN_SPKA0	0 


//codec definitions
#define CODEC_ADDRESS	0x94 //0x10010100

//public functions
void codecInit();
void codecSetVolume(uint8_t volume);
void codecMute(uint8_t enable_mute);

//private functions
void codecGpioInit();
void codecI2CInit();
void codecReadRegister(uint8_t reg, uint8_t *data);
void codecWriteRegister(uint8_t reg, uint8_t data);
void codecSetVolume(uint8_t volume);


void I2SInit();

//I2C functions
void I2CInit();
void I2CStart(uint8_t addr, uint8_t tx);
void I2CStop();
void I2CWrite(uint8_t data);
void I2CReadAck(uint8_t *data);
void I2CReadNack(uint8_t *data);


#endif //_CODEC_H_
