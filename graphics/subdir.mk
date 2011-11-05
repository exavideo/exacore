graphics_OBJECTS = \
	graphics/cairo_frame.o \
	graphics/rsvg_frame.o \
    graphics/freetype_font.o

EXTERNAL_INCLUDES += $(shell pkg-config librsvg-2.0 --cflags)
EXTERNAL_INCLUDES += $(shell pkg-config cairo --cflags)
EXTERNAL_INCLUDES += $(shell freetype-config --cflags)

graphics_LIBS  = $(shell pkg-config librsvg-2.0 --libs)
graphics_LIBS += $(shell pkg-config cairo --libs) 
graphics_LIBS += $(shell freetype-config --libs)
