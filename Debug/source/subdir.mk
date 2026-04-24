################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/New_OSEK.c \
../source/os.c \
../source/semihost_hardfault.c 

C_DEPS += \
./source/New_OSEK.d \
./source/os.d \
./source/semihost_hardfault.d 

OBJS += \
./source/New_OSEK.o \
./source/os.o \
./source/semihost_hardfault.o 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\drivers" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\device" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\utilities\debug_console" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\component\uart" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\utilities\debug_console\config" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\component\serial_manager" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\component\lists" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\device\periph" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\utilities" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\CMSIS" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\CMSIS\m-profile" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\utilities\str" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\board" -I"C:\Users\david\OneDrive\Documentos\Sistemas_Operativos\New_OSEK\source" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source

clean-source:
	-$(RM) ./source/New_OSEK.d ./source/New_OSEK.o ./source/os.d ./source/os.o ./source/semihost_hardfault.d ./source/semihost_hardfault.o

.PHONY: clean-source

