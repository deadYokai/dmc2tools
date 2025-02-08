#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#define phere printf("here?\n")

#define DBUFFER_SIZE 8 * 1024 * 1024 // 8 MiB for now

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

struct miniBlock
{
	uint32_t offset;
	uint32_t size;
};

size_t compress(int16_t* inputBuffer, int16_t* outputBuffer, size_t size) {
    uint32_t bitBuffer = 0;
    uint32_t bitMask = 0x8000;
    int16_t* outPtr = outputBuffer;
    int16_t* inPtr = inputBuffer;
    int16_t* inEnd = inputBuffer + size / sizeof(int16_t);

    int16_t* flagWordPtr = outPtr++;
    *flagWordPtr = 0;

    while (inPtr < inEnd) {
        if (bitMask == 0) {
            flagWordPtr = outPtr++;
            *flagWordPtr = 0;
            bitMask = 0x8000;
        }

        int16_t* searchPtr = inPtr - 1;
        int16_t* searchEnd = (inPtr - 0x7ff) > inputBuffer ? (inPtr - 0x7ff) : inputBuffer;
        int16_t* bestMatch = nullptr;
        uint32_t bestLength = 0;

        while (searchPtr >= searchEnd) {
            uint32_t length = 0;
            while (searchPtr[length] == inPtr[length] && (inPtr + length) < inEnd && length < 0xb) {
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
            *outPtr++ = (bestLength << 0xb) | offset;
            inPtr += bestLength;
            *flagWordPtr |= bitMask;
        } else {
            *outPtr++ = *inPtr++;
        }
        bitMask >>= 1;
    }
    return (outPtr - outputBuffer) * sizeof(int16_t);
}

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
	return (outPtr - outputBuffer) * sizeof(int16_t);
}

enum fileType
{
	unk = -1,
	tim2,
	ptx, // where offset is 2048 (0x800)
	ipu,
	momo
};

const char* fileTypeExt(fileType ft)
{
	switch (ft) {
		case momo: return "bin";
		case tim2: return "tm2";
		case ptx: return "ptx";
		case ipu: return "ipu";
		default: return "unk";
	}
}

fileType findFileType(std::vector<char>& f, size_t fsize)
{
	char magic[4] = {f[0], f[1], f[2], f[3]};
	char momoHeader[4] = {'M', 'O', 'M', 'O'};
	char tim2Header[4] = {'T', 'I', 'M', '2'};
	char ipumHeader[4] = {'i', 'p', 'u', 'm'};

	if((magic[2] == momoHeader[0] && magic[3] == momoHeader[1] && magic[0] != momoHeader[0]) || (magic[2] == tim2Header[0] && magic[3] == tim2Header[1]))
	{
		char tmp[4] = {f[2], f[3], f[4], f[5]};
		if(memcmp(tmp, tim2Header, 4) == 0 || memcmp(tmp, momoHeader, 4) == 0)
		{
			std::vector<char> decompressed_data(DBUFFER_SIZE * sizeof(int16_t));
			size_t decompressed_size = decompress(reinterpret_cast<int16_t*>(f.data()), reinterpret_cast<int16_t*>(decompressed_data.data()), fsize);
			
			if (decompressed_size == 0) return unk;

			decompressed_data.resize(decompressed_size);
			f = std::move(decompressed_data);
			return findFileType(f, decompressed_size);
		}
	}
	else if(memcmp(magic, momoHeader, 4) == 0)
		return momo;
	else if(memcmp(magic, tim2Header, 4) == 0)
		return tim2;
	else if(memcmp(magic, ipumHeader, 4) == 0)
		return ipu;
	else{
		if(fsize > (2048 + 512)){ // offset + some data stuff
			char lm[4] = {f[2048], f[2049], f[2050], f[2051]};
			if(memcmp(lm, tim2Header, 4) == 0)
				return ptx;
			else
			{
				char lm2[4] = {f[0x12], f[0x13], f[0x14], f[0x15]}; // idk if right, but found only in basic.ptz
				if(memcmp(lm2, tim2Header, 4) == 0)
				{
					std::vector<char> decompressed_data(DBUFFER_SIZE * sizeof(int16_t));
					size_t decompressed_size = decompress(reinterpret_cast<int16_t*>(f.data()), reinterpret_cast<int16_t*>(decompressed_data.data()), decompressed_data.size());
					if (decompressed_size == 0) return unk;
					decompressed_data.resize(decompressed_size);
					f = std::move(decompressed_data);
					return findFileType(f, decompressed_size);
				}
			}
		}
	}

	return unk;
}

