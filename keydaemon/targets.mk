keydaemon_keyer_so_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(thread_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(drivers_v4l2_OBJECTS) \
	$(graphics_OBJECTS) \
	keydaemon/character_generator.o \
	keydaemon/subprocess_character_generator.o \
	keydaemon/png_subprocess_character_generator.o \
	keydaemon/svg_subprocess_character_generator.o \
	keydaemon/keyer_app.o \
	keydaemon/keyer_global.rbo

keydaemon/keyer.so: $(keydaemon_keyer_so_OBJECTS)
	$(CXX) $(LDFLAGS) -shared -o $@ $^ -ldl -pthread -lpng $(graphics_LIBS)

all_TARGETS += keydaemon/keyer.so
