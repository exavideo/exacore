keyer_keyer_so_OBJECTS = \
	$(common_OBJECTS) \
	$(ipc_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(thread_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(drivers_v4l2_OBJECTS) \
	$(graphics_OBJECTS) \
	$(keyer_OBJECTS) \
	keyer/keyer_app.o \
	keyer/keyer_global.rbo

keyer/keyer.so: $(keyer_keyer_so_OBJECTS)
	$(CXX) $(LDFLAGS) -shared -o $@ $^ -ldl -pthread -lpng $(keyer_LIBS) $(graphics_LIBS)

all_TARGETS += keyer/keyer.so
