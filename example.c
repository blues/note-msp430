// Copyright 2019 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"

// This example uses the "note-c" library for JSON and Notecard I/O
#if !DISABLE_NOTE_C_LIBRARY
#include "note.h"

// JSON example
void setup() {

    // Register callbacks with note-c subsystem that it needs for I/O, memory, timer
    NoteSetFn(malloc, free, delay, millis);

    // Register callbacks for Notecard I/O, or just do the initialization
#if NOTECARD_USE_I2C
    NoteSetFnI2C(NOTE_I2C_ADDR_DEFAULT, NOTE_I2C_MAX_DEFAULT, noteI2CReset, noteI2CTransmit, noteI2CReceive);
#else
    NoteSetFnSerial(noteSerialReset, noteSerialTransmit, noteSerialAvailable, noteSerialReceive);
#endif

    // "NoteNewRequest()" uses the bundled "J" json package to allocate a "req", which is a JSON object
    // for the request to which we will then add Request arguments.  The function allocates a "req"
    // request structure using malloc() and initializes its "req" field with the type of request.
    J *req = NoteNewRequest("hub.set");

    // This command causes the data to be delivered to the Project on notehub.io that has claimed
    // this Product ID.  (see above)
#ifdef PRODUCT_UID
    if (PRODUCT_UID[0]) {
        JAddStringToObject(req, "product", PRODUCT_UID);
    }
#endif
    // This command determines how often the Notecard connects to the service.  If "continuous" the Notecard
    // immediately establishes a session with the service at notehub.io, and keeps it active continuously.
    // Because of the power requirements of a continuous connection, a battery powered device would instead
    // only sample its sensors occasionally, and would only upload to the service on a periodic basis.
#if myLiveDemo
    JAddStringToObject(req, "mode", "continuous");
#else
    JAddStringToObject(req, "mode", "periodic");
    JAddNumberToObject(req, "outbound", 60);
#endif

    // Issue the request, telling the Notecard how and how often to access the service.
    // This results in a JSON message to Notecard formatted like:
    //     { "req"     : "hub.set",
    //       "product" : PRODUCT_UID,
    //       "mode"    : "continuous"
    //     }
    // Note that NoteRequest() always uses free() to release the request data structure, and it
    // returns "true" if success and "false" if there is any failure.
    NoteRequest(req);

}

// Arduino-like loop
void loop() {

    // Simulate an event counter of some kind
    static unsigned eventCounter = 0;
    eventCounter = eventCounter + 1;

    // Rather than simulating a temperature reading, use a Notecard request to read the temp
    // from the Notecard's built-in temperature sensor.  We use NoteRequestResponse() to indicate
    // that we would like to examine the response of the transaction.  This method takes a "request" JSON
    // data structure as input, then processes it and returns a "response" JSON data structure with
    // the response.  Note that because the Notecard library uses malloc(), developers must always
    // check for NULL to ensure that there was enough memory available on the microcontroller to
    // satisfy the allocation request.
    JNUMBER temperature = 0;
    J *rsp = NoteRequestResponse(NoteNewRequest("card.temp"));
    if (rsp != NULL) {
        temperature = JGetNumber(rsp, "value");
        NoteDeleteResponse(rsp);
    }

    // Do the same to retrieve the voltage that is detected by the Notecard on its V+ pin.
    JNUMBER voltage = 0;
    rsp = NoteRequestResponse(NoteNewRequest("card.voltage"));
    if (rsp != NULL) {
        voltage = JGetNumber(rsp, "value");
        NoteDeleteResponse(rsp);
    }

    // Enqueue the measurement to the Notecard for transmission to the Notehub, adding the "start"
    // flag for demonstration purposes to upload the data instantaneously, so that if you are looking
    // at this on notehub.io you will see the data appearing 'live'.)
    J *req = NoteNewRequest("note.add");
    if (req != NULL) {
        JAddStringToObject(req, "file", "sensors.qo");
#if myLiveDemo
        JAddBoolToObject(req, "start", true);
#endif
        J *body = JCreateObject();
        if (body != NULL) {
            JAddNumberToObject(body, "temp", temperature);
            JAddNumberToObject(body, "voltage", voltage);
            JAddNumberToObject(body, "count", eventCounter);
            JAddItemToObject(req, "body", body);
        }
        NoteRequest(req);
    }

    // Delay between measurements
#if myLiveDemo
    delay(15*1000);     // 15 seconds
#else
    delay(15*60*1000);  // 15 minutes
#endif
}

#endif  // !DISABLE_NOTE_C_LIBRARY
