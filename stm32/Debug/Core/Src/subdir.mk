################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Src/app_main.cpp \
../Core/Src/tlv_parser.cpp \
../Core/Src/traffic_controller.cpp \
../Core/Src/traffic_state_machine.cpp 

C_SRCS += \
../Core/Src/main.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c 

C_DEPS += \
./Core/Src/main.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d 

OBJS += \
./Core/Src/app_main.o \
./Core/Src/main.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/tlv_parser.o \
./Core/Src/traffic_controller.o \
./Core/Src/traffic_state_machine.o 

CPP_DEPS += \
./Core/Src/app_main.d \
./Core/Src/tlv_parser.d \
./Core/Src/traffic_controller.d \
./Core/Src/traffic_state_machine.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.cpp Core/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/SEGGER/Config" -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/SEGGER/OS" -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/SEGGER/SEGGER" -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/FreeRTOS/include" -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/FreeRTOS" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/SEGGER/Config" -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/SEGGER/OS" -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/SEGGER/SEGGER" -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/FreeRTOS/include" -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"/home/shelton/Documents/Projects/traffic-light-sim/stm32/ThirdParty/FreeRTOS" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/app_main.cyclo ./Core/Src/app_main.d ./Core/Src/app_main.o ./Core/Src/app_main.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/tlv_parser.cyclo ./Core/Src/tlv_parser.d ./Core/Src/tlv_parser.o ./Core/Src/tlv_parser.su ./Core/Src/traffic_controller.cyclo ./Core/Src/traffic_controller.d ./Core/Src/traffic_controller.o ./Core/Src/traffic_controller.su ./Core/Src/traffic_state_machine.cyclo ./Core/Src/traffic_state_machine.d ./Core/Src/traffic_state_machine.o ./Core/Src/traffic_state_machine.su

.PHONY: clean-Core-2f-Src

