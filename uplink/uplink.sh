#!/bin/bash

cd concentrator
make
cd uplink/
./uplink_concentrator -r results.csv
python ../../gen_uplink.py results.csv