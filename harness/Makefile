CC ?= clang
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic
BUILD_DIR ?= build
CORE_INCLUDE = ../Core/include
CORE_SRC = ../Core/src
TESTS = \
	$(BUILD_DIR)/basic_listing_tests

.PHONY: test clean

test: $(TESTS)
	@set -e; for test in $(TESTS); do ./$$test; done

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/basic_listing_tests: basic_listing_tests.c $(CORE_SRC)/ak_basic_listing.c $(CORE_INCLUDE)/ak_basic_listing.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) basic_listing_tests.c $(CORE_SRC)/ak_basic_listing.c -o $@

clean:
	rm -rf $(BUILD_DIR)

