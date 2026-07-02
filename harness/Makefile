CC ?= clang
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic
BUILD_DIR ?= build
CORE_INCLUDE = ../Core/include
CORE_SRC = ../Core/src
TESTS = \
	$(BUILD_DIR)/basic_listing_tests \
	$(BUILD_DIR)/game_model_tests \
	$(BUILD_DIR)/random_tests

.PHONY: test clean

test: $(TESTS)
	@set -e; for test in $(TESTS); do ./$$test; done

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/basic_listing_tests: basic_listing_tests.c $(CORE_SRC)/ak_basic_listing.c $(CORE_INCLUDE)/ak_basic_listing.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) basic_listing_tests.c $(CORE_SRC)/ak_basic_listing.c -o $@

$(BUILD_DIR)/game_model_tests: game_model_tests.c $(CORE_SRC)/ak_game.c $(CORE_SRC)/ak_random.c $(CORE_INCLUDE)/ak_game.h $(CORE_INCLUDE)/ak_random.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) game_model_tests.c $(CORE_SRC)/ak_game.c $(CORE_SRC)/ak_random.c -lm -o $@

$(BUILD_DIR)/random_tests: random_tests.c $(CORE_SRC)/ak_random.c $(CORE_INCLUDE)/ak_random.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) random_tests.c $(CORE_SRC)/ak_random.c -o $@

clean:
	rm -rf $(BUILD_DIR)
