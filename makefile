OBJS = \
./udp_forward.o \
./udp_obfuscator.o 

LIBS := -lboost_system -lpthread

./%.o: ./src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -std=c++0x -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

# All Target
all: ./udp-obfuscator

# Tool invocations
./udp-obfuscator: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "$@" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-rm $(OBJS) udp-obfuscator
	-@echo ' '


