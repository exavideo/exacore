# This is a basic template for building an executable.
# Define the objects (or subdirectories) it needs.
# Then add the build rule, and place it into all_TARGETS.
replay_replay_test_OBJECTS = \
	$(common_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(raw_frame_OBJECTS) \
    $(drivers_decklink_OBJECTS) \
    $(thread_OBJECTS) \
    replay/replay_buffer.o \
    replay/replay_ingest.o \
    replay/replay_playout.o \
    replay/replay_test.o

replay/replay_test: $(replay_replay_test_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread

all_TARGETS += replay/replay_test
