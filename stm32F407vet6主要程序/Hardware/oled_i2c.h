#ifndef __OLEDIIC_H
#define __OLEDIIC_H
#include "board.h"
#include <stm32f4xx_hal.h>
#include <stdint.h>
//IO方向设置
#define SDA_IN()  {GPIOB->MODER&=0XFFF3FFFF;GPIOB->MODER|=0x00000000;} // GPIOB PB9 SDA input mode
#define SDA_OUT() {GPIOB->MODER&=0XFFF3FFFF;GPIOB->MODER|=0x00040000;GPIOB->OSPEEDR&=0XFFF3FFFF;GPIOB->OSPEEDR|=0x00080000;} // GPIOB PB9 SDA output 50MHz

//IO操作函数	 

#define OLED_IIC_SCL_High() 		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET)
#define OLED_IIC_SCL_Low()		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET)
#define OLED_IIC_SDA_High()		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET)
#define OLED_IIC_SDA_Low()		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET)
#define OLED_READ_SDA()			HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9)

//IIC所有操作函数
void IIC_Init(void);                //初始化IIC的IO口				 
void IIC_Start(void);				//发送IIC开始信号
void IIC_Stop(void);	  			//发送IIC停止信号
void IIC_Send_Byte(uint8_t txd);			//IIC发送一个字节
uint8_t IIC_Read_Byte(unsigned char ack);//IIC读取一个字节
uint8_t IIC_Wait_Ack(void); 				//IIC等待ACK信号
void IIC_Ack(void);					//IIC发送ACK信号
void IIC_NAck(void);				//IIC不发送ACK信号

void IIC_Write_One_Byte(uint8_t daddr,uint8_t addr,uint8_t data);
uint8_t IIC_Read_One_Byte(uint8_t daddr,uint8_t addr);	  
#endif

