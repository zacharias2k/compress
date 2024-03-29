//-----------------------------------------------------------------------------
// main.cpp
// This file is part of compress
//-----------------------------------------------------------------------------
// Copyright(C) 2014 Markus Sengthaler <m.sengthaler@gmail.com>
//
// This program is free software; you can redistribute it and / or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or(at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
//-----------------------------------------------------------------------------


#include <string>
#include <omp.h>
#include "recursiveword.h"

//#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

size_t getFileSize(FILE* file)
{
	size_t size = 0;
	_fseeki64(file, 0, SEEK_END);
	size = _ftelli64(file);
	if (size == (size_t)-1)
	{
		printf("file size error\n");
		return 0;
	}
	_fseeki64(file, 0, SEEK_SET);
	return size;
}

void execCompression(FILE* in, FILE* out, size_t maxBlockSize)
{
	printf("compressing\n");
	size_t fileSize = getFileSize(in);
	if (maxBlockSize == 0)
		maxBlockSize = fileSize;

	long long nBlocks = (size_t)ceil((double)fileSize / maxBlockSize);

#pragma omp parallel for
	for (long long iBlock = 0; iBlock < nBlocks; iBlock++)
	{
		size_t sizeLeft = fileSize - iBlock * maxBlockSize;
		size_t curBlockSize = min(sizeLeft, maxBlockSize);
		printf("processing block # %d, size = %d\n", iBlock, curBlockSize);

		byte* data = new byte[curBlockSize];
		// read
		fread(data, curBlockSize, 1, in);
		// compress
		recursive_block_compress(data, curBlockSize);
		// write out
		fwrite(data, curBlockSize, 1, out);

		delete[] data;
	}
	printf("finished\n");
}

void execDecompression(FILE* in, FILE* out)
{
	printf("decompressing\n");
	size_t fileSize = getFileSize(in);
	size_t blockNo = 0;

	while ((size_t)_ftelli64(in) < fileSize)
	{
		printf("processing block # %d\n", blockNo++);
		// get block size
		byte idSize = 0;
		byte wSize = 0;
		fread(&idSize, 1, 1, in);	// id size or terminator
		if (idSize != 0)
		{
			_fseeki64(in, idSize, SEEK_CUR);
			fread(&wSize, 1, 1, in);
			_fseeki64(in, wSize, SEEK_CUR);
		}
		size_t dataSize = 0;
		fread(&dataSize, sizeof(size_t), 1, in);

		long long headerSize = 1 + sizeof(size_t);
		if (idSize != 0)
			headerSize += 1 + idSize + wSize;

		_fseeki64(in, -headerSize, SEEK_CUR);	// rewind

		size_t blockSize = dataSize + headerSize;
		byte* data = new byte[blockSize];
		// read whole data
		fread(data, blockSize, 1, in);
		// decompress
		recursive_block_decompress(data, blockSize);
		// write data
		fwrite(data, blockSize, 1, out);
		// cleanup
		delete[] data;
	}
	printf("finished\n");
}

void displayHelp()
{
	printf("useage:\n\
[-rf|-e] [infile] outfile [blocksize]\n\
compress with infile outfile; if no blocksize specified,\n\
 it is assumed to be equal to the file size\n\
-rf\tcreates a random file, no infile parameter necessary\n\
-e\textracts the infile into outfile\n");
}

void main(int argc, char** argv)
{
	bool decompressing = false;
	argv++;
	argc--;

	if (argc < 2)
	{
		displayHelp();
		return;
	}

	if (*argv && strcmp(*argv, "-rf") == 0)
	{
		argv++;
		argc--;
		const char* fileName = *argv;
		if (argc < 0 || !fileName || strlen(fileName) == 0)
		{
			printf("no file specified\n");
			return;
		}

		argc--;
		argv++;

		if (argc < 1)
		{
			printf("no file size specified\n");
			return;
		}

		int fileSize = atoi(*argv);
		if (fileSize == 0)
		{
			printf("nofile size 0 not possible\n");
			return;
		}

		// create random file
		FILE * file = fopen(fileName, "wb");
		for (long i = 0; i < fileSize; i++)
		{
			unsigned char rndChar = rand() % UCHAR_MAX;
			fputc(rndChar, file);
		}

		fclose(file);
		return;
	}
	else if (*argv && strcmp(*argv, "-e") == 0)
	{
		decompressing = true;
		argc--;
		argv++;
	}

#ifdef _OPENMP
	printf("OMP enabled\n");
#endif

	const char* fileName = *argv;
	if (argc < 0 || !fileName || strlen(fileName) == 0)
	{
		printf("no in file specified\n");
		return;
	}
	// in file
	FILE * in_file = fopen(fileName, "rb");
	if (!in_file)
	{
		printf("file not found\n");
		return;
	}
	argc--;
	argv++;
	const char* outFileName = *argv;
	if (argc < 0 || !outFileName || strlen(outFileName) == 0)
	{
		printf("no out file specified\n");
		return;
	}
	// out file
	FILE* out_file = fopen(outFileName, "wb");
	if (!out_file)
	{
		printf("error opening output file\n");
		return;
	}
	argc--;
	argv++;

	if (!decompressing)
	{
		// block size
		long maxBlockSize = 0;
		if (argc > 0)
			maxBlockSize = atoi(*argv);

		execCompression(in_file, out_file, maxBlockSize);
	}
	else
		execDecompression(in_file, out_file);

	fclose(in_file);
	fclose(out_file);
}
