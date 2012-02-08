raw_frame_OBJECTS = \
    raw_frame/raw_frame.o \
    raw_frame/audio_packet.o \
    raw_frame/convert/CbYCrY8422_YCbCr8P422_default.o \
    raw_frame/convert/CbYCrY8422_CbYCrY8422_scan_double.o \
    raw_frame/convert/CbYCrY8422_CbYCrY8422_scan_triple.o \
    raw_frame/convert/CbYCrY8422_CbYCrY8422_default.o \
    raw_frame/convert/YCbCr8P422_CbYCrY8422_default.o \
    raw_frame/convert/CbYCrY8422_BGRAn8_default.o \
    raw_frame/convert/CbYCrY8422_BGRAn8_scale_1_2_default.o \
    raw_frame/convert/CbYCrY8422_BGRAn8_scale_1_4_default.o \
    raw_frame/convert/CbYCrY8422_CbYCrY8422_scale_1_4.o \
    raw_frame/draw/CbYCrY8422_alpha_key.o \
    raw_frame/draw/BGRAn8_blit.o \
    raw_frame/draw/BGRAn8_alpha_key.o \


ifneq ($(SKIP_X86_64_ASM), 1)
raw_frame_OBJECTS += \
    raw_frame/convert/CbYCrY8422_BGRAn8_vector.o \
    raw_frame/convert/CbYCrY8422_BGRAn8_scale_line_1_4_vector.o \
    raw_frame/convert/CbYCrY8422_BGRAn8_scale_1_4_vector.o \
    raw_frame/convert/CbYCrY8422_BGRAn8_scale_line_1_2_vector.o \
    raw_frame/convert/CbYCrY8422_BGRAn8_scale_1_2_vector.o \
    raw_frame/convert/CbYCrY8422_YCbCr8P422_vector.o \
    raw_frame/convert/YCbCr8P422_CbYCrY8422_vector.o \
    raw_frame/convert/CbYCrY8422_CbYCrY8422_scale_1_4_vector.o \
    raw_frame/convert/CbYCrY8422_CbYCrY8422_scale_line_1_4_vector.o \
    raw_frame/convert/BGRAn8_BGRAn8_default.o \
    raw_frame/draw/CbYCrY8422_BGRAn8_key_chunk_sse2.o \
    raw_frame/draw/CbYCrY8422_alpha_key_sse2.o \

endif
