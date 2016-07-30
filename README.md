# geohash-extra

An extension for PostgreSQL that expands on the existing geohash functions.

## Installation

Installing the extension requires superuser access to the database. Additionaly PostGIS is required for wrapping the functions on the SQL side. To build the source run:

>make

>sudo make install

Next up is installing the extension in Postgres.

>CREATE EXTENSION geohash-extra

## Use

Two additional functions will be added to Postgres.

1. Exploding a geometry in geohashes
2. Finding all neighbours of a geohash

Commands:
>geohashfromgeom(geometry)

>geohash_neighbours(geohash, precision)