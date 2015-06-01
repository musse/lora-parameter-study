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

static void blinkfunc (osjob_t* j) {
    // toggle LED
    ledstate = !ledstate;
    debug_led(ledstate);
    // reschedule blink job
    os_setTimedCallback(j, os_getTime()+ms2osticks(100), blinkfunc);
}


void sendNextMessage() {
    static u1_t numberToSend = 0;
    LMIC.frame[0] = numberToSend;
    numberToSend++;
    debug_val("Sending message: ", numberToSend);
    // schedule transmission (port 1, datalen 1, no ack requested)
    LMIC_setTxData2(1, LMIC.frame, 1, 0);
    // (will be sent as soon as duty cycle permits)
}

//////////////////////////////////////////////////
// LMIC EVENT CALLBACK
//////////////////////////////////////////////////

void onEvent (ev_t ev) {
    debug_event(ev);

    switch(ev) {
        // starting to join network
        case EV_JOINING:
            debug_str("Started joining.\r\n");
            // start blinking
            blinkfunc(&blinkjob);

            break;

        case EV_JOINED:
            // join complete
            debug_str("Joined, sending first message.\r\n");
            debug_led(1);
            os_clearCallback(&blinkjob);

            // disables all channels but the tx one
            for (u1_t i = 0; i <= 5; i++)
                if (i != TX_CHANNEL)
                    LMIC_disableChannel(i);
            
            sendNextMessage();

            break;

        // network joined, session established
        case EV_TXCOMPLETE:

            if (LMIC.dataLen) { // data received in rx slot after tx
                debug_str("Received response message:\r\n");
                debug_buf(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                //debug_str("Sending message.\r\n");
                //sendNextMessage();
            } else {
                // nothing received after sending something
                debug_str("No message received after transmission, sending one more message.\r\n");
            }
            sendNextMessage();

            break;
    }
}