fileType findFileType(const char* path)
{
	std::ifstream f(path, std::ios::binary | std::ios::ate);
	size_t fsize = f.tellg();
	f.seekg(0, std::ios::beg);
	std::vector<char> buff(fsize);
	f.read(buff.data(), buff.size());
	f.close();
	return findFileType(buff, fsize);
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

struct ipumHeader
{
	char magic[4];
	uint32_t unk1;
	uint32_t unk2;
	uint32_t frameCount;
	uint32_t framerate;
};

void ipumUnpack(std::vector<char>& buffer, const char* dirname, std::filesystem::path basename, bool isc)
{
	std::istringstream f(std::string(buffer.begin(), buffer.end()), std::ios::binary);
	f.seekg(0, std::ios::beg);
	ipumHeader ipu;
	f.read(reinterpret_cast<char*>(&ipu), sizeof(ipu));
	printf("-- Ipum frame count: %i\n", ipu.frameCount);
	if(!std::filesystem::is_directory(dirname))
	{
		if(std::filesystem::create_directory(dirname))
		{
			printf("-- Directory \"%s\" created\n", dirname);
		}
	}
	
	char* tc = new char[strlen(dirname) + strlen(basename.string().c_str()) + 6];
	sprintf(tc, "%s/.meta.%s", dirname, basename.string().c_str());
	std::ofstream metadata(tc, std::ios::binary);
	metadata.write(reinterpret_cast<char*>(&ipu), sizeof(ipu));
	metadata.close();
	delete [] tc;

	for(uint32_t i = 0; i < ipu.frameCount; i++)
	{
		char frameStart[4];
		char frameEnd[4];
		uint32_t tSize;
		f.read(frameStart, 4);
		f.read(reinterpret_cast<char*>(&tSize), 4);
		char* texture = new char[tSize];
		f.read(texture, tSize-4);
		f.read(frameEnd, 4);
		char* tn = new char[strlen(dirname) + sizeof(uint32_t) + 12];
		sprintf(tn, "%s/frame%i.dds", dirname, i);
		char* tnm = new char[strlen(dirname) + sizeof(uint32_t) + 12 + 1];
		sprintf(tnm, "%s/frame%i.meta", dirname, i);
		printf("-- Writing texture to \"%s\"\n", tn);
		std::ofstream dt(tnm, std::ios::binary);
		dt.write(frameEnd, 4);
		dt.close();
		std::ofstream dds(tn, std::ios::binary);
		dds.write(texture, tSize);
		delete [] texture;
		dds.close();
		delete [] tnm;
		delete [] tn;
	}

}

void ptxUnpack(std::vector<char>& buffer, const char* dirname, std::filesystem::path basename, bool isc)
{

}

struct Tim2Header
{
	char magic[4];
	char formatVer;
	char formatId;
	uint16_t count;
	char pad[8];
};

struct Tim2PicHeader
{
	uint32_t totalSize;
	uint32_t clutSize;
	uint32_t imgSize;
	uint16_t headerSize;
	uint16_t clutColors;
	char picFormat;
	char MipMapTextures;
	char clutType;
	char ImageType;
	uint16_t ImageWidth;
	uint16_t ImageHeight;

	uint64_t GsTex0;
	uint64_t GsTex1;
	uint32_t GsTexaFbaPabe;
	uint32_t GsTexClut;
};

void tim2Unpack(std::vector<char>& buffer, const char* dirname, std::filesystem::path basename, bool isc)
{
	// to pack TotalSize = sizeof(Tim2PicHeader) + imgSize;
	std::istringstream f(std::string(buffer.begin(), buffer.end()), std::ios::binary);
	f.seekg(0, std::ios::beg);
	Tim2Header t2header;
	Tim2PicHeader t2pic;
	f.read(reinterpret_cast<char*>(&t2header), sizeof(t2header));
	if(t2header.formatId != 0x0)
		f.seekg(0x80, std::ios::beg);
	f.read(reinterpret_cast<char*>(&t2pic), sizeof(t2pic));
	f.seekg(t2pic.headerSize - sizeof(t2pic), std::ios::cur);
	size_t pos = f.tellg();
	char ddsMagic[4] = {'D', 'D', 'S', ' '};
	char* texture = new char[t2pic.imgSize];
	f.read(texture, t2pic.imgSize);
	if(memcmp(texture, ddsMagic, 4) != 0)
	{
		printf("-E Supported only PC format of DMC2 textures\n");
		exit(-1);
	}
	if(!std::filesystem::is_directory(dirname))
	{
		if(std::filesystem::create_directory(dirname))
		{
			printf("-- Directory \"%s\" created\n", dirname);
		}
	}
	char* tn = new char[strlen(dirname) + strlen(basename.string().c_str()) + 6];
	sprintf(tn, "%s/%s.dds", dirname, basename.string().c_str());
	printf("-- Writing texture to \"%s\"\n", tn);
	std::ofstream dds(tn, std::ios::binary);
	dds.write(texture, t2pic.imgSize);
	dds.close();
	delete [] tn;
	char* tc = new char[strlen(dirname) + strlen(basename.string().c_str()) + 8];
	sprintf(tc, "%s/.meta.%s", dirname, basename.string().c_str());
	std::ofstream metadata(tc, std::ios::binary);
	f.seekg(0, std::ios::beg);
	char* _r = new char[pos];
	f.read(_r, pos);
	metadata.write(_r, pos);
	delete [] _r;
	metadata.close();
	delete [] tc;
}

void momoUnpack(std::vector<char>& buffer, const char* dirname, std::filesystem::path basename, bool isc)
{
	std::istringstream f(std::string(buffer.begin(), buffer.end()), std::ios::binary);

	std::vector<blockM> blocks;
	std::vector<miniBlock> mBlocks;

	size_t blocks_count;

	bool mb = false;
	char c;
	f.seekg(sizeof(uint32_t));
	f.read(&c, 1);
	if (c != 0x0)
		mb = true;

	if(!mb)
	{
		f.seekg(sizeof(uint64_t), std::ios::beg);
		f.read(reinterpret_cast<char*>(&blocks_count), sizeof(uint64_t));
	}else{
		f.seekg(sizeof(uint32_t), std::ios::beg);
		uint32_t tbc;
		f.read(reinterpret_cast<char*>(&tbc), sizeof(uint32_t));
		blocks_count = tbc;
	}

	for(size_t i = 0; i < blocks_count; i++)
	{
		if(!mb){
			blockM block;
			f.read(reinterpret_cast<char*>(&block), sizeof(block));
			blocks.push_back(block);
		}else{	
			miniBlock mblock;
			f.read(reinterpret_cast<char*>(&mblock), sizeof(mblock));
			mBlocks.push_back(mblock);
		}
	}
	f.seekg(0, std::ios::beg);

	if(!std::filesystem::is_directory(dirname))
	{
		if(std::filesystem::create_directory(dirname))
		{
			printf("-- Directory \"%s\" created\n", dirname);
		}
	}

	size_t bsize = mb ? mBlocks.size() : blocks.size();

	char* tc = new char[strlen(dirname) + 12];
	sprintf(tc, "%s/.metadata", dirname);
	std::vector<std::string> metadataBuffer;
	metadataBuffer.push_back(basename.string());
	if(isc)
		metadataBuffer.push_back("_compressed");
	
	for(size_t i = 0; i < bsize; i++)
	{
		uint64_t o, s;
		if(!mb){
			o = blocks[i].offset;
			s = blocks[i].size;
		}else{
			o = mBlocks[i].offset;
			s = mBlocks[i].size;
		}

		char* ufn = new char[6 + strlen(dirname) + 1 + 3];
		assert(o + s <= buffer.size()); // check if filesize is bigger or equal

		std::vector<char> buff(s);
		f.seekg(o, std::ios::beg);
		f.read(buff.data(), s);

		fileType ft = findFileType(buff, buff.size());

		sprintf(ufn, "%s/id%lu.%s", dirname, i, fileTypeExt(ft));
		printf("-- Writing to \"%s\", offset: %lu, size: %lu\n", ufn, o, s);

		std::ofstream uf(ufn, std::ios::binary);
		uf.write(buff.data(), s);
		uf.close();

		std::filesystem::path ap = std::filesystem::absolute(ufn);
		std::filesystem::path bn = ap.filename();
		std::filesystem::path dn = ap.parent_path() / ("_" + bn.string());


		switch (ft) {
			case momo:
				momoUnpack(buff, dn.string().c_str(), bn, false);
				break;
			case ptx:
				ptxUnpack(buff, dn.string().c_str(), bn, false);
				break;
			case tim2:
				tim2Unpack(buff, dn.string().c_str(), bn, false);
				break;
			case ipu:
				ipumUnpack(buff, dn.string().c_str(), bn, false);
				break;
			default:
				return;
		}
		buff.clear();

		std::filesystem::path _t = std::filesystem::absolute(ufn).filename();
		metadataBuffer.push_back(_t.string());
		delete [] ufn;
	}
	printf("-- Creating \"%s/.metadata\" file\n", dirname);
	std::ofstream metadata(tc, std::ios::out);
	for(const auto e : metadataBuffer)
	{
		metadata << e << '\n';
	}
	delete [] tc;
	metadata.close();
}

void unpack(const char* filename)
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

	bool isCompressed = (fsize != buff.size()) ? true : false;
	printf("-- File size: %lu\n", fsize);
	if(isCompressed)
		printf("-- Decompressed size: %lu\n", buff.size());
	printf("-- File type: %s\n", fileTypeExt(ft));

	std::filesystem::path ap = std::filesystem::absolute(filename);
	std::filesystem::path basename = ap.filename();
        std::filesystem::path dirname = ap.parent_path() / ("_" + basename.string());

	switch (ft) {
		case momo:
			momoUnpack(buff, dirname.string().c_str(), basename, isCompressed);
			break;
		case ptx:
			ptxUnpack(buff, dirname.string().c_str(), basename, isCompressed);
			break;
		case tim2:
			tim2Unpack(buff, dirname.string().c_str(), basename, isCompressed);
			break;
		case ipu:
			ipumUnpack(buff, dirname.string().c_str(), basename, isCompressed);
			break;
		default:
			return;
	}

}

void doSomethingWithFile(const char* filename)
{
	if(met == UNPACK)
		unpack(filename);
	else
		printf("Not implemented yet.\n");
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

