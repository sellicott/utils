#include "codec.h"

//public functions
void codecMute(uint8_t enable_mute){}

//private functions
void codecGpioInit(){
	//enable gpio clocks
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | 
	RCC_AHB1ENR_GPIOBEN |
	RCC_AHB1ENR_GPIOCEN | 
	RCC_AHB1ENR_GPIODEN;
	
	//setup gpio pins
	//--- I2S3 ---
	//PA4:  WS
	//PC7:  MCK
	//PC10: CK
	//PC12: SD
	
	//set to altranate function
	GPIOA->MODER |= GPIO_MODER_MODER4_1;	//PA4
	GPIOC->MODER |= GPIO_MODER_MODER7_1 | 	//PC7
	GPIO_MODER_MODER10_1 |	//PC10
	GPIO_MODER_MODER12_1;	//PC12
	
	//set to fast speed
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR4_1;   //PA4
	GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR7_1 |  //PC7
	GPIO_OSPEEDER_OSPEEDR10_1 | //PC10
	GPIO_OSPEEDER_OSPEEDR12_1;  //PC12
	
	//set to altranate function I2S3 (af6)
	GPIOA->AFR[0] |= (6<<16); //PA4
	GPIOC->AFR[0] |= (6<<28); //PC7
	GPIOC->AFR[1] |= (6<<8) | //PC10
			 (6<<16); //PC12
	
	
	//--- I2C1 ---
	//PB6: SCL
	//PB9: SDA
	
	//set to altranate function
	GPIOB->MODER |= GPIO_MODER_MODER6_1 |
			GPIO_MODER_MODER9_1;
	
	//set to open drain output
	GPIOB->OTYPER |= (1<<6) |
			 (1<<9);
	
	//set to fast speed output
	GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR6_1 |
			  GPIO_OSPEEDER_OSPEEDR9_1;
	
	//set to altranate function I2C (af4)
	GPIOB->AFR[0] |= (4<<24);
	GPIOB->AFR[1] |= (4<<4);
	
	
	//setup reset pin PD4
	GPIOD->MODER |= GPIO_MODER_MODER4_0;//push pull output
	GPIOD->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR4_0;//medium speed output
	//no altranate function
}

void codecInit(){
	uint8_t tempreg;
	codecGpioInit();//setup pins
	
	GPIOD->BSRRH |= RESET_PIN;//set reset pin low
	tickDelay(100);//delay 100ms for power to stabilize
	
	I2CInit();//setup i2c 
	
	codecWriteRegister(CODEC_POWER_1, POWER_OFF);//keep power off
	
	tempreg = (1<<6) | (1<<7);//set headphone outputs to always on
	codecWriteRegister(CODEC_POWER_2, tempreg);
	
	codecWriteRegister(0x05, 0x81); // Clock configuration: Auto detection.
	codecWriteRegister(0x06, 0x04); // Set slave mode and Philips audio standard.
	
	//set audio volume
	codecWriteRegister(0x20, 0xff);
	codecWriteRegister(0x21, 0xff);
	
	
	//setup to use stm32 DAC
	codecSetVolume(0xff);
	
	//enable passthough
	codecWriteRegister(0x0E, 0xC0);
	
	//set the passthough audio volume
	codecWriteRegister(0x14, 0x00);
	codecWriteRegister(0x15, 0x00);
	
	/* Disable the analog soft ramp */
	codecWriteRegister(0x0A, 0x00); 
	/* Disable the digital soft ramp */
	codecWriteRegister(0x0E, 0x04);
	/* Disable the limiter attack level */
	codecWriteRegister(0x27, 0x00);
	/* Adjust Bass and Treble levels */
	codecWriteRegister(0x1F, 0x0F);
	/* Adjust PCM volume level */
	codecWriteRegister(0x1A, 0x0A);
	codecWriteRegister(0x1B, 0x0A);
	
	I2SInit();
	
}

void codecSetVolume(uint8_t volume){
		
	if (volume > 0xE6)
	{
		/* Set the Master volume */
		codecWriteRegister(0x20, volume - 0xE7); 
		codecWriteRegister(0x21, volume - 0xE7);     
	}
	else
	{
		/* Set the Master volume */
		codecWriteRegister(0x20, volume + 0x19); 
		codecWriteRegister(0x21, volume + 0x19); 
	}
}


