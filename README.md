# geohash-extra

An extension for PostgreSQL that expands on the existing geohash functions. Generating the neighbours of a geohash and reverting a geometry back into geohashes are both simple functions that are not included in PostGIS, yet they are often slow to do yourself in PLpgSQL. This extension solves both issues with the help of some C language.

## Installation

Installing the extension requires superuser access to the database. Additionaly PostGIS is required for wrapping the functions on the SQL side. To build the source run:

>make

>sudo make install

Next up is installing the extension in Postgres.

>CREATE EXTENSION geohash_extra

## Use

Two additional functions will be added to Postgres.

1. Exploding a geometry in geohashes
2. Finding all neighbours of a geohash

Commands:
>geohashfromgeom(geometry)

>geohash_neighbours(geohash, precision)
