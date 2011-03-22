# The DeckLink API include here is very Evil. FIXME
drivers_decklink_OBJECTS = \
	drivers/decklink.o \
	$(DECKLINK_SDK_PATH)/DeckLinkAPIDispatch.o \

drivers_pipe_output_OBJECTS = \
    drivers/pipe_output.o