void codecReadRegister(uint8_t reg, uint8_t *data){
	I2CStart(CODEC_ADDRESS, 1);//master rx mode
	I2CWrite(reg);//set which register to read
	I2CStop();
	I2CStart(CODEC_ADDRESS, 0);//master rx mode will read register set earlier
	I2CReadNack(data);
}

void codecWriteRegister(uint8_t reg, uint8_t data){
	I2CStart(CODEC_ADDRESS, 1);//master tx mode
	I2CWrite(reg);
	I2CWrite(data);
	I2CStop();
}

void I2SInit(){
	
	I2S_InitTypeDef I2S_InitType;
// 	I2C_InitTypeDef I2C_InitType;
	
	codecGpioInit();//enable gpio's
	
	//enable prehiprial clocks
	RCC->APB1ENR |= RCC_APB1ENR_SPI3EN | 
	RCC_APB1ENR_I2C1EN;
	
	RCC_PLLI2SCmd(ENABLE);//enable i2s clock
	
	I2S_InitType.I2S_AudioFreq = I2S_AudioFreq_44k;

	I2S_InitType.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
	I2S_InitType.I2S_Mode = I2S_Mode_MasterTx;
	I2S_InitType.I2S_DataFormat = I2S_DataFormat_16b;
	I2S_InitType.I2S_Standard = I2S_Standard_Phillips;
	I2S_InitType.I2S_CPOL = I2S_CPOL_Low;
	
	I2S_Init(SPI3, &I2S_InitType);
	
	/*
	//initilize I2C1
	I2C_InitType.I2C_ClockSpeed = 100000;
	I2C_InitType.I2C_Mode = I2C_Mode_I2C;
	I2C_InitType.I2C_OwnAddress1 = 99;
	I2C_InitType.I2C_Ack = I2C_Ack_Enable;
	I2C_InitType.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitType.I2C_DutyCycle = I2C_DutyCycle_2;
	
	I2C_Init(I2C1, &I2C_InitType);   //initialize the I2C peripheral ...
	I2C_Cmd(I2C1, ENABLE);          //... and turn it on
	*/
	
}


//I2C functions
void I2CInit(){
	// Reset I2C.
	RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
	
	// Configure I2C.
	uint32_t pclk1 = 42000000;
	
	I2C1 ->CR2 = pclk1 / 1000000; // Configure frequency and disable interrupts and DMA.
	I2C1 ->OAR1 = I2C_OAR1_ADDMODE | 0x33;
	
	// Configure I2C speed in standard mode.
	const uint32_t i2c_speed = 100000;
	int ccrspeed = pclk1 / (i2c_speed * 2);
	if (ccrspeed < 4) {
		ccrspeed = 4;
	}
	I2C1 ->CCR = ccrspeed;
	I2C1 ->TRISE = pclk1 / 1000000 + 1;
	
	I2C1 ->CR1 = I2C_CR1_ACK | I2C_CR1_PE; // Enable and configure the I2C peripheral.
}

void I2CStart(uint8_t addr, uint8_t tx){
	while ((I2C1 ->SR2 & I2C_SR2_BUSY));//wait until bus isn't busy
	
	I2C1 ->CR1 |= I2C_CR1_START; // Start the transfer sequence.
	while (!(I2C1 ->SR1 & I2C_SR1_SB )); // Wait for start bit.
	
	if(tx){
		I2C1 ->DR = addr; //set address without read bit
	}
	else{
		I2C1->DR = addr | 0x01;//set address with read bit
	}
	while (!(I2C1 ->SR1 & I2C_SR1_ADDR)); // Wait for master mode.
	I2C1->SR2;//clear busy bit
}

void I2CStop(){
	while ((I2C1 ->SR1 & I2C_SR1_BTF));//wait until data transfer is finished
	I2C1 ->CR1 |= I2C_CR1_STOP; // End the transfer sequence.
}

void I2CWrite(uint8_t data){
	while ((I2C1 ->SR1 & I2C_SR1_TXE));//wait until data is shifted out
	I2C1->DR = data;
}

void I2CReadAck(uint8_t *data){
	I2C1->CR1 |= I2C_CR1_ACK;//ack
	
	while(I2C1->SR1 & I2C_SR1_RXNE);
	*data = I2C1->DR;
}

void I2CReadNack(uint8_t *data){
	I2C1->CR1 |= I2C_CR1_STOP;//nack and stop
	I2C1->CR1 &= ~I2C_CR1_ACK;//clear ack
	
	while(I2C1->SR1 & I2C_SR1_RXNE);
	*data = I2C1->DR;
}
