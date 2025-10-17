# ThirdParty/FreeRTOS/portable/

# Only the Cortex-M4F/GCC port plus one heap scheme.
# If you switch to heap_5 or add MPU support, adjust the list.

add_library(freertos_port
				GCC/ARM_CM4F/port.c
				MemMang/heap_4.c
)
# Re-use the include dirs that the segger target already exports
target_link_libraries(freertos_port PUBLIC segger)


target_include_directories(freertos_port
				PUBLIC
								${CMAKE_CURRENT_LIST_DIR}/GCC/ARM_CM4F
								${CMAKE_CURRENT_LIST_DIR}/../include
								${FREERTOS_CONFIG_DIR}
)

