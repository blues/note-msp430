#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

//
// This library demonstrates two potential ways of sending JSON to the Notecard - the first
// of which is to simply send commands "raw", and the second of which is to use the Notecard
// JSON libraries.
//
// The former takes *extremely* little memory, but has very little flexibility relative to
// the latter which affords the ability to parse responses from the Notecard.
//
// Note that by using the Notecard library, it is also quite easy to connect the Notecard to
// a Microcontroller's I2C ports (SDA and SCL) rather than using Serial, which is quite
// beneficial in terms of freeing up a serial port.
//

#ifndef DISABLE_NOTE_C_LIBRARY
#define DISABLE_NOTE_C_LIBRARY  false
#endif

// Choose whether to use I2C or SERIAL for the Notecard

#ifndef NOTECARD_USE_I2C
#define NOTECARD_USE_I2C        false
#endif



#define myLiveDemo  true

// Externalized
void delay(uint32_t ms);
long unsigned int millis(void);
void setup(void);
void loop(void);
bool noteSerialReset(void);
void noteSerialTransmit(uint8_t *text, size_t len, bool flush);
bool noteSerialAvailable(void);
char noteSerialReceive(void);
bool noteI2CReset(uint16_t DevAddress);
size_t noteDebugSerialOutput(const char *message);
const char *noteI2CTransmit(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size);
const char *noteI2CReceive(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size, uint32_t *avail);
long unsigned int millis(void);
void delay(uint32_t ms);

#endif // MAIN_H

