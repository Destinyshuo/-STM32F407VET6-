#include "oled_i2c.h"
#include <rthw.h>

static void delay_us(uint32_t us)
{
	rt_hw_us_delay(us);
}

 
//初始化IIC
void IIC_Init(void)
{					     
	GPIO_InitTypeDef GPIO_InitStructure;
	__HAL_RCC_GPIOB_CLK_ENABLE();	//使能GPIOB时钟
	   
	GPIO_InitStructure.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;   //输出
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET); 	// PB8,PB9 output high
}
//产生IIC起始信号
void IIC_Start(void)
{
	SDA_OUT();     //sda线输出
	OLED_IIC_SDA_High();
	OLED_IIC_SCL_High();
	delay_us(4);
 	OLED_IIC_SDA_Low();//START:when CLK is high,DATA change form high to low 
	delay_us(4);
	OLED_IIC_SCL_Low();//钳住I2C总线，准备发送或接收数据 
}	  
//产生IIC停止信号
void IIC_Stop(void)
{
	SDA_OUT();//sda线输出
	OLED_IIC_SCL_Low();
	OLED_IIC_SDA_Low();//STOP:when CLK is high DATA change form low to high
 	delay_us(4);
	OLED_IIC_SCL_High();
	OLED_IIC_SDA_High();//发送I2C总线结束信号
	delay_us(4);							   	
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
uint8_t IIC_Wait_Ack(void)
{
	uint8_t ucErrTime=0;
	SDA_IN();      //SDA设置为输入  
	OLED_IIC_SDA_High(); 
	delay_us(1);	   
	OLED_IIC_SCL_High();
	delay_us(1);	 
	while(OLED_READ_SDA())
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	OLED_IIC_SCL_Low();//时钟输出0 	   
	return 0;  
} 
//产生ACK应答
void IIC_Ack(void)
{
	OLED_IIC_SCL_Low();
	SDA_OUT();
	OLED_IIC_SDA_Low();
	delay_us(2);
	OLED_IIC_SCL_High();
	delay_us(2);
	OLED_IIC_SCL_Low();
}
//不产生ACK应答		    
void IIC_NAck(void)
{
	OLED_IIC_SCL_Low();
	SDA_OUT();
	OLED_IIC_SDA_High();
	delay_us(2);
	OLED_IIC_SCL_High();
	delay_us(2);
	OLED_IIC_SCL_Low();
}					 				     
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void IIC_Send_Byte(uint8_t txd)
{                        
    uint8_t t;   
	SDA_OUT(); 	    
    OLED_IIC_SCL_Low();//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {   
				if((txd&0x80)>>7==1)	OLED_IIC_SDA_High();
				else OLED_IIC_SDA_Low();
        txd<<=1; 	  
		delay_us(2);   //对TEA5767这三个延时都是必须的
		OLED_IIC_SCL_High();
		delay_us(2); 
		OLED_IIC_SCL_Low();
		delay_us(2);
    }	 
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
uint8_t IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	SDA_IN();//SDA设置为输入
    for(i=0;i<8;i++ )
	{
        OLED_IIC_SCL_Low();
        delay_us(2);
		OLED_IIC_SCL_High();
        receive<<=1;
        if(OLED_READ_SDA())receive++;   
		delay_us(1); 
    }					 
    if (!ack)
        IIC_NAck();//发送nACK
    else
        IIC_Ack(); //发送ACK   
    return receive;
}







