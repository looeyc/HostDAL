################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/devicedb_sqlite3.c \
../src/devicedb_transfmt.c \
../src/devicedb_zigbee.c \
../src/eventproc.c \
../src/hostdal.c \
../src/module_zigbee.c \
../src/protocol.c 

OBJS += \
./src/devicedb_sqlite3.o \
./src/devicedb_transfmt.o \
./src/devicedb_zigbee.o \
./src/eventproc.o \
./src/hostdal.o \
./src/module_zigbee.o \
./src/protocol.o 

C_DEPS += \
./src/devicedb_sqlite3.d \
./src/devicedb_transfmt.d \
./src/devicedb_zigbee.d \
./src/eventproc.d \
./src/hostdal.d \
./src/module_zigbee.d \
./src/protocol.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/chengm/myLinux/eclipse_prjs/HostDAL/inc" -I"/home/chengm/myLinux/eclipse_prjs/sqlite3" -I"/home/chengm/myLinux/eclipse_prjs/ZNPZcl/inc" -I"/home/chengm/myLinux/eclipse_prjs/ZNPLib/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


