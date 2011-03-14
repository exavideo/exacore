CXX=g++
CXXFLAGS=-g -O3 -march=k8 -W -Wall
LDFLAGS=-g -O3 -march=k8

SUBDIR_INCLUDES = \
	-I mjpeg/ \
	-I common/ \
	-I raw_frame/ \

common_OBJECTS = \
	common/xmalloc.o \
	common/posix_util.o \

mjpeg_OBJECTS = \
	mjpeg/libjpeg_glue.o \
	mjpeg/mjpeg_encode.o \

raw_frame_OBJECTS = \
	raw_frame/raw_frame.o \
	raw_frame/convert/uyvy_YCbCr422p.o \

test_mjpeg_422_encode_OBJECTS = \
	$(common_OBJECTS) \
	$(mjpeg_OBJECTS) \
	$(raw_frame_OBJECTS) \
	mjpeg/libjpeg_glue.o \
	mjpeg/mjpeg_encode.o \

all_DEPS = `find . -iname '*.d'`

#-include $(all_DEPS)

tests/mjpeg_422_encode: $(test_mjpeg_422_encode_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -ljpeg
    
%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(SUBDIR_INCLUDES) -MM -MF $@.d $^
	$(CXX) $(CXXFLAGS) $(SUBDIR_INCLUDES) -c -o $@ $^ 

