#!/bin/bash

cd ../lora_gateway/
make
cd uplink/
./uplink_concentrator -r results.csv
python ../../python/gen_uplink.py results.csv