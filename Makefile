# Adjust these to match your CPU and compiler configuration.
# Defaults should work well for a 64-bit build on AMD hardware. 
# 32-bit is not supported for the time being!

CXX=g++
CXXFLAGS=-g -O3 -march=k8 -W -Wall -DRAWFRAME_POSIX_IO
LDFLAGS=-g -O3 -march=k8
ASM=yasm -f elf64 -g dwarf2

-include local.mk

all: do_all_targets

EXTERNAL_INCLUDES = \
	-I $(DECKLINK_SDK_PATH) \

SUBDIR_INCLUDES = \
	-I mjpeg/ \
	-I common/ \
	-I raw_frame/ \
	-I thread/ \
	-I output_adapter/ \

common_OBJECTS = \
	common/xmalloc.o \
	common/posix_util.o \
	common/cpu_dispatch.o \

mjpeg_OBJECTS = \
	mjpeg/libjpeg_glue.o \
	mjpeg/mjpeg_encode.o \

raw_frame_OBJECTS = \
	raw_frame/raw_frame.o \
	raw_frame/convert/CbYCrY8422_YCbCr8P422_default.o \
	raw_frame/convert/CbYCrY8422_YCbCr8P422_sse3.o \
	raw_frame/convert/CbYCrY8422_YCbCr8P422_ssse3.o \
	raw_frame/convert/CbYCrY8422_CbYCrY8422_default.o \

# The DeckLink API include here is very Evil. FIXME
output_adapter_OBJECTS = \
	output_adapter/decklink_output_adapter.o \
	$(DECKLINK_SDK_PATH)/DeckLinkAPIDispatch.o \

# This is a basic template for building an executable.
# Define the objects (or subdirectories) it needs.
# Then add the build rule, and place it into all_TARGETS.
test_mjpeg_422_encode_OBJECTS = \
	$(common_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(raw_frame_OBJECTS) \
	tests/mjpeg_422_encode.o

tests/mjpeg_422_encode: $(test_mjpeg_422_encode_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg

all_TARGETS += tests/mjpeg_422_encode    

test_decklink_output_random_OBJECTS = \
	$(common_OBJECTS) \
	$(raw_frame_OBJECTS) \
	$(output_adapter_OBJECTS) \
	tests/decklink_output_random.o

tests/decklink_output_random: $(test_decklink_output_random_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg -ldl -pthread

all_TARGETS += tests/decklink_output_random    

# All generic boilerplate from here on down...

# Include all generated dependency files.
# Since dependency files are always generated before object files, it does
# not matter if a dependency file for a particular source file does not
# exist. Since the object file is therefore also missing, a re-compile
# is forced anyway.
all_DEPS = $(shell find . -iname '*.d')
include $(all_DEPS)

# Generic rule for compiling C++ object files.
%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(EXTERNAL_INCLUDES) $(SUBDIR_INCLUDES) -MM -MF $@.d $^
	$(CXX) $(CXXFLAGS) $(EXTERNAL_INCLUDES) $(SUBDIR_INCLUDES) -c -o $@ $^ 

# And one for assembly files
%.o : %.asm
	$(ASM) -o $@ $^

do_all_targets: $(all_TARGETS)

clean:
	find . -iname '*.o' | xargs rm
	rm $(all_TARGETS)

.PHONY: all do_all_targets clean
