# note-msp430

This is an example of how to use the Notecard and the [note-c][note-c] library
with the native TI MSP430 SDK.

The board that I used for testing is the [TI LaunchPad][board] kit with MSP430 MCU, part number
[MSP-EXP430FR2355][board] which was selected because of its minimalism and the fact that it has
an integrated debug probe.

As a proof-of-concept, this example implements the same functions as the
[note-arduino][note-arduino] library's JSON example.

## Hardware Setup

Before you begin using the software, wire your MSP-EXP430FR2355 development board to the Notecarrier containing
the Notecard.  You may wire it for Serial, I2c, or both.  In order to do so, you'll need some standard female-to-female
header wires to jumper between the boards.
- Connect the Notecarrier's GND pin to any of the MSP's GND pins
- For Serial
  - Remove the two JUMPER PINs from the MSP's TXD>> and RXD<< pins
  - Connect the Notecarrier's RX to the MSP's TXD>> pin, on the side of the jumper further away from the USB connector
  - Connect the Notecarrier's TX to the MSP's RXD<< pin, on the side of the jumper further away from the USB connector
- For I2C
  - Connect the Notecarrier's SDA pin to the MSP's P1.2 pin
  - Connect the Notecarrier's SCL pin to the MSP's P1.3 pin
- Connect both the Notecarrier and MSP to power by using their USB connectors

## Installation of the TI Development Environment

In order to do development, it may be the case that all you need to do is to install the MSP430 SDK as above,
and to install the TI Code Composer Studio (CCS) available [here][ccs] with a free license:

During installation, to optimize disk space, select to only install support for the MSP-EXP430FR2355.

After launching
- In View / App Center, and within App Center select MSP430 Compiler and install it.
- In View / Getting Started / Resource Explorer, select MSP-EXP430FR2355 in the Development Tools box in the upper-left.
- in that left tab, click Software / MSP430Ware / Development Tools / MSP-EXP430FR2355 / Demos / Blink LED
  Then, in the upper right, pick the download icon (rightmost) and download the entire support package for the
  Blink LED example.  After it has been downloaded, click the second-to-right icon that says "Import to IDE"
  command to create a project that will be called BlinkLED_MSP-EXP430FR2355.  When the project is open and
  selected in the Project Explorer, press the small "bug" icon to begin a debug session.  Then, you should be
  able to use the single step or green "go" button to verify that your debug setup is working.

At this point, you know that CCS and your MSP_EXP430FR2355 development board are ready for development.

## Installation of this example

Clone this [note-msp430][note-msp430] repo into the CCS projects folder that you selected during
installation, as follows.  You'll note that the latest copy of the [note-c][note-c] C library is already
loaded by default, as a subdirectory of the [note-msp430][note-msp430] directory.

```
CCS projects folder
├ note-msp430  
  ├ note-c  
```

In CCS, open the note-msp430 project.  Make sure that you edit the "my" definitions at the top of example.c
so that this example will send data to your notehub.io project, and so that it uses serial or I2C as you wish.
By using the standard Debug build configuration, you should be able to build and run the project.

*** NOTE ***

This is a very simple project, and it uses the TI libraries as-is.  However, if you are integrating the
[note-c][note-c] libraries into your own MSP430 project using CCS, you will want to take note of the fact
that the [note-c][note-c] libraries use dynamic heap.  By default, CCS assigns new projects only 160 bytes
of heap, and so you'll need to increase it to at least 2K, with 4K recommended.  (How much heap it uses
will depend upon how large are the JSON objects that you transfer to and from the Notecard.  The Notecard
libraries have good error paths and you'll cleanly notice API call failures if the heap size is insufficient;
an insufficient heap does *not* cause crashes.)

In order to set the heap size in CCS, right click your project name and choose Properties.  Under the
Build / MSP430 Linker / Basic Options, in the section that says "Heap size for C/C++ dynamic memory allocation",
specify your heap size.

[note-msp430]: https://github.com/blues/note-msp430
[note-c]: https://github.com/blues/note-c
[note-arduino]: https://github.com/blues/note-arduino
[ccs]: http://www.ti.com/tool/CCSTUDIO
[board]: http://www.ti.com/tool/MSP-EXP430FR2355
