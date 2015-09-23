avspipe_avpinput_decklink_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(thread_OBJECTS) \
        $(graphics_OBJECTS) \
        $(mjpeg_OBJECTS) \
    keydaemon/character_generator.o \
    keydaemon/subprocess_character_generator.o \
    keydaemon/svg_subprocess_character_generator.o \
    keydaemon/png_subprocess_character_generator.o \
	avspipe/avpinput_decklink.o

avspipe/avpinput_decklink: $(avspipe_avpinput_decklink_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(drivers_decklink_LIBS) $(common_LIBS) $(raw_frame_LIBS) $(graphics_LIBS) $(mjpeg_LIBS)

all_TARGETS += avspipe/avpinput_decklink    

