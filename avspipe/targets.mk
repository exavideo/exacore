avspipe_avpinput_decklink_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(thread_OBJECTS) \
	avspipe/avpinput_decklink.o

avspipe/avpinput_decklink: $(avspipe_avpinput_decklink_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread -lrt

all_TARGETS += avspipe/avpinput_decklink    

avspipe_avpinput_decklink_mjpeg_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
        $(mjpeg_OBJECTS) \
	$(thread_OBJECTS) \
	avspipe/avpinput_decklink_mjpeg.o

avspipe/avpinput_decklink_mjpeg: $(avspipe_avpinput_decklink_mjpeg_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread -lrt

all_TARGETS += avspipe/avpinput_decklink_mjpeg

avspipe_avpoutput_decklink_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(thread_OBJECTS) \
	avspipe/avpoutput_decklink.o

avspipe/avpoutput_decklink: $(avspipe_avpoutput_decklink_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread

all_TARGETS += avspipe/avpoutput_decklink    
