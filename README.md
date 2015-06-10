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

The test scripts allow to you to make the mesurements both in **uplink** (object to concentrator) and **downlink** (concentrator to object) mode.

Graphics showing the test's results are automatically generated. A raw `.csv` file with the raw test results is also created. Example results are provided in the folder `xxxxxxxx`.

## Requirements

### Hardware

The tests were built using the following components:

* [SK-iM880A LoRa Starter Kit](http://www.wireless-solutions.de/products/starterkits/sk-im880a): represents a node of the LoRa network. It contains the [iM880A-L module](http://www.wireless-solutions.de/products/radiomodules/im880a), which is based on the [SX1272 LoRa transceiver](http://www.semtech.com/wireless-rf/rf-transceivers/sx1272/) developped by Semtech.
* WiMOD iC880A Concentrator: a LoRa concentrator based on the Semtech SX1301 and SX1257 chips. 

### Software

* [IAR Embedded Workbench](https://www.iar.com/iar-embedded-workbench/): project files for the node's programs are provided. Alternatively, other toolchains may work; however, they haven't been tested and configuration is not provided.
* [Python 2.7](https://www.python.org/)
  * [matplotlib](http://matplotlib.org/index.html)
  * [pandas](http://pandas.pydata.org/)

## Usage

## Limitations