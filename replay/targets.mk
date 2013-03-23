# This is a basic template for building an executable.
# Define the objects (or subdirectories) it needs.
# Then add the build rule, and place it into all_TARGETS.

EXTERNAL_INCLUDES += $(shell pkg-config --cflags libavformat libavcodec libavutil)
lavf_LIBS = $(shell pkg-config --libs libavformat libavcodec libavutil)

replay_replay_test_OBJECTS = \
	$(common_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(raw_frame_OBJECTS) \
    $(drivers_decklink_OBJECTS) \
    $(thread_OBJECTS) \
    $(display_surface_OBJECTS) \
    $(graphics_OBJECTS) \
    replay/replay_buffer.o \
    replay/replay_ingest.o \
    replay/replay_preview.o \
    replay/replay_playout.o \
    replay/replay_multiviewer.o \
    replay/replay_test.o

replay_base_OBJECTS = \
	$(common_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(thread_OBJECTS) \
	$(display_surface_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(drivers_v4l2_OBJECTS) \
	$(graphics_OBJECTS) \
	$(avspipe_OBJECTS) \
	replay/replay_buffer.o \
	replay/replay_buffer_index.o \
	replay/replay_ingest.o \
	replay/replay_preview.o \
	replay/replay_playout.o \
	replay/replay_multiviewer.o \
	replay/replay_frame_extractor.o \
	replay/replay_gamedata.o \
	replay/replay_frame_data.o \
	replay/replay_frame_cache.o \
	replay/replay_playout_bars_source.o \
	replay/replay_playout_buffer_source.o \
	replay/replay_playout_avspipe_source.o \
	replay/replay_playout_lavf_source.o \
	replay/replay_playout_queue_source.o \
	replay/replay_playout_image_filter.o \

replay_replay_so_OBJECTS = \
	$(replay_base_OBJECTS) \
	replay/replay_global.rbo 

replay/replay.so: $(replay_replay_so_OBJECTS)
	$(CXX) $(LDFLAGS) -shared -o $@ $^ -ljpeg -ldl -pthread $(graphics_LIBS) $(lavf_LIBS)

replay/replay_playout_lavf_source_test: $(replay_base_OBJECTS) replay/replay_playout_lavf_source_test.o
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -lpthread $(graphics_LIBS) $(lavf_LIBS)

all_TARGETS += replay/replay.so
