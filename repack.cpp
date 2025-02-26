#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#define DBUFFER_SIZE 8 * 1024 * 1024 // 8 MiB for now

void printErr(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	printf("-\033[1;31mE\033[1;0m ");
	vprintf(format, args);
	printf("\n");
	va_end(args);
}

void printHelp(const char* m)
{
	printf("Usage: %s <filename> [-e | -p]\n\t-e | --extract (default)\n\t-p | --pack\n\n", m);
}

void printUsageError(const char* m, const char* arg)
{
	printErr("Unknown argument: \"%s\"", arg);
	printHelp(m);
}

void pack(const char*);

/*
** ⠀⠀⠀⠀⢀⡴⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡼⠃⣀⠀⠀⣄⡀⠀⠀⠰⠆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣦⡀⠙⢷⣄⠀⠀⠀⠀⠀⢀⣀⣤⣴⣶⣾⣿⣿⣿⡿⠿⠷⢶⣾⣦⣐⠶⠤⠄⣀⣲⡀⠀⠀⠀⠀⠀⢻⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
** ⠀⠀⠀⢀⡖⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⡇⠀⡟⠀⠀⠈⢷⡀⠀⠀⠘⠶⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠸⣿⣛⢶⣄⡙⢷⣤⣤⣴⣿⣿⣿⣿⣿⣿⣿⡟⠉⠀⠀⠀⠀⠈⣷⡈⠻⣧⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
** ⠀⠀⣠⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢿⣿⠇⢠⡇⠀⠀⠀⠈⣷⣄⠀⠀⠀⠘⢷⡀⠀⠀⠀⠀⠀⠀⠀⠀⠘⢻⡎⠛⢾⣿⣶⣿⣿⠟⠋⠙⠻⢿⡿⠿⠋⠀⠀⠀⠀⠀⠀⠀⣿⣷⣤⠈⠛⢧⣄⠀⠀⠀⠀⠀⠀⠀⠀⠘⢷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
** ⠀⢠⡿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⣷⣾⠃⠀⠀⠀⡀⢹⣿⣦⡀⠀⠀⠈⢻⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⢿⡄⢨⣿⡟⠉⠿⢿⣷⣤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⢃⡿⠀⠀⠀⠉⠷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠙⢷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
** ⢠⡿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡼⢰⠀⠀⠻⣿⡿⡆⠀⠀⠀⡇⠀⢿⡏⠻⣆⠀⠀⠀⠹⣦⠀⠀⠀⠀⠀⠀⠀⠀⠈⣷⡈⠉⢿⡀⠀⠀⠈⠉⠉⠃⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⠋⣾⠃⠀⠀⠀⠀⠀⠙⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢷⣶⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
** ⣾⠃⠀⠀⠀⠀⠀⠀⠀⣶⡇⢸⡇⢸⡇⠀⠀⣻⣿⡖⠀⠀⢰⡇⠀⣸⣷⠀⠈⠳⣦⡀⠀⠈⢳⣄⠀⠀⠀⠀⠀⠀⠀⠘⢷⡀⠈⢷⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⡴⠟⠁⣿⠃⠀⠀⠀⠀⠀⠀⠀⢸⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠛⢯⡙⠳⣦⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀
** ⡟⠀⠀⠀⠀⠀⠀⠀⠠⣿⡇⠀⠇⠸⠇⠀⠀⢸⣿⣷⠀⠀⡿⠀⠀⢸⣿⡄⠀⠀⠈⠻⣦⣄⡀⠙⢷⣄⠀⠀⠀⠀⠀⠀⠘⣧⠀⠀⠙⠿⣿⣷⣶⣦⣤⣶⠶⠚⠛⠛⠉⠀⣠⡾⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⣽⣦⡀⠙⠛⢶⣤⡀⠀⠀⠀⠀⠀
** ⠁⠀⠀⠀⠀⠀⠀⠀⠀⣿⡇⠀⠀⠁⠀⠀⠀⢸⡏⢻⣇⣼⠃⠀⠀⠸⣿⡇⠀⣀⣤⣶⣤⣽⡿⠶⣤⣙⣷⣀⠀⠀⠀⠀⠀⠸⣧⠀⠀⠀⠈⠛⠛⠛⠛⠃⠀⠀⠀⣀⣤⠾⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢻⣿⣿⣦⡀⠀⠈⠙⠳⢦⡀⠀⠀
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⡇⠀⠀⠀⠀⠀⠀⢸⡇⠠⣿⣇⠀⠀⠀⠀⣿⣣⣶⣿⣿⡿⠛⠿⣄⠀⠀⠉⠛⠿⢶⣄⠀⠀⠀⠀⠙⣧⣀⣀⣀⣀⣀⣀⣀⣤⣤⠶⠞⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⠻⣿⣿⣿⣦⣄⠀⠀⠀⠈⠒⠂
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⣿⠀⠀⠀⠀⠀⠀⣸⡇⠰⢿⣿⡄⠀⢀⣴⣿⣿⣿⡿⠋⠀⠀⠀⠙⢷⡄⠀⠀⠀⠀⠉⣿⠶⣤⣄⡀⠈⣯⡉⠉⠉⠉⠉⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣹⡃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⣿⣿⣿⣷⣄⠀⠀⠀⠀
** ⠘⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⡀⠀⠀⠀⠀⠀⣾⡟⠁⠀⠻⣇⢰⣿⣿⣿⡿⠋⠀⠀⠀⠀⠀⠀⠈⣷⠀⠀⠀⠀⢰⡏⠀⠀⠈⠙⠛⠺⠷⣤⠀⠀⠀⢀⣄⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣽⣿⣧⡝⠻⢦⣄⠀⠀
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⣧⠀⠀⠀⠀⣠⣿⡇⠀⠀⠀⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⡆⠀⠀⢀⡿⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⣀⣴⠟⠛⠋⠛⢷⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⠇⠙⢷⡄⠀⠀⠀⠀⠀⠀⠀⠠⣤⡀⠀⠀⠈⠙⠻⣧⡄⠀⠉⠓⠀
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⡇⠀⢀⣴⠏⢸⡇⠀⢀⣾⣿⣿⣿⡏⢻⠀⠀⠀⠀⠀⠀⠀⠀⣾⣿⡇⠀⢀⣾⠁⠀⠀⠀⠀⠀⢠⣶⠟⠿⠿⠋⠁⠀⠀⠀⠀⠘⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⡏⠀⠀⠀⠻⣦⡀⠀⠀⠀⠀⠀⠀⠘⢿⡄⠀⠀⠀⠀⠘⢿⣦⡀⠀⠀
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣴⠟⠁⠀⢈⣧⠀⣿⣿⣿⣿⡿⠀⣿⠀⠀⠀⠀⠀⠀⢀⣾⣿⣿⠇⠀⣾⠁⠀⠀⠀⠀⠀⣀⣼⠏⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⠟⠹⣷⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⠃⠀⠀⠀⠐⢹⣷⡴⠀⠀⠀⠀⠀⠀⠈⣿⡄⠀⠀⠀⠀⠈⠻⣍⠀⠀
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⡴⠛⢧⡀⠀⠀⠈⣿⣿⣿⣿⣿⡿⠁⢠⡏⠀⠀⠀⠀⠀⣠⣾⣿⡿⠃⢀⣼⠃⠀⠀⢀⣾⣿⡿⠛⠉⠀⠀⠀⠀⠀⠀⠀⢀⣴⠟⠁⠀⢀⣼⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⡏⠀⠀⠀⠀⠀⠀⠘⢻⣄⠀⠀⠀⠀⠀⠀⢼⣿⣄⠀⠀⠀⠀⠀⠙⣧⡀
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠀⠀⠘⢷⡀⠀⣴⣿⣿⠛⠋⠁⠀⠀⠸⠇⠀⠀⠀⠀⣸⣿⠟⠉⠀⣠⠞⠁⠀⠀⠀⠸⣿⣿⡄⠀⠀⠀⠀⠀⢀⣀⣤⡾⠛⠁⠀⣴⠟⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⡿⠀⠀⠀⠀⠀⠀⠀⠀⠀⣹⣦⠀⠀⠀⠀⠀⠀⣿⣿⢷⣄⠀⠀⠀⠀⠈⠻
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢿⣄⣹⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⢀⣰⣿⠿⠋⠀⣠⡾⠋⠀⠀⠀⠀⠀⠀⢹⣟⣁⣤⡶⠶⠞⠛⠛⠉⠁⠀⠀⠀⢠⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠘⣷⡀⠀⠀⠀⠀⠀⢿⡄⠉⠀⠀⠀⠀⠀⠀
** ⠀⠀⠀⠀⠀⠀⢰⡄⠀⠀⠀⠀⠀⠀⠀⠀⠙⣿⡁⢹⠈⢷⣀⠀⣀⣀⣤⡶⠟⠋⠁⢀⣤⠾⠋⠀⠀⠀⠀⠀⠀⠀⠀⣾⣿⠟⠁⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⡾⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⡿⠙⠀⠀⢀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢷⡀⠀⠀⠀⠀⠸⣇⠀⠀⠀⠀⠀⠀⠀
** ⠀⠀⠀⠀⠀⠀⠀⢿⡀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠻⣾⣇⠈⢿⣿⠟⠛⠁⠀⣠⣤⠾⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣷⠀⠀⠀⠀⠀⠀⢀⣠⠶⠛⠛⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣤⡿⠷⠚⠛⠛⠛⠻⢷⣴⡀⠀⠀⠀⠀⠀⠀⠈⢻⡄⠀⠀⠀⠀⣿⠚⠛⠉⠀⠀⠀⠀
** ⠀⠀⠀⠀⠀⠀⠀⠘⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠛⠀⠀⠻⣆⠶⠞⠛⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠛⠶⠶⠶⢿⣤⠴⠟⠁⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⡴⠖⠛⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠈⠻⣶⡖⠀⢶⣦⠀⢀⣠⣴⠿⣆⠀⠀⠀⢿⡀⠀⠀⠀⠀⠀⠀
** ⠀⠀⠀⠀⠀⠀⠠⣤⡸⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⣾⢟⡁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣷⡿⠳⠾⠛⠉⠀⠀⠹⣆⠀⠀⢸⡇⠀⠀⠀⠀⠀⠀
** ⠀⠀⠀⠀⠀⠀⠀⠈⠙⢿⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⢧⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⠟⠁⠁⠈⠻⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠋⠀⠀⠀⠀⠀⠀⠀⠀⢻⡄⠀⠈⡇⠀⠀⠀⠀⠀⠀
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢻⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⢿⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⡿⠁⠀⠀⠀⠀⠀⠈⠻⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⡀⠀⡇⠀⠀⠀⠀⠀⠀
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢷⡀⠀⠀⠀⠀⠀⠀⠀⢰⠀⠀⠀⠀⠀⠀⠀⢽⣧⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣴⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠢⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⢷⠀⡇⠀⠀⠀⠀⠀⠀
** ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢧⠀⠀⠀⠀⠀⠀⠀⠈⠃⠀⠀⠀⠀⠀⠀⠀⠘⢷⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⣾⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡀⠓⢀⣀⠀⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⠃⠘⣷⡇⠀⠀⠀⠀⠀⠀
**
**
**  SOME BLACK MAGIC HAPPENING HERE ** 
**************************************/

