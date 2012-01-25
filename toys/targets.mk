toys_swigtoy_OBJECTS = \
	toys/swigtoy.rbo 

toys/swigtoy.so: $(toys_swigtoy_OBJECTS)
	# maybe???
	$(CXX) $(LDFLAGS) -shared -o $@ $^ -ldl -pthread

toys_framebuffer_hack_OBJECTS = \
	toys/framebuffer_hack.o \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(display_surface_OBJECTS) \
        $(thread_OBJECTS) \
	$(graphics_OBJECTS) \

toys/framebuffer_hack: $(toys_framebuffer_hack_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ldl -pthread $(graphics_LIBS)
