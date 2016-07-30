\echo Use "CREATE EXTENSION geohash_extra" to load this file. \quit

CREATE FUNCTION geohashfromgeom_ccall(IN double precision, IN double precision, IN double precision, IN double precision, IN int, IN int, IN int, OUT f1 cstring) RETURNS SETOF cstring
AS 'geohash_extra', 'geohashfromgeom' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION geohashfromgeom(geometry, integer) RETURNS SETOF text AS $$
    WITH coord AS (
        SELECT CASE WHEN (ST_SRID($1)) = 4326 THEN
            $1
        ELSE
            ST_Transform($1, 4326)
        END as geom
    )

    SELECT geohashfromgeom_ccall(
        ST_X(ST_Centroid(hash.se)),
        ST_Y(ST_Centroid(hash.se)),
        ST_XMax(hash.se) - ST_XMin(hash.se),
        ST_YMax(hash.se) - ST_YMin(hash.se),
        ((ST_X(ST_Centroid(hash.sw)) - ST_X(ST_Centroid(hash.se))) / (ST_XMax(hash.se) - ST_XMin(hash.se)))::int,
        ((ST_Y(ST_Centroid(hash.ne)) - ST_Y(ST_Centroid(hash.se))) / (ST_YMax(hash.se) - ST_YMin(hash.se)))::int,
        $2::int
    )::text

    FROM (
        SELECT
            ST_GeomFromGeoHash(ST_GeoHash(ST_SetSRID(ST_MakePoint(ST_XMin(coord.geom), ST_YMin(coord.geom)), 4326), $2)) se,
            ST_GeomFromGeoHash(ST_GeoHash(ST_SetSRID(ST_MakePoint(ST_Xmin(coord.geom), ST_YMax(coord.geom)), 4326), $2)) ne,
            ST_GeomFromGeoHash(ST_GeoHash(ST_SetSRID(ST_MakePoint(ST_Xmax(coord.geom), ST_Ymin(coord.geom)), 4326), $2)) sw
        FROM coord
    ) hash;
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE FUNCTION geohash_neighbours(IN text, OUT cstring) RETURNS SETOF cstring
AS 'geohash_extra', 'geohash_neighbours' LANGUAGE C IMMUTABLE STRICT;
