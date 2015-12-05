# The DeckLink API include here is very Evil. FIXME
drivers_decklink_OBJECTS = \
	drivers/decklink.o \
	$(DECKLINK_SDK_PATH)/DeckLinkAPIDispatch.o \

drivers_v4l2_OBJECTS = \
    drivers/v4l2_input.o

drivers_h264_tcp_input_OBJECTS = \
    drivers/h264_tcp_input.o

drivers_pipe_output_OBJECTS = \
    drivers/pipe_output.o

drivers_decklink_LIBS = -ldl
