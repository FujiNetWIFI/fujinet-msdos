CXX ?= g++
CC ?= gcc

CPPFLAGS += -Dfar= -I. -I../sys -I../include
CXXFLAGS += -std=c++17 -Wall -Wextra -Werror -O2
CFLAGS += -std=c99 -Wall -Wextra -Werror -O2

BUILD_DIR ?= build
TEST_BIN := $(BUILD_DIR)/fujinet-msdos-tests
OBJS := \
	$(BUILD_DIR)/run_main.o \
	$(BUILD_DIR)/test_nio_protocol.o \
	$(BUILD_DIR)/test_timeout_math.o \
	$(BUILD_DIR)/test_nio_transaction.o \
	$(BUILD_DIR)/test_nio_disk_protocol.o \
	$(BUILD_DIR)/nio_disk_protocol.o \
	$(BUILD_DIR)/nio_protocol.o \
	$(BUILD_DIR)/nio_transaction.o

.PHONY: all test clean

all: $(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/nio_protocol.o: ../sys/nio_protocol.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/nio_disk_protocol.o: ../sys/nio_disk_protocol.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/nio_transaction.o: ../sys/nio_transaction.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)
