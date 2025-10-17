# ThirdParty/FreeRTOS/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
#  A single STATIC library called freertos_kernel
# ---------------------------------------------------------------------------

add_library(freertos_kernel
				croutine.c
				event_groups.c
				list.c
				queue.c
				stream_buffer.c
				tasks.c
				timers.c
)

# ---------------------------------------------------------------------------
#  Public include paths
#     • kernel “include/”
#     • your FreeRTOSConfig.h directory (passed from top level)
# ---------------------------------------------------------------------------
if (NOT FREERTOS_CONFIG_DIR)
	message(FATAL_ERROR "FREERTOS_CONFIG_DIR must point to FreeRTOSConfig.h")
endif()

target_include_directories(freertos_kernel
				PUBLIC
								${CMAKE_CURRENT_LIST_DIR}/include
								${FREERTOS_CONFIG_DIR}
)

# ---------------------------------------------------------------------------
#  Pull in the architecture port (built in portable/CMakeLists.txt)
# ---------------------------------------------------------------------------
add_subdirectory(portable)           # builds target  freertos_port
target_link_libraries(freertos_kernel PUBLIC freertos_port)

