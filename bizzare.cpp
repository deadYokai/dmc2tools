#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>

#define DEBUG_LOG(msg) std::cout << "[DEBUG] " << msg << std::endl
#define ERROR_LOG(msg) std::cerr << "[ERROR] " << msg << std::endl

size_t decompress(const std::vector<int16_t>& inputData, std::vector<int16_t>& outputData) {
    uint32_t bitBuffer = 0;
    uint32_t bitMask = 0;
    const int16_t* inputPtr = inputData.data();
    const int16_t* inputEnd = inputData.data() + inputData.size(); // Prevent reading beyond input
    int16_t* outputPtr = outputData.data();
    size_t outputSize = outputData.size();

    DEBUG_LOG("Starting decompression...");

    while (true) {
        if (bitMask == 0) {
            if (inputPtr >= inputEnd) {
                ERROR_LOG("Out-of-bounds read at bitBuffer.");
                break;
            }
            bitBuffer = static_cast<uint32_t>(*inputPtr++);
            bitMask = 0x8000;
            DEBUG_LOG("New bitBuffer loaded: " + std::to_string(bitBuffer));
        }

        if ((bitMask & bitBuffer) == 0) {
            // Literal copy
            if (inputPtr >= inputEnd) {
                ERROR_LOG("Out-of-bounds read at literal copy.");
                break;
            }

            if (outputPtr - outputData.data() >= outputSize) {
                size_t currentOffset = outputPtr - outputData.data();
                outputData.resize(outputSize * 2, 0);
                outputSize = outputData.size();
                outputPtr = outputData.data() + currentOffset;
                DEBUG_LOG("Output buffer resized to: " + std::to_string(outputSize));
            }

            *outputPtr++ = *inputPtr++;
            DEBUG_LOG("Literal copy: " + std::to_string(*(outputPtr - 1)));
        } else {
            // Match copy
            if (inputPtr >= inputEnd) {
                ERROR_LOG("Out-of-bounds read at match copy.");
                break;
            }

            uint64_t offset = static_cast<uint64_t>(*inputPtr++);
            uint32_t length = offset >> 11;
            offset &= 0x7FF;

            if (length == 0) {
                if (inputPtr >= inputEnd) {
                    ERROR_LOG("Out-of-bounds read for length.");
                    break;
                }
                length = static_cast<uint32_t>(*inputPtr++);
            }

            if(outputSize <length) continue;
            DEBUG_LOG("Match copy: offset=" + std::to_string(offset) + ", length=" + std::to_string(length));

            if (offset != 0) {
                if (outputPtr - offset < outputData.data()) {
                    ERROR_LOG("Invalid back-reference offset.");
                    break;
                }

                int16_t* copySrc = outputPtr - offset;
                for (uint32_t i = 0; i < length; i++) {
                    if (outputPtr - outputData.data() >= outputSize) {
                        size_t currentOffset = outputPtr - outputData.data();
                        outputData.resize(outputSize * 2, 0);
                        outputSize = outputData.size();
                        outputPtr = outputData.data() + currentOffset;
                        DEBUG_LOG("Output buffer resized during match copy to: " + std::to_string(outputSize));
                    }
                    *outputPtr++ = *copySrc++;
                }
            } else {
                if (length == 0) break;
                for (uint32_t i = 0; i < length; i++) {
                    if (outputPtr - outputData.data() >= outputSize) {
                        size_t currentOffset = outputPtr - outputData.data();
                        outputData.resize(outputSize * 2, 0);
                        outputSize = outputData.size();
                        outputPtr = outputData.data() + currentOffset;
                        DEBUG_LOG("Output buffer resized during zero-fill to: " + std::to_string(outputSize));
                    }
                    *outputPtr++ = 0;
                }
            }
        }

        bitMask >>= 1;
        if (bitMask == 0) {
            if (inputPtr >= inputEnd) {
                ERROR_LOG("Out-of-bounds read at bit reset.");
                break;
            }
            bitBuffer = static_cast<uint32_t>(*inputPtr++);
            bitMask = 0x8000;
            DEBUG_LOG("New bitBuffer loaded at reset: " + std::to_string(bitBuffer));
        }
    }

    DEBUG_LOG("Decompression complete. Output size: " + std::to_string(outputPtr - outputData.data()));
    return outputPtr - outputData.data();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file>\n";
        return 1;
    }

    std::ifstream inputFile(argv[1], std::ios::binary | std::ios::ate);
    if (!inputFile) {
        std::cerr << "Failed to open input file: " << argv[1] << std::endl;
        return 1;
    }

    size_t fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);
    if (fileSize % sizeof(int16_t) != 0) {
        std::cerr << "[ERROR] Input file size is not aligned to 2 bytes.\n";
        return 1;
    }

    std::vector<int16_t> inputData(fileSize / sizeof(int16_t));
    inputFile.read(reinterpret_cast<char*>(inputData.data()), fileSize);
    inputFile.close();
    DEBUG_LOG("Input file loaded: " + std::to_string(fileSize) + " bytes.");

    std::vector<int16_t> outputData(inputData.size() * 2, 0);
    size_t decompressedSize = decompress(inputData, outputData);

    std::ofstream outputFile(argv[2], std::ios::binary);
    if (!outputFile) {
        std::cerr << "Failed to open output file: " << argv[2] << std::endl;
        return 1;
    }
    outputFile.write(reinterpret_cast<char*>(outputData.data()), decompressedSize * sizeof(int16_t));
    outputFile.close();
    DEBUG_LOG("Output file written: " + std::to_string(decompressedSize * sizeof(int16_t)) + " bytes.");

    return 0;
}
