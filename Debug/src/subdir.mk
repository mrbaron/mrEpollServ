################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/mrepoll.c \
../src/myThreadPool.c \
../src/poll.c 

OBJS += \
./src/mrepoll.o \
./src/myThreadPool.o \
./src/poll.o 

C_DEPS += \
./src/mrepoll.d \
./src/myThreadPool.d \
./src/poll.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/mrbaron/workspace/mrEpoolServ/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


