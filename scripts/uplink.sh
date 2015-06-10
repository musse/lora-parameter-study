#!/bin/bash

cd ../lora_gateway/uplink/
./uplink_concentrator -r results.txt
python ../../python/gen_uplink.py results.txt