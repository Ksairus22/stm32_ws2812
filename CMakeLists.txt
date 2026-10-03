cmake_minimum_required(VERSION 3.22)
project(stm32_ws2812 C ASM)
set(CMAKE_C_STANDARD 11)
set(WS2812_LED_COUNT 8 CACHE STRING "Number of WS2812 LEDs")
set(EFFECT_BRIGHTNESS 64 CACHE STRING "Maximum brightness (1..255)")
set(EFFECT_FRAME_INTERVAL_MS 40 CACHE STRING "Animation frame interval in milliseconds")
set(EFFECT_TRANSITION_MS 250 CACHE STRING "Cross-fade duration between modes in milliseconds")
set(EFFECT_SATURATION 255 CACHE STRING "Colour saturation (0..255)")
set(EFFECT_TRAIL_LENGTH 8 CACHE STRING "Length of moving effect trails")
set(EFFECT_SEQUENCE "0,1,2,3,4,5,6,7,8,9,10,11" CACHE STRING "Comma-separated ordered effect IDs")
set(EFFECT_DURATIONS_MS "4000,4000,4000,4000,4000,4000,4000,4000,4000,4000,4000,4000" CACHE STRING "Comma-separated ordered mode durations")
set(TOOLCHAIN_PREFIX arm-none-eabi)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
add_compile_options(-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections -Wall -Wextra -Wno-unused-parameter -Os)
add_link_options(-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -T${CMAKE_SOURCE_DIR}/linker/stm32f103c8_flash.ld -Wl,--gc-sections -Wl,-Map=${PROJECT_NAME}.map --specs=nano.specs --specs=nosys.specs)
set(CUBE ${CMAKE_SOURCE_DIR}/third_party/STM32CubeF1)
include_directories(include
  ${CUBE}/Drivers/CMSIS/Include
  ${CUBE}/Drivers/CMSIS/Device/ST/STM32F1xx/Include
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Inc
  ${CUBE}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
  ${CUBE}/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc)
set(HAL_SRC
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pcd.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pcd_ex.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c
  ${CUBE}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_usb.c)
set(USB_CORE_SRC
  ${CUBE}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
  ${CUBE}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
  ${CUBE}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c)
set(USB_MSC_SRC
  ${CUBE}/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc.c
  ${CUBE}/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_bot.c
  ${CUBE}/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_data.c
  ${CUBE}/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_scsi.c)
set(STARTUP ${CUBE}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s)
add_executable(${PROJECT_NAME}.elf src/main.c src/clock.c src/stm32f1xx_it.c src/usbd_conf.c src/usbd_desc.c src/usbd_storage.c src/ws2812.c src/effects.c ${CUBE}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c ${HAL_SRC} ${USB_CORE_SRC} ${USB_MSC_SRC} ${STARTUP})
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE USE_HAL_DRIVER STM32F103xB HSE_VALUE=8000000U APP_WS2812_LED_COUNT=${WS2812_LED_COUNT} APP_EFFECT_BRIGHTNESS=${EFFECT_BRIGHTNESS} APP_EFFECT_FRAME_INTERVAL_MS=${EFFECT_FRAME_INTERVAL_MS} APP_EFFECT_TRANSITION_MS=${EFFECT_TRANSITION_MS} APP_EFFECT_SATURATION=${EFFECT_SATURATION} APP_EFFECT_TRAIL_LENGTH=${EFFECT_TRAIL_LENGTH} APP_EFFECT_SEQUENCE=${EFFECT_SEQUENCE} APP_EFFECT_DURATIONS_MS=${EFFECT_DURATIONS_MS})
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
  COMMAND ${TOOLCHAIN_PREFIX}-objcopy -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
  COMMAND ${TOOLCHAIN_PREFIX}-objcopy -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex)
