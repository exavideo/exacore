toys_swigtoy_OBJECTS = \
	toys/swigtoy.rbo 

toys/swigtoy.so: $(toys_swigtoy_OBJECTS)
	# maybe???
	$(CXX) $(LDFLAGS) -shared -o $@ $^ -ldl -pthread