size_t compress(const std::vector<char>& inputBuffer, std::vector<char>& outputBuffer)
{
	uint32_t bitBuffer = 0;
	uint32_t bitMask = 0x8000;
	int searchWindow = 2047;
	size_t size = inputBuffer.size() / sizeof(uint16_t);
	const uint16_t* inp = reinterpret_cast<const uint16_t*>(inputBuffer.data());

	outputBuffer.clear();
	outputBuffer.resize(size * 2 + 2048);

	uint16_t* outPtr = reinterpret_cast<uint16_t*>(outputBuffer.data());

	const uint16_t* inPtr = inp;
	const uint16_t* inEnd = inp + size;

	uint16_t* flagWordPtr = outPtr++;
	*flagWordPtr = 0;

	while (inPtr < inEnd) {
		if (bitMask == 0) {
			flagWordPtr = outPtr++;
			*flagWordPtr = 0;
			bitMask = 0x8000;
		}

		const uint16_t* searchEnd = (inPtr - searchWindow) > inp ? (inPtr - searchWindow) : inp;

		const uint16_t* bestMatch = nullptr;
		uint32_t bestLength = 0;
		const uint16_t* searchPtr = inPtr - 1;

		while (searchPtr >= searchEnd) {
			uint32_t length = 0;
			while (searchPtr[length] == inPtr[length] && (inPtr + length) < inEnd && length < 11) {
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

	if (bitMask != 0x8000) {
		*flagWordPtr |= bitMask;
	}

	size_t outputSize = (reinterpret_cast<char*>(outPtr) - outputBuffer.data());

	size_t align = (outputSize / 2048) * 2048;
	if(align == 0 || align < outputSize)
		align += 2048;

	outputBuffer.resize(align);
	return align;
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
			uint32_t length = controlWord >> 11;  // 0xB

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
	icon_sys, // aka icon.sys file
	ps2icn,	  // ps2 icon
	momo
};

const char* fileTypeExt(fileType ft)
{
	switch (ft) {
		case momo: return "bin";
		case tim2: return "tm2";
		case ptx: return "ptx";
		case ipu: return "ipu";
		case icon_sys: return "icon.sys";
		case ps2icn: return "icn";
		default: return "unk";
	}
}

fileType findFileType(std::vector<char>& f, size_t fsize)
{
	char magic[4] = {f[0], f[1], f[2], f[3]};
	char momoHeader[4] = {'M', 'O', 'M', 'O'};
	char tim2Header[4] = {'T', 'I', 'M', '2'};
	char ipumHeader[4] = {'i', 'p', 'u', 'm'};
	char ps2dHeader[4] = {'P', 'S', '2', 'D'};

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
	else if(memcmp(magic, ps2dHeader, 4) == 0)
		return icon_sys;
	else if(memcmp(magic, "\x00\x00\x01\x00", 4) == 0)
		return ps2icn;
	else{
		if(fsize > (2048 + 512)){ // offset + some data stuff
			char lm[4] = {f[2048], f[2049], f[2050], f[2051]};
			if(memcmp(lm, tim2Header, 4) == 0)
				return ptx;
			else
			{
				auto findCompressedPtx = std::search(f.begin(), f.end(), std::begin(tim2Header), std::end(tim2Header));
				if(findCompressedPtx != f.end())
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

struct ipumHeader
{
	char magic[4];
	uint32_t unk1;
	uint32_t unk2;
	uint32_t frameCount;
	uint32_t framerate;
};

void ipumPack(std::string dirname, std::string metadataFile, std::string origPath)
{
	// header aka .meta.your.ipu
	// dds size
	// dds data
	// frame end aka frame0.meta
	bool isc = false;
	std::string basename, line;
	std::filesystem::path _tp = dirname;
	std::vector<std::string> files;

	std::ifstream meta(metadataFile);
	std::getline(meta, basename);

	while(std::getline(meta, line))
	{
		if(line.find("_compressed") != std::string::npos)
		{
			isc = true;
			continue;
		}
		if(line == "") break;
		_tp = _tp.append(line);
		files.push_back(_tp.string());
		_tp = dirname;

	}
	meta.close();

	std::string fn = origPath + ".bak";
	if(!std::filesystem::exists(fn))
	{
		std::ifstream _fib(origPath, std::ios::binary);
		std::ofstream _fob(fn, std::ios::binary);
		_fob << _fib.rdbuf();
		_fob.close();
		_fib.close();
	}

	std::ostringstream f(std::ios::binary);

	_tp = _tp.append(".meta." + basename);
	std::ifstream ipuHeader(_tp.string(), std::ios::binary);
	f << ipuHeader.rdbuf();
	ipuHeader.close();


	for(size_t i = 0; i < files.size(); i++)
	{	
		printf("-- Writing \"%s\"\n", files[i].c_str());
		std::ifstream _t(files[i], std::ios::binary | std::ios::ate);
		size_t fsize = _t.tellg();
		_t.seekg(0, std::ios::beg);
		printf("   Size: %lu\n", fsize);
		f << "frmj";
		f.write(reinterpret_cast<char*>(&fsize), sizeof(uint32_t));
		f << _t.rdbuf();
		_t.close();
		std::string ddsMeta = files[i].erase(files[i].size() - 3) + "meta";
		_t.open(ddsMeta, std::ios::binary);
		_t.seekg(0, std::ios::beg);
		f.seekp(-4, std::ios::cur);
		f << _t.rdbuf();
		_t.close();
	}

	std::vector<char> buff;
	std::string _buff = f.str();
	buff.insert(buff.end(), _buff.begin(), _buff.end());
	if(isc)
	{
		printf("-- Compressing file\n");
		size_t _size = buff.size();
		std::vector<char> out(_size / 2);
		size_t csize = compress(buff, out);
		out.resize(csize);

		buff = std::move(out);
	}
	std::ofstream _f(origPath, std::ios::binary);
	_f.write(buff.data(), buff.size());
	_f.close();

}

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

	char* _metan = new char[strlen(dirname) + 12];
	sprintf(_metan, "%s/.metadata", dirname);
	std::vector<std::string> _meta;
	_meta.push_back(basename.string());
	if(isc)
		_meta.push_back("_compressed");
	printf("-- Creating \"%s/.metadata\" file\n", dirname);
	std::ofstream _metadata(_metan, std::ios::out);
	for(const auto e : _meta)
	{
		_metadata << e << '\n';
	}

	char* tc = new char[strlen(dirname) + strlen(basename.string().c_str()) + 6];
	sprintf(tc, "%s/.meta.%s", dirname, basename.string().c_str());
	std::ofstream metadata(tc, std::ios::binary);
	metadata.write(reinterpret_cast<char*>(&ipu), sizeof(ipu));
	delete [] tc;
	metadata.close();

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
		_metadata << "frame" << i << ".dds" << '\n';
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

	delete [] _metan;
	_metadata.close();
}
struct Tim2Header
{
	char magic[4];
	char formatVer;
	char formatId;
	uint16_t count;
	// char pad[8];
	// Maybe:
	uint32_t texOff;
	uint32_t texSize;
	//
};

struct GsTex {
	unsigned long TBP0 : 14;
	unsigned long TBW : 6;
	unsigned long PSM : 6;
	unsigned long TW : 4;
	unsigned long TH : 4;
	unsigned long TCC : 1;
	unsigned long TFX : 2;
	unsigned long CBP : 14;
	unsigned long CPSM : 4;
	unsigned long CSM : 1;
	unsigned long CSA : 5;
	unsigned long CLD : 3;
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

	GsTex GsTex0;
	GsTex GsTex1;
	uint32_t GsTexaFbaPabe;
	uint32_t GsTexClut;
};

void tim2Pack(std::string dirname, std::string metadataFile, std::string origPath)
{
	std::ifstream meta(metadataFile);
	std::string basename;
	std::string _tmpc;
	std::getline(meta, basename);
	std::getline(meta, _tmpc);
	meta.close();

	bool isc = false;
	if(_tmpc != "")
		isc = true;

	std::filesystem::path texFile = dirname;
	texFile = texFile.append(basename + ".dds");
	std::filesystem::path texMetaFile = dirname;
	texMetaFile = texMetaFile.append(".meta." + basename);

	std::vector<char> tim2;

	std::ifstream _tim2(texMetaFile, std::ios::binary);
	if(!_tim2.is_open())
	{
		printErr("Cannot open \"%s\"", texMetaFile.string().c_str());
		exit(-1);
	}
	Tim2Header t2header;
	Tim2PicHeader t2pic;
	_tim2.seekg(0, std::ios::end);
	size_t ts = _tim2.tellg();
	_tim2.seekg(0, std::ios::beg);
	_tim2.read(reinterpret_cast<char*>(&t2header), sizeof(t2header));
	std::vector<char> addData;
	std::vector<char> addData2;
	if(t2header.formatId != 0x0)
	{
		addData.resize(128 - _tim2.tellg());
		_tim2.read(addData.data(), addData.size());
	}
	_tim2.read(reinterpret_cast<char*>(&t2pic), sizeof(t2pic));
	size_t s = _tim2.tellg();
	if(ts > s)
	{
		addData2.resize(ts-s);
		_tim2.read(addData2.data(), (ts - s));
	}
	size_t hS = _tim2.tellg();
	_tim2.close();


	std::ifstream _dds(texFile, std::ios::binary | std::ios::ate);
	if(!_dds.is_open())
	{
		printErr("Cannot open \"%s\"", texFile.string().c_str());
		exit(-1);
	}
	size_t ddsSize = _dds.tellg();

	_dds.seekg(12, std::ios::beg);
	uint32_t w, h;
	_dds.read(reinterpret_cast<char*>(&w), sizeof(uint32_t));
	_dds.read(reinterpret_cast<char*>(&h), sizeof(uint32_t));
	_dds.seekg(0, std::ios::beg);

	std::vector<char> ddsData(ddsSize);
	_dds.read(ddsData.data(), ddsData.size());

	/// :( don't works....
	// GsTex tex = t2pic.GsTex0;
	// t2header.texSize = ddsSize;
	// t2pic.ImageWidth = w;
	// t2pic.ImageHeight = h;
	//
	// tex.TW = log2(w);
	// tex.TH = log2(h);
	//
	// t2pic.GsTex0 = tex;

	t2pic.imgSize = ddsSize;
	t2pic.totalSize = ddsSize + hS - t2pic.headerSize;


	tim2.insert(tim2.end(), reinterpret_cast<char*>(&t2header), reinterpret_cast<char*>(&t2header) + sizeof(t2header));
	if(t2header.formatId != 0x0)
	{
		tim2.insert(tim2.end(), addData.begin(), addData.end());
	}
	tim2.insert(tim2.end(), reinterpret_cast<char*>(&t2pic), reinterpret_cast<char*>(&t2pic) + sizeof(t2pic));
	if(addData2.size() != 0)
		tim2.insert(tim2.end(), addData2.begin(), addData2.end());
	tim2.insert(tim2.end(), ddsData.begin(), ddsData.end());
	_dds.close();

	if(isc)
	{
		std::vector<char> out(tim2.size());
		size_t csize = compress(tim2, out);
		out.resize(csize);

		tim2 = std::move(out);
	}

	std::string fn = origPath + ".bak";
	if(!std::filesystem::exists(fn))
	{
		std::ifstream _fib(origPath, std::ios::binary);
		std::ofstream _fob(fn, std::ios::binary);
		_fob << _fib.rdbuf();
		_fob.close();
		_fib.close();
	}

	if(tim2.size() % 2048 != 0)
	{
		size_t old = tim2.size();
		size_t newsize = (old + 2047) & ~2047;
		tim2.resize(newsize);
	}
	std::ofstream f(origPath, std::ios::binary);
	f.write(tim2.data(), tim2.size());
	f.close();
	

}

void tim2Unpack(std::vector<char>& buffer, const char* dirname, std::filesystem::path basename, bool isc)
{
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
		printErr("Supported only PC format of DMC2 textures");
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


	// for ex. item0.biz is compressed tim2, so... needed (also filename)
	char* _metan = new char[strlen(dirname) + 12];
	sprintf(_metan, "%s/.metadata", dirname);
	std::vector<std::string> _meta;
	_meta.push_back(basename.string());
	if(isc)
		_meta.push_back("_compressed");
	printf("-- Creating \"%s/.metadata\" file\n", dirname);
	std::ofstream _metadata(_metan, std::ios::out);
	for(const auto e : _meta)
	{
		_metadata << e << '\n';
	}
	delete [] _metan;
	_metadata.close();

	// dump a header or data to texture point
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

void ptxPack(std::string dirname, std::string metadataFile, std::string origPath)
{
	bool isc = false;
	std::string basename, line;
	std::filesystem::path _tp = dirname;
	std::vector<std::string> files;

	std::ifstream meta(metadataFile);
	std::getline(meta, basename);

	while(std::getline(meta, line))
	{
		if(line.find("_compressed") != std::string::npos)
		{
			isc = true;
			continue;
		}
		if(line == "") break;
		std::filesystem::path p = _tp;
		p.append("_" + line);
		if(std::filesystem::is_directory(p))
			pack(p.string().c_str());
		_tp = _tp.append(line);
		files.push_back(_tp.string());
		_tp = dirname;

	}
	meta.close();

	std::string fn = origPath + ".bak";
	if(!std::filesystem::exists(fn))
	{
		std::ifstream _fib(origPath, std::ios::binary);
		std::ofstream _fob(fn, std::ios::binary);
		_fob << _fib.rdbuf();
		_fob.close();
		_fib.close();
	}

	std::ostringstream f(std::ios::binary);

	std::vector<char> zeros(0x800, 0);
	f.write(zeros.data(), zeros.size());

	f.seekp(0, std::ios::beg);
	uint32_t count = files.size();
	f.write(reinterpret_cast<char*>(&count), sizeof(uint32_t));

	f.seekp(0x800, std::ios::beg);

	std::streampos p;
	for(size_t i = 0; i < files.size(); i++)
	{
		
		printf("-- Writing \"%s\"\n", files[i].c_str());
		std::ifstream _t(files[i], std::ios::binary | std::ios::ate);
		size_t fsize = _t.tellg();
		_t.seekg(0, std::ios::beg);
		printf("   Size: %lu\n", fsize);
		f << _t.rdbuf();
		p = f.tellp();
		f.seekp(i * sizeof(uint32_t) + sizeof(uint32_t), std::ios::beg);
		uint32_t ptxSize = fsize / 2048;
		f.write(reinterpret_cast<char*>(&ptxSize), sizeof(uint32_t));
		f.seekp(p);
		_t.close();
	}

	std::vector<char> buff;
	std::string _buff = f.str();
	buff.insert(buff.end(), _buff.begin(), _buff.end());
	if(isc)
	{
		printf("-- Compressing file\n");
		size_t _size = buff.size();
		std::vector<char> out(_size / 2);
		size_t csize = compress(buff, out);
		out.resize(csize);

		buff = std::move(out);
	}
	std::ofstream _f(origPath, std::ios::binary);
	_f.write(buff.data(), buff.size());
	_f.close();
}

void ptxUnpack(std::vector<char>& buffer, const char* dirname, std::filesystem::path basename, bool isc)
{
	std::istringstream f(std::string(buffer.begin(), buffer.end()), std::ios::binary);
	f.seekg(0, std::ios::beg);
	uint32_t count;
	f.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));
	std::vector<uint32_t> align; // tim2 size / 2048
	for(size_t i = 0; i < count; i++)
	{
		uint32_t _tu;
		f.read(reinterpret_cast<char*>(&_tu), sizeof(uint32_t));
		align.push_back(_tu);
	}
	f.seekg(0x800, std::ios::beg);	
	if(!std::filesystem::is_directory(dirname))
	{
		if(std::filesystem::create_directory(dirname))
		{
			printf("-- Directory \"%s\" created\n", dirname);
		}
	}

	char* tc = new char[strlen(dirname) + 12];
	sprintf(tc, "%s/.metadata", dirname);
	std::vector<std::string> metadataBuffer;
	metadataBuffer.push_back(basename.string());
	if(isc)
		metadataBuffer.push_back("_compressed");

	for(size_t i = 0; i < count; i++)
	{
		size_t s = align[i] * 2048;
		char* ufn = new char[6 + strlen(dirname) + 1 + 3];
		std::vector<char> buff(s);
		// f.seekg(o, std::ios::beg);
		f.read(buff.data(), buff.size());

		fileType ft = findFileType(buff, buff.size());

		sprintf(ufn, "%s/id%lu.%s", dirname, i, fileTypeExt(ft));
		printf("-- Writing to \"%s\", size: %lu\n", ufn, s);

		std::ofstream uf(ufn, std::ios::binary);
		uf.write(buff.data(), s);
		uf.close();

		std::filesystem::path ap = std::filesystem::absolute(ufn);
		std::filesystem::path bn = ap.filename();
		std::filesystem::path dn = ap.parent_path() / ("_" + bn.string());
		
		switch (ft) {
			case tim2:
				tim2Unpack(buff, dn.string().c_str(), bn, false);
				break;
			default:
				break;
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

void momoPack(std::string dirname, std::string metadataFile, std::string origPath)
{
	bool mb = false;
	std::ifstream _m(origPath, std::ios::binary);
	char c;
	_m.seekg(sizeof(uint32_t));
	_m.read(&c, 1);
	if(c == 'M' || c == 'O')
	{
		_m.seekg(sizeof(uint32_t) + 2);
		_m.read(&c, 1);
	}
	if(c != 0x0)
		mb = true;
	_m.close();
	
	bool isc = false;
	std::ifstream meta(metadataFile);
	std::string basename;
	std::string line;
	std::vector<std::string> files;
	std::getline(meta, basename);
	std::filesystem::path _tp = dirname;
	while(std::getline(meta, line))
	{
		if(line.find("_compressed") != std::string::npos)
		{
			isc = true;
			continue;
		}
		if(line == "") break;
		std::filesystem::path p = _tp;
		p.append("_" + line);
		if(std::filesystem::is_directory(p))
			pack(p.string().c_str());
		_tp = _tp.append(line);
		files.push_back(_tp.string());
		_tp = dirname;
	}
	meta.close();

	char magic[4] = {'M', 'O', 'M', 'O'};
	std::string fn = origPath + ".bak";
	if(!std::filesystem::exists(fn))
	{
		std::ifstream _fib(origPath, std::ios::binary);
		std::ofstream _fob(fn, std::ios::binary);
		_fob << _fib.rdbuf();
		_fob.close();
		_fib.close();
	}
	std::ostringstream f(std::ios::binary);
	size_t lastpos = 0;
	size_t lastoff = 0;
	size_t fsize = 0;

	size_t bcount = sizeof(uint64_t);
	if(mb)
		bcount = sizeof(uint32_t);

	f.write(magic, 4);
	if(!mb)
	{
		uint32_t q = 0;
		f.write(reinterpret_cast<char*>(&q), sizeof(uint32_t));
	}
	size_t tmp = files.size();
	f.write(reinterpret_cast<char*>(&tmp), bcount);
	char* _tmp = new char[(bcount*2) * files.size() + (bcount*2)];
	f.write(_tmp, (bcount*2) * files.size());
	delete [] _tmp;
	lastpos = f.tellp();
	if(lastpos % 64 != 0)
	{
		size_t old = lastpos;
		size_t newpos = (old + 63) & ~63;
		std::string _tmpBuff(newpos - lastpos, '\0');
		f.write(_tmpBuff.data(), _tmpBuff.size());
		lastpos = newpos;
	}	
	for(size_t i = 0; i < files.size(); i++)
	{
		
		printf("-- Writing \"%s\"\n", files[i].c_str());
		std::ifstream _t(files[i], std::ios::binary | std::ios::ate);
		lastoff = lastpos;
		fsize = _t.tellg();
		_t.seekg(0, std::ios::beg);
		printf("   Size: %lu\n", fsize);
		printf("   Offset: %lu\n", lastoff);
		f << _t.rdbuf();
		lastpos = f.tellp();
		if(lastpos % 64 != 0)
		{
			size_t old = lastpos;
			size_t newpos = (old + 63) & ~63;
			std::string _tmpBuff(newpos - lastpos, '\0');
			f.write(_tmpBuff.data(), _tmpBuff.size());
			lastpos = newpos;
		}
		f.seekp((bcount*2) + (bcount * 2) * i);
		f.write(reinterpret_cast<char*>(&lastoff), bcount);
		f.write(reinterpret_cast<char*>(&fsize), bcount);
		f.seekp(lastpos);
		_t.close();
	}
	lastpos = f.tellp();

	std::vector<char> buff;
	std::string _buff = f.str();
	buff.insert(buff.end(), _buff.begin(), _buff.end());
	std::ofstream __f(origPath + ".bin", std::ios::binary);
	__f.write(buff.data(), buff.size());
	__f.close();
	if(isc)
	{
		printf("-- Compressing file\n");
		size_t _size = buff.size();
		std::vector<char> out(_size / 2);
		size_t csize = compress(buff, out);
		out.resize(csize);

		buff = std::move(out);
	}
	std::ofstream _f(origPath, std::ios::binary);
	_f.write(buff.data(), buff.size());
	_f.close();
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
		size_t o, s;
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
				break;
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

void pack(const char* dirname)
{
	if(!std::filesystem::is_directory(dirname))
	{
		printErr("Directory not exists");
		return;
	}

	std::filesystem::path _dname = std::filesystem::absolute(dirname);
	_dname = std::filesystem::canonical(_dname);

	std::filesystem::path _metadataFile = _dname;
	_metadataFile = _metadataFile.append(".metadata");
	if(!std::filesystem::exists(_metadataFile))
	{
		printf("-- Metadata file not exists\n");
		return;
	}
	std::ifstream meta(_metadataFile.string());
	std::string basename;
	std::getline(meta, basename);
	meta.close();
	std::filesystem::path filePath = _dname.parent_path();
	filePath = filePath.append(basename);

	fileType ft = findFileType(filePath.string().c_str());
	printf("-- Packing file \"%s\"\n", basename.c_str());
	printf("-- File type: %s\n", fileTypeExt(ft));
	switch (ft) {
		case momo:
			momoPack(_dname.string(), _metadataFile.string(), filePath.string());
			break;
		case ptx:
			ptxPack(_dname.string(), _metadataFile.string(), filePath.string());
			break;
		case tim2:
			tim2Pack(_dname.string(), _metadataFile.string(), filePath.string());
			break;
		case ipu:
			ipumPack(_dname.string(), _metadataFile.string(), filePath.string());
			break;
		default:
			return;
	}
}

void unpack(const char* filename)
{
	if(!std::filesystem::exists(filename) || std::filesystem::is_directory(filename))
	{
		printErr("File not exists");
		return;
	}

	std::ifstream f(filename, std::ios::binary);

	if(!f.is_open())
	{
		printErr("Failed to open file");
		return;
	}

	f.seekg(0, std::ios::end);
	size_t fsize = f.tellg();
	f.seekg(0, std::ios::beg);
	std::vector<char> buff(fsize);
	f.read(buff.data(), buff.size());
	f.close();

	if (fsize < 16){
		printErr("Wrong file?");
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

void doSomethingWithFile(const char* input, bool isP)
{
	if(!isP)
		unpack(input);
	else
		pack(input);

}

int main(int argc, char** argv)
{
	printf("== Shitty repacker from deadYokai ==\n");
	if(argc <= 1){
		printHelp(argv[0]);
		return 0;
	}

	bool isPack = false;

	if(argv[2])
	{
		if(strcmp(argv[2], "-p") == 0 || strcmp(argv[2], "--pack") == 0)
			isPack = true;
		else if(strcmp(argv[2], "-e") == 0 || strcmp(argv[2], "--extract") == 0)
			isPack = false;
		else
		{
			printUsageError(argv[0], argv[2]);
			return 0;
		}
	}

	printf("-- Input file: '%s'\n", argv[1]);
	doSomethingWithFile(argv[1], isPack);
	return 1;
}

