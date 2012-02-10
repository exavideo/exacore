# Adjust these to match your CPU and compiler configuration.
# Defaults should work well for a 64-bit build on AMD hardware. 
# 32-bit is not supported for the time being!

-include local.mk

SWIG=swig
CXX=g++
CXXFLAGS=-g $(LOCAL_CFLAGS) -W -Wall -Werror -DRAWFRAME_POSIX_IO -fPIC 

# don't use -Werror for swig-generated code
SWIG_CXXFLAGS=-g -rdynamic $(LOCAL_CFLAGS) -W -Wall -DRAWFRAME_POSIX_IO -fPIC
LDFLAGS=-g -rdynamic -ltcmalloc $(LOCAL_LDFLAGS)
RUBY_INCLUDES=`ruby ruby_cflags.rb`
ASM=yasm -f elf64 -g dwarf2


all: do_all_targets

EXTERNAL_INCLUDES += \
	-I$(DECKLINK_SDK_PATH) \

SUBDIR_INCLUDES = \
	-Imjpeg/ \
	-Icommon/ \
	-Iraw_frame/ \
	-Ithread/ \
	-Idrivers/ \
	-Igraphics/ \
	-Ireplay/ \
	-Iavspipe/ \
	-Idisplay_surface \

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

ifeq ($(SKIP_X86_64_ASM),1)
	CXXFLAGS += -DSKIP_ASSEMBLY_ROUTINES
else
	CXXFLAGS += -DX86_64
endif

# Generic rule for compiling C++ object files.
%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(EXTERNAL_INCLUDES) $(SUBDIR_INCLUDES) -MM -MF $@.d $^
	$(CXX) $(CXXFLAGS) $(EXTERNAL_INCLUDES) $(SUBDIR_INCLUDES) -c -o $@ $^ 

%.rbo: %.rbcpp
	$(CXX) $(SWIG_CXXFLAGS) $(EXTERNAL_INCLUDES) $(SUBDIR_INCLUDES) $(RUBY_INCLUDES) -c -o $@ -x c++ $^

%.rbcpp : %.i
	$(SWIG) $(SUBDIR_INCLUDES) -Wall -c++ -ruby -o $@ $^

# And one for assembly files
%.o : %.asm
	$(ASM) -o $@ $^

do_all_targets: $(all_TARGETS)

clean:
	-find . -iname '*.o' | xargs rm
	-find . -iname '*.rbo' | xargs rm
	-find . -iname '*.rbcpp' | xargs rm
	-rm -f $(all_TARGETS)

.PHONY: all do_all_targets clean
