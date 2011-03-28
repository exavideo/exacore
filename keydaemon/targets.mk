keydaemon_test_subprocess_cg_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
    $(graphics_OBJECTS) \
    $(thread_OBJECTS) \
    keydaemon/character_generator.o \
    keydaemon/svg_subprocess_character_generator.o \
	keydaemon/test_subprocess_cg.o \

keydaemon/test_subprocess_cg: $(keydaemon_test_subprocess_cg_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(graphics_LIBS)

all_TARGETS += keydaemon/test_subprocess_cg

keydaemon_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
    $(graphics_OBJECTS) \
    $(thread_OBJECTS) \
    $(drivers_decklink_OBJECTS) \
    keydaemon/character_generator.o \
    keydaemon/svg_subprocess_character_generator.o \
	keydaemon/keydaemon.o \

keydaemon/keydaemon: $(keydaemon_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(graphics_LIBS)

all_TARGETS += keydaemon/keydaemon

keyer_lib_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
    $(graphics_OBJECTS) \
    $(thread_OBJECTS) \
    $(drivers_decklink_OBJECTS) \
    keydaemon/character_generator.o \
    keydaemon/svg_subprocess_character_generator.o \

keydaemon/libkeyerfuncs.a: $(keyer_lib_OBJECTS)
	ar rcs $@ $^

all_TARGETS += keydaemon/libkeyerfuncs.a
