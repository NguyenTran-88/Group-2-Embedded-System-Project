################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Third_Parties/ssd1306/ssd1306.c \
../Third_Parties/ssd1306/ssd1306_fonts.c 

OBJS += \
./Third_Parties/ssd1306/ssd1306.o \
./Third_Parties/ssd1306/ssd1306_fonts.o 

C_DEPS += \
./Third_Parties/ssd1306/ssd1306.d \
./Third_Parties/ssd1306/ssd1306_fonts.d 


# Each subdirectory must supply rules for building sources it contributes
Third_Parties/ssd1306/%.o Third_Parties/ssd1306/%.su Third_Parties/ssd1306/%.cyclo: ../Third_Parties/ssd1306/%.c Third_Parties/ssd1306/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"E:/ASUS/OneDrive/Desktop/firmware/stm32/Third_Parties/dfplayer" -I"E:/ASUS/OneDrive/Desktop/firmware/stm32/Third_Parties/ssd1306" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Third_Parties-2f-ssd1306

clean-Third_Parties-2f-ssd1306:
	-$(RM) ./Third_Parties/ssd1306/ssd1306.cyclo ./Third_Parties/ssd1306/ssd1306.d ./Third_Parties/ssd1306/ssd1306.o ./Third_Parties/ssd1306/ssd1306.su ./Third_Parties/ssd1306/ssd1306_fonts.cyclo ./Third_Parties/ssd1306/ssd1306_fonts.d ./Third_Parties/ssd1306/ssd1306_fonts.o ./Third_Parties/ssd1306/ssd1306_fonts.su

.PHONY: clean-Third_Parties-2f-ssd1306

