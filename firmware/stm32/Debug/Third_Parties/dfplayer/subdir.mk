################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Third_Parties/dfplayer/DFPLAYER.c 

OBJS += \
./Third_Parties/dfplayer/DFPLAYER.o 

C_DEPS += \
./Third_Parties/dfplayer/DFPLAYER.d 


# Each subdirectory must supply rules for building sources it contributes
Third_Parties/dfplayer/%.o Third_Parties/dfplayer/%.su Third_Parties/dfplayer/%.cyclo: ../Third_Parties/dfplayer/%.c Third_Parties/dfplayer/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"E:/ASUS/OneDrive/Desktop/firmware/stm32/Third_Parties/dfplayer" -I"E:/ASUS/OneDrive/Desktop/firmware/stm32/Third_Parties/ssd1306" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Third_Parties-2f-dfplayer

clean-Third_Parties-2f-dfplayer:
	-$(RM) ./Third_Parties/dfplayer/DFPLAYER.cyclo ./Third_Parties/dfplayer/DFPLAYER.d ./Third_Parties/dfplayer/DFPLAYER.o ./Third_Parties/dfplayer/DFPLAYER.su

.PHONY: clean-Third_Parties-2f-dfplayer

