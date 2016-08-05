keydaemon_OBJECTS = \
	keydaemon/character_generator.o \
	keydaemon/subprocess_character_generator.o \
	keydaemon/png_subprocess_character_generator.o \
	keydaemon/svg_subprocess_character_generator.o \
	keydaemon/js_character_generator.o \
	keydaemon/js_character_generator_script.o \
	keydaemon/shm_character_generator.o

keydaemon_LIBS = -lv8

