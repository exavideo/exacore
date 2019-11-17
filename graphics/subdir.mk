graphics_OBJECTS = \
	graphics/cairo_frame.o \
	graphics/rsvg_frame.o \
    graphics/freetype_font.o

EXTERNAL_INCLUDES += $(shell pkg-config librsvg-2.0 --cflags)
EXTERNAL_INCLUDES += $(shell pkg-config cairo --cflags)
EXTERNAL_INCLUDES += $(shell pkg-config freetype2 --cflags)

graphics_LIBS  = $(shell pkg-config librsvg-2.0 --libs)
graphics_LIBS += $(shell pkg-config cairo --libs) 
graphics_LIBS += $(shell pkg-config freetype2 --libs)
