#include <cstddef>
#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <unordered_map>

size_t compress(int16_t* inputBuffer, size_t inputSize, int16_t* outputBuffer) {
    uint32_t bitBuffer = 0;
    uint32_t bitMask = 0x8000;
    int16_t* outPtr = outputBuffer;
    int16_t* inPtr = inputBuffer;
    int16_t* inEnd = inputBuffer + inputSize / sizeof(int16_t);

    int16_t* flagWordPtr = outPtr++;
    *flagWordPtr = 0;

    while (inPtr < inEnd) {
        if (bitMask == 0) {
            flagWordPtr = outPtr++;
            *flagWordPtr = 0;
            bitMask = 0x8000;
        }

        int16_t* searchPtr = inPtr - 1;
        int16_t* searchEnd = (inPtr - 2047) > inputBuffer ? (inPtr - 2047) : inputBuffer;
        int16_t* bestMatch = nullptr;
        uint32_t bestLength = 0;

        while (searchPtr >= searchEnd) {
            uint32_t length = 0;
            while (searchPtr[length] == inPtr[length] && (inPtr + length) < inEnd && length < 31) {
                length++;
            }
            if (length > bestLength) {
                bestLength = length;
                bestMatch = searchPtr;
            }
            searchPtr--;
        }

        if (bestLength >= 2) {
            uint32_t offset = inPtr - bestMatch;
            *outPtr++ = (bestLength << 11) | offset;
            inPtr += bestLength;
            *flagWordPtr |= bitMask;
        } else {
            *outPtr++ = *inPtr++;
        }

        bitMask >>= 1;
    }

    return (outPtr - outputBuffer) * sizeof(int16_t);
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file>\n";
        return 1;
    }

    std::ifstream inputFile(argv[1], std::ios::binary);
    if (!inputFile) {
        std::cerr << "Error opening input file." << std::endl;
        return 1;
    }

    inputFile.seekg(0, std::ios::end);
    size_t inputSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    std::vector<int16_t> inputBuffer(inputSize / sizeof(int16_t));
    inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), inputSize);
    inputFile.close();

    std::vector<int16_t> outputBuffer(inputSize); // Allocate same space for compression
    size_t outputSize = compress(inputBuffer.data(), inputSize, outputBuffer.data());

    std::ofstream outputFile(argv[2], std::ios::binary);
    if (!outputFile) {
        std::cerr << "Failed to open output file: " << argv[2] << std::endl;
        return 1;
    }
    outputFile.write(reinterpret_cast<char*>(outputBuffer.data()), outputSize);
    outputFile.close();

    return 0;
}
