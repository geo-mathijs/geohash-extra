EXTENSION = geohash_extra
DATA = geohash_extra--0.0.1.sql
MODULES = geohash_extra

# postgres build stuff
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
