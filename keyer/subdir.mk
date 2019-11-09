keyer_OBJECTS = \
	keyer/character_generator.o \
	keyer/subprocess_character_generator.o \
	keyer/png_subprocess_character_generator.o \
	keyer/svg_subprocess_character_generator.o \
	keyer/js_character_generator.o \
	keyer/js_character_generator_script.o \
	keyer/shm_character_generator.o

keyer_LIBS = -lv8

