#include "stdint.h"
#include "mmw_output.h"
#include "wiringPi.h"
#include "wiringSerial.h"
#include "detected_obj.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

constexpr int BAUD_RX = 921600;
constexpr int BAUD_TX = 115200;

void startSensor();
void setup();
void waitForMagicWord();
void loadHeader();
void printHeader();
void processTLVs();

static int fd;
static const char magicWord[8] = { 0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07 };
static MmwDemo_output_message_header header;
int main(int argc, char* argv[])
{
    startSensor();
    setup();
    while (1) {
        // Align on magic word.
        waitForMagicWord();
        std::cout << "Header Found!\n";
        // Parse into structs.
        loadHeader();
        // Print out info.
        printHeader();
        processTLVs();
        std::cout <<"\n-------------------\n"; // Break between outputs.
    }
    return 0;
}

void setup()
{
    fd = serialOpen("/dev/serial0", BAUD_RX);
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
    while (start < (((uint8_t*)&header.numTLVs) + sizeof(header.numTLVs))) {
        int next_byte = serialGetchar(fd);
        *(start++) = next_byte;
    }
    //Endianness ??
}

void printHeader()
{
    printf("Seq Num: %d,Version %d, totalPacketLen %d, numDetectedObj %d, numTLVs %d\n",
        header.frameNumber,
        header.version,
        header.totalPacketLen,
        header.numDetectedObj,
        header.numTLVs);
}

void processTLVs()
{
    for (int tlv_num = 0; tlv_num < header.numTLVs; tlv_num++) {
        MmwDemo_output_message_tl tag_length;
        auto ptr = (uint8_t*)&tag_length;
        for (int i = 0; i < sizeof(tag_length); ++i) {
            int next_byte = serialGetchar(fd);
            *(ptr++) = next_byte;
        }
        printf("Tag %d, Length %d\n", tag_length.type, tag_length.length);
        if (tag_length.type == MMWDEMO_OUTPUT_MSG_DETECTED_POINTS) { // Obj detected.
            std::cout << "Object TLV detected!\n";
            /* Get Metadata */
            MmwDemo_output_message_dataObjDescr obj_metadata;
            auto ptr = (uint8_t*)&obj_metadata;
            for (int i = 0; i < sizeof(obj_metadata); ++i) {
                int next_byte = serialGetchar(fd);
                *(ptr++) = next_byte;
            }
            std::cout << "Num detected objects: " << obj_metadata.numDetetedObj
                << ", xyzqformat: " << obj_metadata.xyzQFormat << "\n";
            /* Process all of the associated objects. */
            for (int obj_num = 0; obj_num < obj_metadata.numDetetedObj; obj_num++) {
                MmwDemo_detectedObj detected_obj;
                auto ptr = (uint8_t*)&detected_obj;
                for (int i = 0; i < sizeof(detected_obj); ++i) {
                    int next_byte = serialGetchar(fd);
                    *(ptr++) = next_byte;
                }
                std::cout << "Object Peak Val: " << detected_obj.peakVal
                    << ", X Coord Meters (Q Format): " << detected_obj.x 
                    <<", Y: " << detected_obj.y
                    <<", Z: " << detected_obj.z << "\n";
                //TODO(Ben) Figure out Q format.
                int attempt = detected_obj.rangeIdx + detected_obj.x * 65536;
                std::cout << "Secret Range: " << attempt << "\n";
                double real_range_meters = attempt / 1048576.0;
                std::cout << "Floating-pt Range: " << real_range_meters << "\n";
                double adjusted_range = (real_range_meters - 0.0696) * 1000;
                std::cout << "Adjusted: " << adjusted_range << "\n";
            }

        } else {
            std::cout << "Non-object TLV detected. Skipping TLV.\n";
            // Advance to the next TLV.
            // TODO(Ben) does the length include the tlv header or not?
            for (int i = 0; i < tag_length.length; i++) {
                serialGetchar(fd);
            }
        }
    }
}

void startSensor()
{
    fd = serialOpen("/dev/serial0", BAUD_TX);
    std::ifstream file("config");
    std::string line;
    while (std::getline(file, line)) {
        serialPuts(fd, line.c_str());
        serialPutchar(fd, '\n');
    }
    serialClose(fd);
}
