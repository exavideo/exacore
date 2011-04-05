replay_tests_tests_OBJECTS = \
	$(common_OBJECTS) \
    $(thread_OBJECTS) \
    replay/tests/tests.o \
    replay/buffer.o

replay/tests/tests: $(replay_tests_tests_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -lboost_unit_test_framework

all_TARGETS += replay/tests/tests

