#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <utility>
#include <vector>

#define phere printf("here?\n")

enum
{
	UNPACK,
	PACK
};

struct blockM
{
	uint64_t offset;
	uint64_t size;
};

// somehow basic.ptz broke (also in bizzare)
size_t decompress(int16_t* inputBuffer, int16_t* outputBuffer, size_t size) {
	uint32_t bitBuffer = 0;
	uint32_t bitMask = 0;
	int16_t* inpPtr = inputBuffer;
	int16_t* outPtr = outputBuffer;
	int16_t* inpEnd = inputBuffer + size / sizeof(int16_t);

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

enum fileType
{
	unk = -1,
	tim2,
	ptx, // where offset is 2048 (0x800)
	momo
};

fileType findFileType(std::vector<char>& f, size_t fsize)
{
	char magic[4] = {f[0], f[1], f[2], f[3]};
	char momoHeader[4] = {'M', 'O', 'M', 'O'};
	char tim2Header[4] = {'T', 'I', 'M', '2'};

	if((magic[2] == momoHeader[2] && magic[3] == momoHeader[3]) || (magic[2] == tim2Header[2] && magic[3] == tim2Header[3]))
	{
		char tmp[4] = {f[2], f[3], f[4], f[5]};
		if(memcmp(tmp, tim2Header, 4) == 0 || memcmp(tmp, momoHeader, 4) == 0)
		{
			std::vector<char> decompressed_data(fsize*2);
			size_t decompressed_size = decompress(reinterpret_cast<int16_t*>(f.data()), reinterpret_cast<int16_t*>(decompressed_data.data()), decompressed_data.size());
			if (decompressed_size == 0) return unk;
			printf("%c %c %c %c\n", f[0], f[1], f[2], f[3]);

			decompressed_data.resize(decompressed_size);
			f = std::move(decompressed_data);
			return findFileType(f, decompressed_size);
		}
		return (memcmp(magic, momoHeader, 4) == 0) ? momo : tim2;
	}else{
		if(fsize > (2048 + 512)){ // offset + some data stuff
			char lm[4] = {f[2048], f[2049], f[2050], f[2051]};
			if(memcmp(lm, tim2Header, 4) == 0)
				return ptx;
			else
			{
				char lm2[4] = {f[0x52], f[0x53], f[0x54], f[0x55]}; // idk if right, but found only in basic.ptz
				if(memcmp(lm2, tim2Header, 4) == 0)
				{
					std::vector<char> decompressed_data(fsize*2);
					size_t decompressed_size = decompress(reinterpret_cast<int16_t*>(f.data()), reinterpret_cast<int16_t*>(decompressed_data.data()), fsize*2);
					if (decompressed_size == 0) return unk;
					phere;
					decompressed_data.resize(decompressed_size);
					f = std::move(decompressed_data);
					return findFileType(f, decompressed_size);
				}
			}
		}
	}

	return unk;
}

int met = UNPACK;

void printHelp(const char* m)
{
	printf("Usage: %s <filename> [-e | -p]\n\t-e | --extract (default)\n\t-p | --pack\n\n", m);
}

void printUsageError(const char* m, const char* arg)
{
	printf("-- Unknown argument: \"%s\"\n", arg);
	printHelp(m);
}

void doSomethingWithFile(const char* filename)
{
	if(!std::filesystem::exists(filename))
	{
		printf("-- File not exists\n");
		return;
	}

	std::ifstream f(filename, std::ios::binary);

	if(!f.is_open())
	{
		printf("-- Failed to open file\n");
		return;
	}

	f.seekg(0, std::ios::end);
	size_t fsize = f.tellg();
	f.seekg(0, std::ios::beg);
	std::vector<char> buff(fsize);
	f.read(buff.data(), buff.size());
	f.close();

	if (fsize < 16){
		printf("-- Wrong file?");
		exit(-1);
	}

	fileType ft = findFileType(buff, fsize);

	printf("-- File size: %lu\n", fsize);
	printf("-- File type: %i\n", ft);


	// char* dirname = new char[sizeof(filename) + 1];
	// sprintf(dirname, "_%s", filename);
	// if(!compressed)
	// {
	// 	std::vector<blockM> blocks;
	// 	uint64_t blocks_count;
	// 	f.seekg(8);
	// 	f.read(reinterpret_cast<char*>(&blocks_count), sizeof(blocks_count));
	// 	for(size_t i = 0; i < blocks_count; i++)
	// 	{
	// 		blockM block;
	// 		f.read(reinterpret_cast<char*>(&block), sizeof(block));
	// 		blocks.push_back(block);
	// 	}
	// 	f.seekg(0, std::ios::beg);
	//
	// 	if(!std::filesystem::is_directory(dirname))
	// 	{
	// 		if(std::filesystem::create_directory(dirname))
	// 		{
	// 			printf("-- Directory \"%s\" created\n", dirname);
	// 		}
	// 	}
	//
	// 	for(size_t i = 0; i < blocks.size(); i++)
	// 	{
	// 		uint64_t o = blocks[i].offset;
	// 		uint64_t s = blocks[i].size;
	// 			
	// 		char* ufn = new char[6 + sizeof(dirname) + 1];
	// 		sprintf(ufn, "%s/id%lu", dirname, i);
	// 		printf("-- Writing to \"%s\", offset: %lu, size: %lu\n", ufn, o, s);
	// 		assert(o + s <= fsize); // check if filesize is bigger or equal
	// 		
	// 		std::vector<char> buff(s);
	// 		f.seekg(o, std::ios::beg);
	// 		f.read(buff.data(), s);
	//
	// 		std::ofstream uf(ufn);
	// 		uf.write(buff.data(), s);
	// 		uf.close();
	//
	// 		buff.clear();
	// 		delete [] ufn;
	// 	}
	// }
	// else
	// {
	// 	
	// }
	// delete [] dirname;
}

int main(int argc, char** argv)
{
	printf("== Shitty repacker from deadYokai ==\n");
	if(argc <= 1){
		printHelp(argv[0]);
		return 0;
	}

	if(argv[2])
	{
		if(strcmp(argv[2], "-p") == 0 || strcmp(argv[2], "--pack") == 0)
		{
			met = PACK;
		}
		else if(strcmp(argv[2], "-e") == 0 || strcmp(argv[2], "--extract") == 0)
		{
			met = UNPACK;
		}
		else
		{
			printUsageError(argv[0], argv[2]);
			return 0;
		}
	}

	printf("-- Input file: '%s'\n", argv[1]);
	doSomethingWithFile(argv[1]);
	return 1;
}

