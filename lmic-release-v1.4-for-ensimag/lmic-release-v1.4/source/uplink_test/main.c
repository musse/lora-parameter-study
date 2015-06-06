/*******************************************************************************
 * Copyright (c) 2014-2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Zurich Research Lab - initial API, implementation and documentation
 *******************************************************************************/

#include "lmic.h"
#include "debug.h"
#include "id.h"

// CONSTANTS AND MACROS

// Uncomment desired test

/*
#define UPDATE_TEST() testPow()
#define TEST_STRING "POW"
*/

/*
#define UPDATE_TEST() testBw()
#define TEST_STRING "BW"
*/

/*
#define UPDATE_TEST() testSF()
#define TEST_STRING "SF"
*/

/*
#define UPDATE_TEST() testCRC()
#define TEST_STRING "CRC"
*/

/*
#define UPDATE_TEST() testPacketSize()
#define TEST_STRING "SIZE"
*/

#define FIXED_CRC       CR_4_5
#define FIXED_DR        DR_SF7
#define FIXED_SF        SF7
#define FIXED_POW       14
#define FIXED_BW        BW125
#define FIXED_FREQ      EU868_F6
#define FIXED_SIZE      8

#define MSGS_PER_SETTING 5

#define ARRAY_FILLER    0x33 // value for filling unused tx array positions
#define TEST_FINISHED   0xFF

#define TX_PORT         1
#define TX_REQ_ACK      0

#define INITIAL_POW     2
#define POW_INC         3
#define MAX_POW         17 

#define INITIAL_SIZE    4
#define SIZE_INC        8
#define MAX_SIZE        47

#define bool            u1_t
#define false           0
#define true            1

//////////////////////////////////////////////////
// CONFIGURATION (FOR APPLICATION CALLBACKS BELOW)
//////////////////////////////////////////////////


//////////////////////////////////////////////////
// APPLICATION CALLBACKS
//////////////////////////////////////////////////

// provide application router ID (8 bytes, LSBF)
void os_getArtEui (u1_t* buf) {
    memcpy(buf, APPEUI, 8);
}

// provide device ID (8 bytes, LSBF)
void os_getDevEui (u1_t* buf) {
    memcpy(buf, DEVEUI, 8);
}

// provide device key (16 bytes)
void os_getDevKey (u1_t* buf) {
    memcpy(buf, DEVKEY, 16);
}


//////////////////////////////////////////////////
// MAIN - INITIALIZATION AND STARTUP
//////////////////////////////////////////////////

// initial job
static void initfunc (osjob_t* j) {
    // reset MAC state
    LMIC_reset();
    // start joining
    LMIC_startJoining();
    // init done - onEvent() callback will be invoked...
}

// application entry point
int main () {
    osjob_t initjob;

    // initialize runtime env
    os_init();
    // initialize debug library
    debug_init();
    // setup initial job
    os_setCallback(&initjob, initfunc);
    // execute scheduled jobs and events
    os_runloop();
    // (not reached)
    return 0;
}


//////////////////////////////////////////////////
// UTILITY JOB
//////////////////////////////////////////////////

static osjob_t blinkjob;
static u1_t ledstate = 0;
bool testEnded = false;

static void blinkfunc (osjob_t* j) {
    // toggle LED
    ledstate = !ledstate;
    debug_led(ledstate);
    // reschedule blink job
    os_setTimedCallback(j, os_getTime()+ms2osticks(100), blinkfunc);
}


void currentTestEnd () {
    LMIC.message_type = END_MESSAGE;

    u1_t sizeToSend = 0;
    os_copyMem(LMIC.pendTxData + sizeToSend, &LMIC.errcr, sizeof(LMIC.errcr));
    sizeToSend += sizeof(LMIC.errcr);

    //enum _sf_t sf = getSf(LMIC.rps);

    os_copyMem(LMIC.pendTxData + sizeToSend, &LMIC.datarate, sizeof(LMIC.datarate));
    sizeToSend += sizeof(LMIC.datarate);

    u1_t bw = getBw(LMIC.tx_rps);
    os_copyMem(LMIC.pendTxData + sizeToSend, &bw, sizeof(bw));
    sizeToSend += sizeof(bw);

    os_copyMem(LMIC.pendTxData + sizeToSend, &LMIC.txpow, 1); // power is represented by 1 byte
    sizeToSend += sizeof(s1_t);

    /*
    debug_val("coderate = ", LMIC.errcr);
    debug_val("SF = ", LMIC.datarate);
    debug_val("BW = ", bw);
    debug_val("pow = ", LMIC.txpow);
    debug_val("LMIC.coderate = ", *LMIC.pendTxData);
    debug_val("LMIC.SF = ", *(LMIC.pendTxData+sizeof(s1_t)));
    debug_val("LMIC.BW = ", *(LMIC.pendTxData+2*sizeof(s1_t)));
    debug_val("LMIC.pow = ", *(LMIC.pendTxData+3*sizeof(s1_t)));
    */

    // sends message
    LMIC.pendTxLen = sizeToSend;
    LMIC.pendTxConf = TX_REQ_ACK;
    LMIC.pendTxPort = TX_PORT;
    LMIC_setTxData();
}

u1_t testBw () {

    static u1_t currentSettings = 0;
    static enum _bw_t currentBw;

    currentSettings++;

    switch (currentSettings) {
        case 1:
            currentBw = BW125; debug_str("BW125"); break;
        case 2:
            currentBw = BW250; debug_str("BW250"); break;
        case 3:
            currentBw = BW500; debug_str("BW500"); break;
        default:
            return TEST_FINISHED;
    }

    setTxParameters(FIXED_CRC, FIXED_DR, FIXED_SF, FIXED_POW, currentBw,
        FIXED_FREQ);

    return FIXED_SIZE;
}

