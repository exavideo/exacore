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
	-I drivers/ \
	-I graphics/ \

include $(shell find . -iname 'subdir.mk')
include $(shell find . -iname 'targets.mk')

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
	rm -f $(all_TARGETS)

.PHONY: all do_all_targets clean
