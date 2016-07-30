#include "postgres.h"
#include "funcapi.h"
#include "fmgr.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(geohashfromgeom);

static char *base_32 = "0123456789bcdefghjkmnpqrstuvwxyz";

char *geohash_encode(double longitude, double latitude, int precision) {
    int is_even = 1;
    int i = 0;
    int bit = 0;
    int ch = 0;

    double lat[2], lng[2], mid;

    char bits[] = {16, 8, 4, 2, 1};
    char *geohash = NULL;

    geohash = palloc(precision + 1);

    lat[0] = -90.0;
    lat[1] = 90.0;
    lng[0] = -180.0;
    lng[1] = 180.0;

    // encode per char
    while (i < precision) {

        // even chars are longitude
        if (is_even) {
            mid = (lng[0] + lng[1]) / 2;
            if (longitude >= mid) {
                ch |= bits[bit];
                lng[0] = mid;
            }
            else {
                lng[1] = mid;
            }
        }

        // odd chars are latitude
        else {
            mid = (lat[0] + lat[1]) / 2;
            if (latitude >= mid) {
                ch |= bits[bit];
                lat[0] = mid;
            }
            else {
                lat[1] = mid;
            }
        }

        is_even = !is_even;
        if (bit < 4) {
            bit++;
        }
        else {
            geohash[i++] = base_32[ch];
            bit = 0;
            ch = 0;
        }
    }

    geohash[i] = '\0'; // null
    return geohash;
}

Datum geohashfromgeom(PG_FUNCTION_ARGS) {
    FuncCallContext *funcctx;

    float8 x = PG_GETARG_FLOAT8(0);
    float8 y = PG_GETARG_FLOAT8(1);
    float8 width = PG_GETARG_FLOAT8(2);
    float8 height = PG_GETARG_FLOAT8(3);
    float8 fx;

    int call_cntr;
    int max_calls;
    int shared_cnt;
    int xdim = PG_GETARG_INT64(4);
    int ydim = PG_GETARG_INT64(5);
    int precision = PG_GETARG_INT64(6);

    char *hash = palloc(precision + 1);

    Datum result;

    if (SRF_IS_FIRSTCALL()) {
        funcctx = SRF_FIRSTCALL_INIT();

        // init control structs
        funcctx->max_calls = xdim * ydim;
        funcctx->user_fctx = 0;
    }

    // loop with setof return type
    funcctx = SRF_PERCALL_SETUP();
    call_cntr = funcctx->call_cntr;
    max_calls = funcctx->max_calls;

    // user_fctx can store shared memory
    if (funcctx->user_fctx >= ydim) {
        funcctx->user_fctx = 0;
    }
    shared_cnt = funcctx->user_fctx++;

    if (call_cntr < max_calls) {
        fx = floor((float)call_cntr / (float)ydim);

        // encode each cartesion product
        memcpy(hash, geohash_encode(x + (width * fx), y + (height * (float)shared_cnt), precision), precision);
        hash[precision] = '\0';
        result = CStringGetDatum(hash);

        SRF_RETURN_NEXT(funcctx, result);
    }
    else {
        SRF_RETURN_DONE(funcctx);
    }
}

PG_FUNCTION_INFO_V1(geohash_neighbours);

static const char *neighbour[] = {
    "p0r21436x8zb9dcf5h7kjnmqesgutwvy", "bc01fg45238967deuvhjyznpkmstqrwx", //N
    "bc01fg45238967deuvhjyznpkmstqrwx", "p0r21436x8zb9dcf5h7kjnmqesgutwvy", //E
    "14365h7k9dcfesgujnmqp0r2twvyx8zb", "238967debc01fg45kmstqrwxuvhjyznp", //S
    "238967debc01fg45kmstqrwxuvhjyznp", "14365h7k9dcfesgujnmqp0r2twvyx8zb"  //W
};

static const char *border[] = {
    "prxz", "bcfguvyz", //N
    "bcfguvyz", "prxz", //E
    "028b", "0145hjnp", //S
    "0145hjnp", "028b"  //W
};

// inspired by github.com/davetroy/geohash-js
char *geohash_adjacent(char *geohash, int dir) {
    int len = strlen(geohash);

    char *ptr;
    char *parent = palloc(sizeof(char) * len);
    char *result = palloc(sizeof(char) * (len + 1));
    char lastch = geohash[len - 1];

    // substring of geohash
    strncpy(parent, geohash, len - 1);
    parent[len - 1] = '\0';

    // check if lastch is a border tile
    ptr = strchr(border[dir + (len % 2)], lastch);

    // recursive call if the ptr exists
    if (ptr && len != 1) {
        memcpy(parent, geohash_adjacent(parent, dir), len - 1);
        parent[len - 1] = '\0';
    }

    ptr = strchr(neighbour[dir + (len % 2)], lastch);

    memcpy(result, parent, len - 1);
    result[len - 1] = base_32[ptr - neighbour[dir + (len % 2)]];
    result[len] = '\0';

    return result;
}

Datum geohash_neighbours(PG_FUNCTION_ARGS) {
    FuncCallContext *funcctx;

    int call_cntr;
    int max_calls;
    int len;
    int i = 0;

    text *arg = PG_GETARG_TEXT_P(0);

    char *buf = palloc(VARSIZE(arg) - VARHDRSZ);
    char *hash = palloc(VARSIZE(arg) - VARHDRSZ);

    Datum result;

    // transform text to char
    len = VARSIZE(arg) - VARHDRSZ;
    memcpy(buf, VARDATA(arg), len);
    buf[len] = '\0';

    if (SRF_IS_FIRSTCALL()) {
        funcctx = SRF_FIRSTCALL_INIT();

        // check if a viable geohash has been parsed
        for (i; i++ < (len - 1);) {
            if (! strchr(base_32, buf[i])) {
                PG_RETURN_NULL();
            }
        }

        // init control struct
        funcctx->max_calls = 8;
    }

    // loop with setof return type
    funcctx = SRF_PERCALL_SETUP();
    call_cntr = funcctx->call_cntr;
    max_calls = funcctx->max_calls;

    if (call_cntr < max_calls) {
        switch(call_cntr) {
            case 0:
                memcpy(hash, geohash_adjacent(buf, 0), len);
                break;
            case 1:
                memcpy(hash, geohash_adjacent(geohash_adjacent(buf, 0), 2), len);
                break;
            case 2:
                memcpy(hash, geohash_adjacent(buf, 2), len);
                break;
            case 3:
                memcpy(hash, geohash_adjacent(geohash_adjacent(buf, 4), 2), len);
                break;
            case 4:
                memcpy(hash, geohash_adjacent(buf, 4), len);
                break;
            case 5:
                memcpy(hash, geohash_adjacent(geohash_adjacent(buf, 4), 6), len);
                break;
            case 6:
                memcpy(hash, geohash_adjacent(buf, 6), len);
                break;
            case 7:
                memcpy(hash, geohash_adjacent(geohash_adjacent(buf, 0), 6), len);
        }
        hash[len] = '\0';

        result = CStringGetDatum(hash);
        SRF_RETURN_NEXT(funcctx, result);
    }
    else {
        SRF_RETURN_DONE(funcctx);
    }
}
