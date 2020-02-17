#include "mmw_output.h"
#include "stdint.h"
#include "wiringPi.h"
#include "wiringSerial.h"
#include <iostream>

constexpr int BAUD = 921600;

void setup();
void waitForMagicWord();
void loadHeader();
void printHeader();

static int fd;
static const char magicWord[8] = { 0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07 };
static MmwDemo_output_message_header header;
int main(int argc, char* argv[])
{
    setup();
    // Align on magic word.
    waitForMagicWord();
    std::cout << "Header Found!\n";
    // Parse into structs.
    loadHeader();
    // Print out info.
    printHeader();
    return 0;
}

void setup()
{
    fd = serialOpen("/dev/serial1", BAUD);
}

void waitForMagicWord()
{
    int mw_offset = 0;
    while (mw_offset != sizeof(magicWord)) {
        int next_byte = serialGetchar(fd);
        if (next_byte == magicWord[mw_offset]) {
            mw_offset++;
        } else {
            mw_offset = 0;
        }
    }
}

void loadHeader()
{
    uint8_t* start = (uint8_t*)&header.version;
    while (start < (((uint8_t *)&header.numTLVs) + sizeof(header.numTLVs))) {
        int next_byte = serialGetchar(fd);
        *(start++) = next_byte;
    }
    //Endianness ??
}

void printHeader() {
    printf("Version %d, totalPacketLen %d, numDetectedObj %d, numTLVs %d",
            header.version,
            header.totalPacketLen,
            header.numDetectedObj,
            header.numTLVs);
}
