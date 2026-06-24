# ThirdParty/SEGGER/CMakeLists.txt
#   - RTT + SystemView core      -> SEGGER/
#   - FreeRTOS glue + config     -> OS/, Config/
#   - One tiny ASM file for RTT  -> SEGGER/

cmake_minimum_required(VERSION 3.16)

# 1. Collect only the sources we need
file(GLOB SEGGER_CORE_SRCS CONFIGURE_DEPENDS
					SEGGER/SEGGER.c
					SEGGER/SEGGER_SYSVIEW*.c
					SEGGER/SEGGER_RTT*.c)

set(SEGGER_OS_SRCS
				OS/SEGGER_SYSVIEW_FreeRTOS.c
				Config/SEGGER_SYSVIEW_Config_FreeRTOS.c
				# Config/SEGGER_SYSVIEW_Config_UART.c
				OS/SEGGER_SYSVIEW_REC.c
				# Config/BSP_UART.c
)

set(SEGGER_ASM_SRCS
				SEGGER/SEGGER_RTT_ASM_ARMv7M.S)

add_library(segger STATIC
				${SEGGER_CORE_SRCS}
				${SEGGER_OS_SRCS}
				${SEGGER_ASM_SRCS})

# 2. Public include paths (so user code can #include <SEGGER_RTT.h> etc.)
target_include_directories(segger PUBLIC
				${CMAKE_CURRENT_LIST_DIR}/SEGGER
				${CMAKE_CURRENT_LIST_DIR}/OS
				${CMAKE_CURRENT_LIST_DIR}/Config
				${CMAKE_SOURCE_DIR}/ThirdParty/FreeRTOS/include
				${FREERTOS_CONFIG_DIR}
				${CMAKE_SOURCE_DIR}/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F
)

# 3. Optional compile-time constants
# target_compile_definitions(segger PUBLIC
#     SEGGER_SYSVIEW_CPU_FREQ=168000000        # HCLK on the NUCLEO-F446RE
#     SEGGER_SYSVIEW_TICK_FREQ=1000            # configTICK_RATE_HZ
#     SEGGER_SYSVIEW_RTT_BUFFER_SIZE=4096)     # shrink/grow as you like
#
