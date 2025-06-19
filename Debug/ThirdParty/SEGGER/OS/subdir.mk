################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_FreeRTOS.c \
../ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_REC.c 

C_DEPS += \
./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_FreeRTOS.d \
./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_REC.d 

OBJS += \
./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_FreeRTOS.o \
./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_REC.o 


# Each subdirectory must supply rules for building sources it contributes
ThirdParty/SEGGER/OS/%.o ThirdParty/SEGGER/OS/%.su ThirdParty/SEGGER/OS/%.cyclo: ../ThirdParty/SEGGER/OS/%.c ThirdParty/SEGGER/OS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/SEGGER/Config" -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/SEGGER/OS" -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/SEGGER/SEGGER" -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/FreeRTOS/include" -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/FreeRTOS" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-ThirdParty-2f-SEGGER-2f-OS

clean-ThirdParty-2f-SEGGER-2f-OS:
	-$(RM) ./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_FreeRTOS.cyclo ./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_FreeRTOS.d ./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_FreeRTOS.o ./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_FreeRTOS.su ./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_REC.cyclo ./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_REC.d ./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_REC.o ./ThirdParty/SEGGER/OS/SEGGER_SYSVIEW_REC.su

.PHONY: clean-ThirdParty-2f-SEGGER-2f-OS

