graphics_OBJECTS = \
	graphics/cairo_frame.o \
	graphics/rsvg_frame.o \

CXXFLAGS += $(shell pkg-config librsvg-2.0 --cflags)
CXXFLAGS += $(shell pkg-config cairo --cflags)

graphics_LIBS  = $(shell pkg-config librsvg-2.0 --libs)
graphics_LIBS += $(shell pkg-config cairo --libs) 

