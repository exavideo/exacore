keydaemon_keyer_so_OBJECTS = \
	$(common_OBJECTS) \
	$(ipc_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(thread_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(drivers_v4l2_OBJECTS) \
	$(graphics_OBJECTS) \
	$(keydaemon_OBJECTS) \
	keydaemon/keyer_app.o \
	keydaemon/keyer_global.rbo

keydaemon_keyer_run_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(thread_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(drivers_v4l2_OBJECTS) \
	$(graphics_OBJECTS) \
	$(keydaemon_OBJECTS) \
	keydaemon/keyer_app.o \
	keydaemon/keyer_run.o 

keydaemon/keyer.so: $(keydaemon_keyer_so_OBJECTS)
	$(CXX) $(LDFLAGS) -shared -o $@ $^ -ldl -pthread -lpng $(keydaemon_LIBS) $(graphics_LIBS)

keydaemon/keyer_run: $(keydaemon_keyer_run_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ldl -pthread -lpng $(keydaemon_LIBS) $(graphics_LIBS)


all_TARGETS += keydaemon/keyer.so