u1_t testPow () {

    static s1_t currentPow = INITIAL_POW;
    static bool firstRun = true;

    if (firstRun)
        firstRun = false;
    else
        currentPow += POW_INC;

    if (currentPow > MAX_POW)
        return TEST_FINISHED;
    else
        debug_hex((u1_t)currentPow);

    setTxParameters(FIXED_CRC, FIXED_DR, FIXED_SF, currentPow, FIXED_BW,
        FIXED_FREQ);
    
    return FIXED_SIZE;
}

u1_t testSF () {

    static u1_t currentSettings = 0;
    static enum _sf_t currentSF;
    static enum _dr_eu868_t currentDR;

    currentSettings++;

    switch (currentSettings) {
        case 1:
            currentSF = SF7; currentDR = DR_SF7; debug_str("SF7"); break;
        case 2:
            currentSF = SF8; currentDR = DR_SF8; debug_str("SF8"); break;
        case 3:
            currentSF = SF9; currentDR = DR_SF9; debug_str("SF9"); break;
        case 4:
            currentSF = SF10; currentDR = DR_SF10; debug_str("SF10"); break;
        case 5:
            currentSF = SF11; currentDR = DR_SF11; debug_str("SF11"); break;
        case 6:
            currentSF = SF12; currentDR = DR_SF12; debug_str("SF12"); break;
        default:
            return TEST_FINISHED;
    }

    setTxParameters(FIXED_CRC, currentDR, currentSF, FIXED_POW, FIXED_BW,
        FIXED_FREQ);

    return FIXED_SIZE;
}

u1_t testCRC () {

    static u1_t currentSettings = 0;
    static enum _cr_t currentCRC;

    currentSettings++;

    switch (currentSettings) {
        case 1:
            currentCRC = CR_4_5; debug_str("CR_4_5"); break;
        case 2:
            currentCRC = CR_4_6; debug_str("CR_4_6"); break;
        case 3:
            currentCRC = CR_4_7; debug_str("CR_4_7"); break;
        case 4:
            currentCRC = CR_4_8; debug_str("CR_4_8"); break;
        default:
            return TEST_FINISHED;
    }

    setTxParameters(currentCRC, FIXED_DR, FIXED_SF, FIXED_POW, FIXED_BW,
        FIXED_FREQ);

    return FIXED_SIZE;
}

u1_t testPacketSize () {
    
    static bool firstRun = true;
    static u1_t currentSize = INITIAL_SIZE;

    if (firstRun)
        firstRun = false;
    else
        currentSize += SIZE_INC;

    if (currentSize > MAX_SIZE)
        return TEST_FINISHED;
    else
        debug_hex(currentSize);

    return currentSize;
}

void sendTestMessage (void) {
    
    static bool firstCall = true;
    static u1_t currentSettingsCount = 0;

    // if first call of the function, set tx fixed parameters
    if (firstCall) {

        debug_str("\r\n");
        debug_str(TEST_STRING);
        debug_str(" : starting test.\r\n");
        firstCall = false;
        setTxParameters(FIXED_CRC, FIXED_DR, FIXED_SF, FIXED_POW,
        FIXED_BW, FIXED_FREQ);
    }

    static u1_t sizeToSend;

    if (currentSettingsCount == MSGS_PER_SETTING) {
        currentSettingsCount = 0;
        currentTestEnd();
    } else {
        if (currentSettingsCount == 0) {
            
            debug_str("\r\n");
            sizeToSend = UPDATE_TEST();
                        
            if (sizeToSend != TEST_FINISHED) {
                debug_str(" : changed parameter.\r\n");
                LMIC.message_type = TEST_MESSAGE;
                // initialise tx array
                for (u1_t i = 0; i < MAX_SIZE; i++) {
                    LMIC.pendTxData[i] = ARRAY_FILLER;
                }
            }
        }

        if (sizeToSend == TEST_FINISHED) {
            testEnded = true;
            debug_str("\r\n");
            debug_str(TEST_STRING);
            debug_str(" : test has finished.\r\n");
        } else {
            debug_hex(currentSettingsCount);
            debug_str(" ");
            // sends message
            LMIC.pendTxData[0] = currentSettingsCount;
            LMIC.pendTxLen  = sizeToSend;
            LMIC.pendTxConf = TX_REQ_ACK;
            LMIC.pendTxPort = TX_PORT;    
            LMIC_setTxData();
            currentSettingsCount++;
        }
    }
}

//////////////////////////////////////////////////
// LMIC EVENT CALLBACK
//////////////////////////////////////////////////

void onEvent (ev_t ev) {
    //debug_event(ev);

    switch(ev) {
        // starting to join network
        case EV_JOINING:
            debug_str("Started joining.\r\n");
            // start blinking
            blinkfunc(&blinkjob);
            break;

        case EV_JOINED:
            // join complete
            debug_str("Joined.\r\n");
            debug_led(1);
            os_clearCallback(&blinkjob);
            sendTestMessage();
            break;

        // network joined, session established
        case EV_TXCOMPLETE:
            if (testEnded) {
                break;
            }
            if (LMIC.dataLen) { // data received in rx slot after tx
                //debug_str("Received response message:\r\n");
                //debug_buf(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                //debug_str("Sending message.\r\n");
                //sendNextMessage();
            } else {
                // nothing received after sending something
                //debug_str("No message received after sending data.\r\n");
            }
            sendTestMessage();         
            break;
    }
}
