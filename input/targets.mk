input_input_so_OBJECTS = \
	$(common_OBJECTS) \
        input/evdev.o \
        input/input.rbo

input/input.so: $(input_input_so_OBJECTS)
	$(CXX) $(LDFLAGS) -shared -o $@ $^ -ldl -pthread 

all_TARGETS += input/input.so
