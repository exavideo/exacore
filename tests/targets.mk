# This is a basic template for building an executable.
# Define the objects (or subdirectories) it needs.
# Then add the build rule, and place it into all_TARGETS.
test_mjpeg_422_encode_OBJECTS = \
	$(common_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/mjpeg_422_encode.o

tests/mjpeg_422_encode: $(test_mjpeg_422_encode_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -lpng -lrt

all_TARGETS += tests/mjpeg_422_encode    

test_mjpeg_422_encode_bench_OBJECTS = \
	$(common_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/mjpeg_422_encode_bench.o

tests/mjpeg_422_encode_bench: $(test_mjpeg_422_encode_bench_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg

all_TARGETS += tests/mjpeg_422_encode_bench

test_mjpeg_422_decode_OBJECTS = \
	$(common_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/mjpeg_422_decode.o

tests/mjpeg_422_decode: $(test_mjpeg_422_decode_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg

all_TARGETS += tests/mjpeg_422_decode_scaled    

test_mjpeg_422_decode_scaled_OBJECTS = \
	$(common_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/mjpeg_422_decode_scaled.o

tests/mjpeg_422_decode_scaled: $(test_mjpeg_422_decode_scaled_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg

all_TARGETS += tests/mjpeg_422_decode_scaled    

test_mjpeg_422_decode_bench_OBJECTS = \
	$(common_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/mjpeg_422_decode_bench.o

tests/mjpeg_422_decode_bench: $(test_mjpeg_422_decode_bench_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg

all_TARGETS += tests/mjpeg_422_decode_bench

test_CbYCrY8422_scan_double_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/CbYCrY8422_scan_double.o

tests/CbYCrY8422_scan_double: $(test_CbYCrY8422_scan_double_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

all_TARGETS += tests/CbYCrY8422_scan_double    

test_CbYCrY8422_alpha_BGRAn8_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/CbYCrY8422_alpha_BGRAn8.o

tests/CbYCrY8422_alpha_BGRAn8: $(test_CbYCrY8422_alpha_BGRAn8_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

all_TARGETS += tests/CbYCrY8422_alpha_BGRAn8    

test_CbYCrY8422_BGRAn8_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/CbYCrY8422_BGRAn8.o

tests/CbYCrY8422_BGRAn8: $(test_CbYCrY8422_BGRAn8_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

all_TARGETS += tests/CbYCrY8422_BGRAn8

test_CbYCrY8422_BGRAn8_scale_1_4_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/CbYCrY8422_BGRAn8_scale_1_4.o

tests/CbYCrY8422_BGRAn8_scale_1_4: $(test_CbYCrY8422_BGRAn8_scale_1_4_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

all_TARGETS += tests/CbYCrY8422_BGRAn8_scale_1_4

test_CbYCrY8422_BGRAn8_scale_1_2_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/CbYCrY8422_BGRAn8_scale_1_2.o

tests/CbYCrY8422_BGRAn8_scale_1_2: $(test_CbYCrY8422_BGRAn8_scale_1_2_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

all_TARGETS += tests/CbYCrY8422_BGRAn8_scale_1_2

test_stretch_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/stretch.o

tests/stretch: $(test_stretch_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

all_TARGETS += tests/stretch    

test_scan_triple_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/scan_triple.o

tests/scan_triple: $(test_scan_triple_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

all_TARGETS += tests/scan_triple    

test_decklink_output_random_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(thread_OBJECTS) \
	tests/decklink_output_random.o

tests/decklink_output_random: $(test_decklink_output_random_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread

test_decklink_multi_capture_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(thread_OBJECTS) \
	tests/decklink_multi_capture.o

tests/decklink_multi_capture: $(test_decklink_multi_capture_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread -lpng -lrt

all_TARGETS += tests/decklink_multi_capture

test_v4l2_input_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_v4l2_OBJECTS) \
	$(thread_OBJECTS) \
        $(mjpeg_OBJECTS) \
	tests/v4l2_input.o

tests/v4l2_input: $(test_v4l2_input_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread

all_TARGETS += tests/v4l2_input    

test_decklink_output_random_audio_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(thread_OBJECTS) \
	tests/decklink_output_random_audio.o

tests/decklink_output_random_audio: $(test_decklink_output_random_audio_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread -lpng

all_TARGETS += tests/decklink_output_random_audio

test_decklink_copy_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(thread_OBJECTS) \
	tests/decklink_copy.o

tests/decklink_copy: $(test_decklink_copy_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread

test_decklink_audio_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(thread_OBJECTS) \
	tests/decklink_audio.o

tests/decklink_audio: $(test_decklink_audio_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread

all_TARGETS += tests/decklink_copy    

test_decklink_key_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(drivers_decklink_OBJECTS) \
	$(thread_OBJECTS) \
	tests/decklink_key.o

tests/decklink_key: $(test_decklink_key_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread

all_TARGETS += tests/decklink_key    

test_rsvg_frame_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(graphics_OBJECTS) \
	tests/test_rsvg_frame.o

tests/test_rsvg_frame: $(test_rsvg_frame_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread $(graphics_LIBS)

all_TARGETS += tests/test_rsvg_frame    

test_svg_key_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(graphics_OBJECTS) \
    $(drivers_decklink_OBJECTS) \
    $(thread_OBJECTS) \
	tests/svg_key.o

tests/svg_key: $(test_svg_key_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread $(graphics_LIBS)

all_TARGETS += tests/svg_key    

test_fbdev_OBJECTS = \
	$(common_OBJECTS) \
	tests/fbdev.o

tests/fbdev: $(test_fbdev_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ldl -pthread $(graphics_LIBS)

all_TARGETS += tests/fbdev    

test_freetype_test_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(graphics_OBJECTS) \
    $(thread_OBJECTS) \
	tests/freetype_test.o

tests/freetype_test: $(test_freetype_test_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread $(graphics_LIBS)

all_TARGETS += tests/freetype_test    

test_png_decode_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(graphics_OBJECTS) \
        $(thread_OBJECTS) \
	tests/png_decode.o

tests/png_decode: $(test_png_decode_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -lpng -ljpeg -ldl -pthread $(graphics_LIBS)

all_TARGETS += tests/png_decode    

test_clock_monotonic_OBJECTS = \
	$(common_OBJECTS) \
        $(thread_OBJECTS) \
	tests/clock_monotonic.o

tests/clock_monotonic: $(test_clock_monotonic_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -pthread -lrt

all_TARGETS += tests/clock_monotonic    

test_phase_vocoder_OBJECTS= \
        $(common_OBJECTS) \
	$(phase_vocoder_OBJECTS) \
        $(raw_frame_OBJECTS) \
	tests/phase_vocoder.o

tests/phase_vocoder: $(test_phase_vocoder_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -pthread -lrt -lpng $(phase_vocoder_LIBS)

all_TARGETS += tests/phase_vocoder    
