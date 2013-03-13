avspipe_avpinput_decklink_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(thread_OBJECTS) \
	avspipe/avpinput_decklink.o

avspipe/avpinput_decklink: $(avspipe_avpinput_decklink_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread -lpng -lrt

all_TARGETS += avspipe/avpinput_decklink    

