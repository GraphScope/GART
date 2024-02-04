EXTENSION = gart
MODULE_big = gart
DATA = gart--0.0.1.sql
OBJS = gart.o utility.o

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
