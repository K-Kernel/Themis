#!/bin/sh
# Downloads the OpenFlights datasets into data/real/ (gitignored).
set -e
cd "$(dirname "$0")/../.."
mkdir -p data/real
curl -sSL -o data/real/airports.dat https://raw.githubusercontent.com/jpatokal/openflights/master/data/airports.dat
curl -sSL -o data/real/routes.dat https://raw.githubusercontent.com/jpatokal/openflights/master/data/routes.dat
