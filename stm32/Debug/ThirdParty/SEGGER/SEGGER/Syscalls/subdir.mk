################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ThirdParty/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.c 

C_DEPS += \
./ThirdParty/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.d 

OBJS += \
./ThirdParty/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.o 


# Each subdirectory must supply rules for building sources it contributes
ThirdParty/SEGGER/SEGGER/Syscalls/%.o ThirdParty/SEGGER/SEGGER/Syscalls/%.su ThirdParty/SEGGER/SEGGER/Syscalls/%.cyclo: ../ThirdParty/SEGGER/SEGGER/Syscalls/%.c ThirdParty/SEGGER/SEGGER/Syscalls/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/SEGGER/Config" -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/SEGGER/OS" -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/SEGGER/SEGGER" -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/FreeRTOS/include" -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"/home/shelton/Documents/Project/SelfStudy/Embedded_Systems/Mastering_RTOS/stm32/nucleo-f44re/traffic_light_sim/ThirdParty/FreeRTOS" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-ThirdParty-2f-SEGGER-2f-SEGGER-2f-Syscalls

clean-ThirdParty-2f-SEGGER-2f-SEGGER-2f-Syscalls:
	-$(RM) ./ThirdParty/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.cyclo ./ThirdParty/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.d ./ThirdParty/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.o ./ThirdParty/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.su

.PHONY: clean-ThirdParty-2f-SEGGER-2f-SEGGER-2f-Syscalls

