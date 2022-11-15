// Copyright 2019 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include <string.h>
#include "main.h"
#include "note.h"

// This example avoids using the "note-c" library in its entirety, for extremely low-memory MCUs.  It is only
// written for Serial because I2C requires a "serial-over-i2c" protocol that is implemented within the library.
#if DISABLE_NOTE_C_LIBRARY && !NOTECARD_USE_I2C

// Forwards
void my_itoa(int dataIn, char* bffr, int radix);

// JSON example
void setup() {

    // Initialize I/O
    noteSerialReset();

    // The request field sets up service operation parameters
#define F_REQ "\"req\":\"hub.set\""

    // This command (required) causes the data to be delivered to the Project on notehub.io that has claimed
    // this Product ID.  (see above)
#define F_PRODUCT "\"product\":\"" myProductID "\""

    // This command determines how often the Notecard connects to the service.  If "continuous" the Notecard
    // immediately establishes a session with the service at notehub.io, and keeps it active continuously.
    // Because of the power requirements of a continuous connection, a battery powered device would instead
    // only sample its sensors occasionally, and would only upload to the service on a periodic basis.
#if myLiveDemo
#define F_MODE "\"mode\":\"continuous\""
#else
#define F_MODE "\"mode\":\"periodic\",\"outbound\":60"
#endif

    // Issue the request, telling the Notecard how and how often to access the service.
    // This results in a JSON message to Notecard formatted like:
    //     { "req"     : "hub.set",
    //       "product" : myProductID,
    //       "mode"    : "continuous"
    //     }
    char *request = "{" F_REQ "," F_PRODUCT "," F_MODE "}\n";
    noteSerialTransmit((uint8_t *)request, strlen(request), true);

}

// Arduino-like loop
void loop() {

    // Simulate an event counter of some kind
    static unsigned eventCounter = 0;
    eventCounter = eventCounter + 1;

    // Begin the request
#if myLiveDemo
    char *request = "{\"req\":\"note.add\",\"start\":true,\"file\":\"sensors.qo\",\"body\":{";
#else
    char *request = "{\"req\":\"note.add\",\"file\":\"sensors.qo\",\"body\":{";
#endif
    noteSerialTransmit((uint8_t *)request, strlen(request), false);

    // Output the data field
    request = "\"count\":";
    noteSerialTransmit((uint8_t *)request, strlen(request), false);
    char countstr[20];
    my_itoa(eventCounter, countstr, 10);
    noteSerialTransmit((uint8_t *)countstr, strlen(countstr), false);

    // Complete and issue the request
    request = "}}\n";
    noteSerialTransmit((uint8_t *)request, strlen(request), true);

    // Delay between measurements
#if myLiveDemo
    delay(15*1000);     // 15 seconds
#else
    delay(15*60*1000);  // 15 minutes
#endif
}

// int to string
void my_itoa(int dataIn, char* bffr, int radix) {
    int temp_dataIn;
    temp_dataIn = dataIn;
    int stringLen=1;
    while ((int)temp_dataIn/radix != 0){
        temp_dataIn = (int)temp_dataIn/radix;
        stringLen++;
    }
    temp_dataIn = dataIn;
    do {
        *(bffr+stringLen-1) = (temp_dataIn%radix)+'0';
        temp_dataIn = (int) temp_dataIn / radix;
    } while(stringLen--);
}

#endif  // DISABLE_NOTE_C_LIBRARY
