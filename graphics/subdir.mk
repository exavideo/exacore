graphics_OBJECTS = \
	graphics/cairo_frame.o \
	graphics/rsvg_frame.o \
    graphics/freetype_font.o

CXXFLAGS += $(shell pkg-config librsvg-2.0 --cflags)
CXXFLAGS += $(shell pkg-config cairo --cflags)
CXXFLAGS += $(shell freetype-config --cflags)

graphics_LIBS  = $(shell pkg-config librsvg-2.0 --libs)
graphics_LIBS += $(shell pkg-config cairo --libs) 
graphics_LIBS += $(shell freetype-config --libs)
