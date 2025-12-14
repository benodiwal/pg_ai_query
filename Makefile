EXTENSION = pg_ai_query
DATA = sql/pg_ai_query--1.0.sql
# Note: We don't set MODULES here because we use CMake for compilation, not PGXS

# Build directories (defined before PGXS for EXTRA_CLEAN)
BUILD_DIR = build
TEST_BUILD_DIR = build_tests
TESTS_DIR = tests

# Tell PGXS to clean our build directories
EXTRA_CLEAN = $(BUILD_DIR) $(TEST_BUILD_DIR) install

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

# Determine extension suffix based on PostgreSQL version on macOS
ifeq ($(shell uname),Darwin)
    PG_VERSION := $(shell $(PG_CONFIG) --version | sed 's/PostgreSQL \([0-9]*\).*/\1/')
    ifeq ($(shell test $(PG_VERSION) -ge 16; echo $$?),0)
        EXT_SUFFIX := .dylib
    else
        EXT_SUFFIX := .so
    endif
else
    EXT_SUFFIX := .so
endif

TARGET_LIB := pg_ai_query$(EXT_SUFFIX)

all: $(TARGET_LIB)

$(TARGET_LIB): $(BUILD_DIR)/CMakeCache.txt
	$(MAKE) -C $(BUILD_DIR)
	cp $(BUILD_DIR)/$(TARGET_LIB) .

$(BUILD_DIR)/CMakeCache.txt:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DCMAKE_INSTALL_PREFIX=$(shell $(PG_CONFIG) --pkglibdir)

.PHONY: all

# Formatting
SRC_FILES = $(shell find . -type f \
	\( -name "*.c" -o -name "*.cc" -o -name "*.cpp" \
	   -o -name "*.h" -o -name "*.hpp" \) \
	-not -path "./$(BUILD_DIR)/*" \
	-not -path "./third_party/*")

# Run clang-format in-place
format:
	@echo "Formatting $(words $(SRC_FILES)) file(s)..."
	@clang-format -i -style=file $(SRC_FILES)

# Dry-run: show a diff of what *would* be changed
format-check:
	@echo "Checking formatting..."
	@clang-format -style=file --dry-run --Werror $(SRC_FILES) \
		|| (echo "ERROR: Code is not formatted. Run 'make format'." && exit 1)

.PHONY: format format-check

# =============================================================================
# Testing
# =============================================================================

NPROCS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

## Setup test environment and build test executable
test-setup:
	@echo "Setting up test environment..."
	@mkdir -p $(TEST_BUILD_DIR)
	@cd $(TEST_BUILD_DIR) && cmake .. -DBUILD_TESTS=ON
	@cd $(TEST_BUILD_DIR) && make pg_ai_query_tests -j$(NPROCS)
	@echo "\n✓ Test setup complete. Run 'make test-unit' to run tests."

## Run all tests (unit + PostgreSQL)
test: test-unit test-pg

## Run C++ unit tests
test-unit:
	@if [ ! -f "$(TEST_BUILD_DIR)/tests/pg_ai_query_tests" ]; then \
		echo "Test executable not found. Running test-setup..."; \
		$(MAKE) test-setup; \
	fi
	@echo "\n=== Running Unit Tests ==="
	@cd $(TEST_BUILD_DIR) && ./tests/pg_ai_query_tests
	@echo "\n✓ All unit tests passed."

## Run specific test suite (usage: make test-suite SUITE=ConfigManagerTest)
test-suite:
	@if [ ! -f "$(TEST_BUILD_DIR)/tests/pg_ai_query_tests" ]; then \
		$(MAKE) test-setup; \
	fi
	@cd $(TEST_BUILD_DIR) && ./tests/pg_ai_query_tests --gtest_filter="$(SUITE).*"

## Run PostgreSQL extension tests
test-pg:
	@echo "\n=== Running PostgreSQL Extension Tests ==="
	@if ! pg_isready -q 2>/dev/null; then \
		echo "Error: PostgreSQL is not running."; \
		exit 1; \
	fi
	@DB=$${PGDATABASE:-postgres}; \
	echo "Using database: $$DB"; \
	psql -d $$DB -f $(TESTS_DIR)/sql/setup.sql -q 2>/dev/null || true; \
	psql -d $$DB -f $(TESTS_DIR)/sql/test_extension_functions.sql; \
	psql -d $$DB -f $(TESTS_DIR)/sql/teardown.sql -q 2>/dev/null || true
	@echo "\n✓ PostgreSQL tests complete."

## Clean test build artifacts
test-clean:
	@rm -rf $(TEST_BUILD_DIR)
	@echo "✓ Test build cleaned."

## Show test help
test-help:
	@echo "pg_ai_query - Test Commands"
	@echo "============================"
	@echo ""
	@echo "Setup:"
	@echo "  make test-setup   - Build test executable (runs automatically if needed)"
	@echo ""
	@echo "Running Tests:"
	@echo "  make test-unit    - Run C++ unit tests (91 tests)"
	@echo "  make test-pg      - Run PostgreSQL extension tests"
	@echo "  make test         - Run all tests (unit + pg)"
	@echo ""
	@echo "Advanced:"
	@echo "  make test-suite SUITE=ConfigManagerTest   - Run specific test suite"
	@echo "  make test-suite SUITE=QueryParserTest"
	@echo "  make test-suite SUITE=ProviderSelectorTest"
	@echo "  make test-suite SUITE=ResponseFormatterTest"
	@echo "  make test-suite SUITE=UtilsTest"
	@echo ""
	@echo "Cleanup:"
	@echo "  make test-clean   - Remove test build artifacts"
	@echo ""
	@echo "Environment Variables:"
	@echo "  PGDATABASE        - Database for PostgreSQL tests (default: postgres)"
	@echo ""
	@echo "Examples:"
	@echo "  make test-unit                           # Run all unit tests"
	@echo "  make test-suite SUITE=ConfigManagerTest  # Run one suite"
	@echo "  PGDATABASE=mydb make test-pg             # Run PG tests on 'mydb'"

.PHONY: test test-setup test-unit test-suite test-pg test-clean test-help
