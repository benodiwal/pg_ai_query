EXTENSION = pg_ai_query
DATA = sql/pg_ai_query--1.0.sql
MODULES = pg_ai_query

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

BUILD_DIR = build
all: pg_ai_query.so

pg_ai_query.so: $(BUILD_DIR)/CMakeCache.txt
	$(MAKE) -C $(BUILD_DIR)
	cp $(BUILD_DIR)/pg_ai_query.so .

$(BUILD_DIR)/CMakeCache.txt:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DCMAKE_INSTALL_PREFIX=$(shell $(PG_CONFIG) --pkglibdir)

clean:
	rm -rf $(BUILD_DIR) install *.so

.PHONY: all clean