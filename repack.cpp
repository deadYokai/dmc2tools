#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <vector>

void phere()
{
	printf("here?\n");
}

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

struct blockM_compressed
{
	uint32_t offset;
	uint32_t size;
};


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

	bool compressed = false;

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

	printf("-- File size: %lu\n", fsize);

	char magic[4] = {};
	char expect[4] = {'M', 'O', 'M', 'O'};
	f.read(magic, 4);

	if(memcmp(magic, expect, 4) != 0)
	{
		f.seekg(2);
		f.read(magic, 4);
		compressed = true;
		if(memcmp(magic, expect, 4) != 0)
		{
			printf("Unsupported file type\n");
			return;
		}
		printf("-- WARNING: compressed MOMO file\n\t    some broken shit here\n");
	}
	
	char* dirname = new char[sizeof(filename) + 1];
	sprintf(dirname, "_%s", filename);
	if(!compressed)
	{
		std::vector<blockM> blocks;
		uint64_t blocks_count;
		f.seekg(8);
		f.read(reinterpret_cast<char*>(&blocks_count), sizeof(blocks_count));
		for(size_t i = 0; i < blocks_count; i++)
		{
			blockM block;
			f.read(reinterpret_cast<char*>(&block), sizeof(block));
			blocks.push_back(block);
		}
		f.seekg(0, std::ios::beg);

		if(!std::filesystem::is_directory(dirname))
		{
			if(std::filesystem::create_directory(dirname))
			{
				printf("-- Directory \"%s\" created\n", dirname);
			}
		}

		for(size_t i = 0; i < blocks.size(); i++)
		{
			uint64_t o = blocks[i].offset;
			uint64_t s = blocks[i].size;
				
			char* ufn = new char[6 + sizeof(dirname) + 1];
			sprintf(ufn, "%s/id%lu", dirname, i);
			printf("-- Writing to \"%s\", offset: %lu, size: %lu\n", ufn, o, s);
			assert(o + s <= fsize); // check if filesize is bigger or equal
			
			std::vector<char> buff(s);
			f.seekg(o, std::ios::beg);
			f.read(buff.data(), s);

			std::ofstream uf(ufn);
			uf.write(buff.data(), s);
			uf.close();

			buff.clear();
			delete [] ufn;
		}
	}
	else
	{
		std::vector<blockM_compressed> blocks;
		uint32_t blocks_count;
		f.seekg(6);
		f.read(reinterpret_cast<char*>(&blocks_count), sizeof(blocks_count));
		for(size_t i = 0; i < blocks_count; i++)
		{
			blockM_compressed block;
			f.read(reinterpret_cast<char*>(&block), sizeof(block));
			blocks.push_back(block);
		}
		f.seekg(0, std::ios::beg);

		if(!std::filesystem::is_directory(dirname))
		{
			if(std::filesystem::create_directory(dirname))
			{
				printf("-- Directory \"%s\" created\n", dirname);
			}
		}

		for(size_t i = 0; i < blocks.size(); i++)
		{
			uint32_t o = blocks[i].offset;
			uint32_t s = blocks[i].size;
				
			char* ufn = new char[6 + sizeof(dirname) + 1];
			sprintf(ufn, "%s/id%lu", dirname, i);
			printf("-- Writing to \"%s\", offset: %i, size: %i\n", ufn, o, s);
			assert(o + s <= fsize); // check if filesize is bigger or equal
			
			std::vector<char> buff(s);
			f.seekg(o, std::ios::beg);
			f.read(buff.data(), s);

			std::ofstream uf(ufn);
			uf.write(buff.data(), s);
			uf.close();

			buff.clear();
			delete [] ufn;
		}

	}
	delete [] dirname;
	f.close();
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

