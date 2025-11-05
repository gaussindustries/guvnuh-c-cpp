# stm32/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(guvnuh_stm32_c C)

# TODO: Point these to your CMSIS/Device headers if you want <stm32h7xx.h>.
# set(CMSIS_DIR /path/to/CMSIS)
# include_directories(${CMSIS_DIR}/Include ${CMSIS_DIR}/Device/ST/STM32H7xx/Include)

add_executable(stm32_phase0
    main.c
    bsp/board_init.c
    app/app_main.c
)

target_include_directories(stm32_phase0 PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/STM32CubeH7/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/third_party/STM32CubeH7/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    # if you decide to use ST's HAL later:
    # ${CMAKE_SOURCE_DIR}/third_party/STM32CubeH7/Drivers/STM32H7xx_HAL_Driver/Inc
)
target_compile_definitions(stm32_phase0 PRIVATE
    # -DUSE_FREERTOS
)

target_compile_options(stm32_phase0 PRIVATE
    -O0 -g3 -Wall -Wextra -Werror
)

# Linker script & startup would be added later (Phase-1/2).
