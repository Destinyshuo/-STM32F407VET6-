import os

# toolchains options
ARCH = 'arm'
CPU = 'cortex-m4'
CROSS_TOOL = 'gcc'

# cross_tool provides the cross compiler
# EXEC_PATH is the compiler execute path, for example, CodeSourcery, Keil MDK, IAR
PLATFORM = 'gcc'
EXEC_PATH = ''

if os.getenv('RTT_EXEC_PATH'):
    EXEC_PATH = os.getenv('RTT_EXEC_PATH')

PREFIX = 'arm-none-eabi-'
CC = PREFIX + 'gcc'
AS = PREFIX + 'gcc'
AR = PREFIX + 'ar'
CXX = PREFIX + 'g++'
LINK = PREFIX + 'gcc'
TARGET_EXT = 'elf'
SIZE = PREFIX + 'size'
OBJDUMP = PREFIX + 'objdump'
OBJCPY = PREFIX + 'objcopy'
DEVICE = ' -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard'
CFLAGS = DEVICE + ' -Wall -ffunction-sections -fdata-sections'
AFLAGS = DEVICE + ' -c -x assembler-with-cpp -Wa,-mimplicit-it=thumb'
LFLAGS = DEVICE + ' -T linkscripts//STM32F407VE//link.lds -Wl,--gc-sections'
CPATH = [
    'drivers',
    'drivers/include',
    'drivers/include/config',
    'applications',
    'Hardware',
    'Software',
    'libraries/CMSIS/Device/ST/STM32F4xx/Include',
    'libraries/CMSIS/Include',
    'libraries/CMSIS/RTOS/Template',
    'libraries/STM32F4xx_HAL_Driver/Inc',
    'libraries/STM32F4xx_HAL_Driver/Inc/Legacy',
    'rt-thread/components/drivers/include',
    'rt-thread/libcpu/arm/cortex-m4',
]
LPATH = ''
CXXFLAGS = ''
POST_ACTION = ''
