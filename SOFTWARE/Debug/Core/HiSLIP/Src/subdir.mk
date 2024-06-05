################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/HiSLIP/Src/HiSLIP.c 

OBJS += \
./Core/HiSLIP/Src/HiSLIP.o 

C_DEPS += \
./Core/HiSLIP/Src/HiSLIP.d 


# Each subdirectory must supply rules for building sources it contributes
Core/HiSLIP/Src/%.o Core/HiSLIP/Src/%.su Core/HiSLIP/Src/%.cyclo: ../Core/HiSLIP/Src/%.c Core/HiSLIP/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g -DDEBUG -DDATA_IN_D2_SRAM -DUSE_HAL_DRIVER -DSTM32H743xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I"/home/grzegorz/git/HiSLIP/SOFTWARE/Core/SCPI" -I"/home/grzegorz/git/HiSLIP/SOFTWARE/Core/SCPI/libscpi/inc" -I"/home/grzegorz/git/HiSLIP/SOFTWARE/Core/SCPI/libscpi/inc/scpi" -I"/home/grzegorz/git/HiSLIP/SOFTWARE/Core/HiSLIP/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/BSP/Components/lan8742 -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-HiSLIP-2f-Src

clean-Core-2f-HiSLIP-2f-Src:
	-$(RM) ./Core/HiSLIP/Src/HiSLIP.cyclo ./Core/HiSLIP/Src/HiSLIP.d ./Core/HiSLIP/Src/HiSLIP.o ./Core/HiSLIP/Src/HiSLIP.su

.PHONY: clean-Core-2f-HiSLIP-2f-Src

