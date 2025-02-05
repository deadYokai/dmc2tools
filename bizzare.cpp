#include <cstddef>
#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <cstring>

size_t decompress(int16_t* inputBuffer, size_t inputSize, int16_t* outputBuffer) {
    uint32_t bitBuffer = 0;
    uint32_t bitMask = 0;
    int16_t* inpPtr = inputBuffer;
    int16_t* outPtr = outputBuffer;
    int16_t* inpEnd = inputBuffer + inputSize / sizeof(int16_t);

    while (inpPtr < inpEnd) {
        if (bitMask == 0) {
            if (inpPtr >= inpEnd) break;
            bitBuffer = *inpPtr;
            bitMask = 0x8000;
            inpPtr++;
        }

        if ((bitMask & bitBuffer) == 0) {
            if (inpPtr >= inpEnd) break;
            *outPtr = *inpPtr;
            inpPtr++;
            outPtr++;
        } else {
            if (inpPtr >= inpEnd) break;
            uint16_t controlWord = *inpPtr;
            inpPtr++;
            uint32_t offset = controlWord & 0x7FF;
            uint32_t length = controlWord >> 11;

            if (length == 0) {
                if (inpPtr >= inpEnd) break;
                length = *inpPtr;
                inpPtr++;
            }

            if (offset == 0) {
                if (length == 0) break;
                std::memset(outPtr, 0, length * sizeof(int16_t));
                outPtr += length;
            } else {
                int16_t* copySrc = outPtr - offset;
                for (uint32_t i = 0; i < length; i++) {
                    *outPtr = *copySrc;
                    outPtr++;
                    copySrc++;
                }
            }
        }

        bitMask >>= 1;
    }

    return outPtr - outputBuffer;
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

    std::vector<int16_t> outputBuffer(inputSize * 2); // Allocate larger space for decompression
    size_t outputSize = decompress(inputBuffer.data(), inputSize, outputBuffer.data());

	std::ofstream outputFile(argv[2], std::ios::binary);
	if (!outputFile) {
		std::cerr << "Failed to open output file: " << argv[2] << std::endl;
		return 1;
	}
	outputFile.write(reinterpret_cast<char*>(outputBuffer.data()), outputSize * sizeof(int16_t));
	outputFile.close();

	return 0;
}
