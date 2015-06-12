# lora-parameter-study

## Description

This test library allows you to easily mesure the performance of the [**LoRa**](http://lora-alliance.org/What-Is-LoRa/Technology) technology in fonction of the protocol's radio parameters, so that you can adequately choose them for your project.

By varying one of the following radio parameters (and fixing the others):

* [Code rate](http://en.wikipedia.org/wiki/Code_rate)
* Spreading factor
* Emission power
* Packet size
* Bandwidth

We are then able to mesure and identify its influence over the following performance indicators:

* [Signal-to-noise ration](http://en.wikipedia.org/wiki/Signal-to-noise_ratio)
* Average transmission time
* [Packet loss](http://en.wikipedia.org/wiki/Packet_loss)

The test scripts allow to you to make the measurements both in **uplink** (object to concentrator) and **downlink** (concentrator to object) mode.

Graphics showing the test's results are automatically generated. A `.csv` file with the raw test results is also created. Example results are provided in the folder `xxxxxxxx`.

## Requirements

### Hardware

The tests were built using the following components:

* [SK-iM880A LoRa Starter Kit](http://www.wireless-solutions.de/products/starterkits/sk-im880a): represents a node of the LoRa network. It contains the [iM880A-L module](http://www.wireless-solutions.de/products/radiomodules/im880a), which is based on the [SX1272 LoRa transceiver](http://www.semtech.com/wireless-rf/rf-transceivers/sx1272/) developped by Semtech.
* WiMOD iC880A Concentrator: a LoRa concentrator based on the Semtech SX1301 and SX1257 chips. 

### Software

* Linux machine with the concentrator drivers, which can be installed by following the instructions given in the iC880A quick start guide (steps 3.1 to 3.5).
	* [Python 2.7](https://www.python.org/)
	  * [matplotlib](http://matplotlib.org/index.html): for generating the result graphics
	  * [pandas](http://pandas.pydata.org/): for parsing the csv files
* A Windows machine with [IAR Embedded Workbench](https://www.iar.com/iar-embedded-workbench/): project files for the node's programs are provided. Alternatively, other toolchains may work; however, they haven't been tested and configuration is not provided.

## Usage

A test sequence is made by varying one of the radio parameters and by fixing the others. The ensemble of packet sent with the same parameters in a test sequence is called a test series. The user is free to choose which parameter he wishes to vary, the fixed parameters values and the number of packets sent in each test series. According to the desired test mode, the steps to ajust this parameters are different.

### Uplink

1. Connect the node to the Windows machine in which IAR Workbench is installed. 
2. Open the IAR's project for the node's uplink program, which is located in `lmic-release-v1.4-for-ensimag/lmic-release-v1.4/source/uplink_test/join.eww`.
3. In the first lines of the file `lmic-release-v1.4-for-ensimag/lmic-release-v1.4/source/uplink_test/main.c`:
	1. `#define` the constant corresponding to the desired test: `CRC_TEST`, `POW_TEST`, `CRC_TEST`, `SIZE_TEST` or `BW_TEST`. Only one of them can be defined at a time.
	2. Change the value of the constants prefixed by `FIXED` to choose the fixed parameters of the test. The fixed value of the parameter which will be varied is not considered.
	3. Change `MSGS_PER_SETTING` to choose the number of packets sent in each test series.
4. Connect the concentrator to the Linux machine and execute the `uplink.sh` script located in the `scripts/` folder.
5. When the concentrator is ready to receive the packets, compile and upload the node's code with IAR.
6. After uploading it to the board, press the reset button to start it. The test will now be executed.
7. When it is over, the graphics with the test results will be automatically generated and opened in the Linux machine, and they can then be saved as a image if so desired.

### Downlink

1. Connect the concentrator to the Linux machine and open the `util_pkt_logger.c` file located in the `util_pkt_logger/src` folder.
2. In the first lines of the file:
	1. `#define` the constant corresponding to the desired test: `CRC_TEST`, `POW_TEST`, `CRC_TEST`, `SIZE_TEST` or `BW_TEST`. Only one of them can be defined a a time.
	2. Change `MSGS_PER_SETTING` to choose the number of packets sent in each test series.
3. Use the makefile to compile all the sources and execute the `util_pkt_logger`.
4. Connect the node to the Windows machine in which IAR Workbench is installed. 
5. Open the IAR's project for the node's downlink program, which is located in `lmic-release-v1.4-for-ensimag/lmic-release-v1.4/source/downlink_test/join.eww`.
6. When the concentrator is ready to receive the join packet (and start sending the data packets after this), compile and upload the node's code with IAR.
7. After uploading it to the board, press the reset button to start it. The test will now be executed.
8. Use a program like RS232 [Port Logger](http://www.eltima.com/products/rs232-data-logger/) to transform the serial exit of the node in a txt file (use a baudrate of 115200)
9. Use the `gen_downlink.py` to generate the graphics with the data of the txts files. The results can be saved as a image if so desired.

## Limitations

* Even though the LoRa protocol and the test boards support a 500 kHz bandwith, the bandwith test does not currently implements it.
* It's not possible to use the downlink bandwidth test.

## Credits

* The node's code uses a heavily modified version of [LMIC](https://github.com/mirakonta/lmic). It was changed so that we have more flexibility to choose the radio parameters as we want.
* The concentrator's programs are heavily inspired in the [pkt_logger](https://github.com/Lora-net/lora_gateway/tree/master/util_pkt_logger) example of the [lora_gateway](https://github.com/Lora-net/lora_gateway) project.

## License