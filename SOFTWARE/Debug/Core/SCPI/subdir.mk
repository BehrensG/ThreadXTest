################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/SCPI/SCPI_Def.c \
../Core/SCPI/SCPI_Task.c 

OBJS += \
./Core/SCPI/SCPI_Def.o \
./Core/SCPI/SCPI_Task.o 

C_DEPS += \
./Core/SCPI/SCPI_Def.d \
./Core/SCPI/SCPI_Task.d 


# Each subdirectory must supply rules for building sources it contributes
Core/SCPI/%.o Core/SCPI/%.su Core/SCPI/%.cyclo: ../Core/SCPI/%.c Core/SCPI/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g -DDEBUG -DDATA_IN_D2_SRAM -DUSE_HAL_DRIVER -DSTM32H743xx -DUSE_FULL_LL_DRIVER -DTX_INCLUDE_USER_DEFINE_FILE -DNX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I"D:/tmp/ThreadXTest/SOFTWARE/Core/HiSLIP/Inc" -I"D:/tmp/ThreadXTest/SOFTWARE/Core/SCPI" -I"D:/tmp/ThreadXTest/SOFTWARE/Core/SCPI/libscpi/inc" -I"D:/tmp/ThreadXTest/SOFTWARE/Core/SCPI/libscpi/inc/scpi" -I"D:/tmp/ThreadXTest/SOFTWARE/Core/HiSLIP/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../NetXDuo/App -I../NetXDuo/Target -I../Drivers/BSP/Components/lan8742/ -I../Middlewares/ST/netxduo/common/drivers/ethernet/ -I../Middlewares/ST/netxduo/addons/dhcp/ -I../Middlewares/ST/netxduo/common/inc/ -I../Middlewares/ST/netxduo/ports/cortex_m7/gnu/inc/ -I../Middlewares/ST/netxduo/addons/mdns/ -I../Middlewares/ST/threadx/common/inc/ -I../Middlewares/ST/threadx/ports/cortex_m7/gnu/inc/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-SCPI

clean-Core-2f-SCPI:
	-$(RM) ./Core/SCPI/SCPI_Def.cyclo ./Core/SCPI/SCPI_Def.d ./Core/SCPI/SCPI_Def.o ./Core/SCPI/SCPI_Def.su ./Core/SCPI/SCPI_Task.cyclo ./Core/SCPI/SCPI_Task.d ./Core/SCPI/SCPI_Task.o ./Core/SCPI/SCPI_Task.su

.PHONY: clean-Core-2f-SCPI

