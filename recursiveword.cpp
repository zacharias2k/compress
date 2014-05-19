//-----------------------------------------------------------------------------
// recursiveword.cpp
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

#include "recursiveword.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <omp.h>


size_t searchForWord(const byte* data, size_t dataSize, const byte* word, byte wordSize)
{
	size_t occurences = 0;
	const byte* r = data;
	const byte* last = data + dataSize;
	while (r != 0)
	{
		if (r = (const byte*)memchr(r, *word, last - r))
		{
			if(!memcmp(r, word, wordSize))
				occurences++;
			r++;
		}
	}
	return occurences;
}

bool doesWordOccur(const byte* data, size_t dataSize, const byte* word, byte wordSize)
{
	size_t occurences = 0;
	const size_t redDataSize = dataSize - wordSize;
	for (long long i = 0; i <= (long long)redDataSize; i++)
	{
		const byte* pData = data + i;
		if (memcmp(pData, word, wordSize) == 0)
			return true;
	}

	return false;
}

bool searchForSmallestNonExistingWord(const byte* data, size_t dataSize, Word& wordOut)
{
	byte word[MAX_WORD_SIZE];

	// larger than 4 is less efficient than index based
	for (byte wordSize = 1; wordSize <= 4; wordSize *= 2)
	{
#ifdef _DEBUG
		printf("checking word size %d\n", wordSize);
#endif

		memset(word, 0, sizeof(byte)*wordSize);

		for (size_t i = 0; i < pow((float)256, (size_t)wordSize); i++)
		{
			// check each possibility
			word[0]++;
			for (int i = 1; i < wordSize; i++)
				if (word[i - 1] == 0)
					word[i]++;

			if (!doesWordOccur(data, dataSize, word, wordSize))
			{
				wordOut = Word(word, wordSize, 0);
				return true;
			}
		}
	}

	return false;
}

bool contains(const std::vector<Word>& vec, const byte* word, byte wordSize)
{
	for (const Word& w : vec)
	{
		if (w.wordSize == wordSize &&
			memcmp(w.word, word, wordSize) == 0)
			return true;
	}
	return false;
}

std::vector<Word> searchForLargestExistingWords(const byte* data, size_t dataSize)
{
	std::vector<Word> words;
	long maxWordSize = 128;

	if (dataSize < maxWordSize)
	{
		// make it a power of 2
		double ll = log2(dataSize);
		maxWordSize = (long)pow(2, (long)floor(ll));
	}

	for (byte wordSize = (byte)maxWordSize; wordSize > 2; wordSize /= 2)
	{
#ifdef _DEBUG
		printf("checking word size %d\n", wordSize);
#endif
		for (const byte* word = data; word <= data + dataSize - wordSize; word++)
		{
			const byte* searchStart = word;
			size_t searchLength = dataSize - (word - data);
			size_t nOccurences = searchForWord(searchStart, searchLength, word, wordSize);
			if (nOccurences > 1 && !contains(words, word, wordSize))
				words.push_back(Word(word, wordSize, nOccurences));
		}
	}

	return words;
}

long long replaceSafings(const Word& w, long idOverhead)
{
	return w.occurence * (w.wordSize - idOverhead) - w.wordSize - idOverhead - (sizeof(char)+sizeof(char)+sizeof(size_t));
}

bool extractBestMatch(std::vector<Word>& words, byte idWordOverhead, Word& result)
{
	size_t nWords = words.size();
	
	// evaluate the safings
	long long safe = replaceSafings(words.front(), idWordOverhead);
	size_t safeIdx = 0;
	for (size_t i = 1; i < nWords; i++)
	{
		long long iSafe = replaceSafings(words[i], idWordOverhead);
		if (iSafe > safe)
		{
			safe = iSafe;
			safeIdx = i;
		}
	}

#ifdef _DEBUG
	printf("safing: %d bytes\n", safe);
#endif
	if (safe > 0)
	{
		result = words[safeIdx];
		return true;
	}
	return false;
}

byte* create_compressed_block(byte* data, size_t dataSize, const Word& idWord, const Word& rplWord, size_t& compressedSize)
{
	byte* tmpData = new byte[dataSize];
	byte* pTmpData = tmpData;

	// dump id len
	*pTmpData = idWord.wordSize;
	pTmpData++;
	// dump id
	memcpy(pTmpData, idWord.word, idWord.wordSize);
	pTmpData += idWord.wordSize;
	// dump word len
	*pTmpData = rplWord.wordSize;
	pTmpData++;
	// dump word
	memcpy(pTmpData, rplWord.word, rplWord.wordSize);
	pTmpData += rplWord.wordSize;

	// safe pointer for later block size dumping
	byte* pBlockSize = pTmpData;
	pTmpData += sizeof(size_t);

	// replace
	byte* pData = data;
	while ((size_t)(pData - data) < dataSize)
	{
		if (memcmp(pData, rplWord.word, rplWord.wordSize) == 0)
		{
			// replace
			memcpy(pTmpData, idWord.word, idWord.wordSize);
			pTmpData += idWord.wordSize;
			pData += rplWord.wordSize;
		}
		else
		{
			// copy char
			*pTmpData = *pData;
			pTmpData++;
			pData++;
		}
	}
	size_t blockSize = pTmpData - pBlockSize - sizeof(size_t);
	memcpy(pBlockSize, &blockSize, sizeof(size_t));

	compressedSize = pTmpData - tmpData;

	return tmpData;
}

void create_uncompressed_block(byte*& data, size_t& dataSize)
{
	byte* tmp = new byte[dataSize + sizeof(char) + sizeof(size_t)];
	byte *p = tmp;
	// add null terminator
	*p = 0;
	p++;
	// add block size
	memcpy(p, &dataSize, sizeof(size_t));
	p += sizeof(size_t);
	// copy data
	memcpy(p, data, dataSize);
	// delete old data
	delete[] data;
	// replace
	data = tmp;
	dataSize = dataSize + sizeof(char)+sizeof(size_t);
}

void _recursive_block_compress(byte*& data, size_t& dataSize)
{
	// one replacement per layer
	Word idWord;
	bool foundId = searchForSmallestNonExistingWord(data, dataSize, idWord);

	std::vector<Word> rplWords = searchForLargestExistingWords(data, dataSize);
	if (rplWords.size() == 0)
	{
#ifdef _DEBUG
		printf("no replacement found, cancel\n");
#endif
		return;
	}
	Word rplWord;
	if (!extractBestMatch(rplWords, idWord.wordSize, rplWord))
	{
#ifdef _DEBUG
		printf("deficient compression, cancel\n");
#endif
		return;
	}

	if (foundId)
	{
		//printf("found id with len %d\n", idWord.wordSize);
		size_t compressedSize = 0;
		byte* compressedData = create_compressed_block(data, dataSize, idWord, rplWord, compressedSize);
		printf("compression ratio: %g\n", (float)compressedSize / dataSize);

		delete[] data;
		data = compressedData;
		dataSize = compressedSize;

		// recursive call
		_recursive_block_compress(data, dataSize);
	}
}

void recursive_block_compress(byte*& data, size_t& dataSize)
{
	create_uncompressed_block(data, dataSize);
	_recursive_block_compress(data, dataSize);
}


void recursive_block_decompress(byte*& data, size_t& dataSize)
{
	size_t rawDataSize = 0;
	byte* pData = data;
	// get id size
	byte idSize = *pData;
	pData++;
	if (idSize == 0)
	{	
		// terminator, just return data
		memcpy(&rawDataSize, pData, sizeof(size_t));
		pData += sizeof(size_t);
		byte* tmp = new byte[rawDataSize];
		memcpy(tmp, pData, rawDataSize);
		delete data;

		data = tmp;
		dataSize = rawDataSize;
		return;
	}
	// get id word
	byte* idWord = new byte[idSize];
	memcpy(idWord, pData, idSize);
	pData += idSize;
	// get replacing word size
	byte rplSize = *pData;
	pData++;
	// get replacing word
	byte* rplWord = new byte[rplSize];
	memcpy(rplWord, pData, rplSize);
	pData += rplSize;
	// get raw data size
	memcpy(&rawDataSize, pData, sizeof(size_t));
	pData += sizeof(size_t);

	// calc new size
	size_t occurences = searchForWord(pData, rawDataSize, idWord, idSize);
	size_t newDataSize = rawDataSize + occurences*(rplSize - idSize);
	byte* newData = new byte[newDataSize];
	byte* pNewData = newData;
	// replace
	while ((size_t)(pData - data) < dataSize)
	{
		if (memcmp(pData, idWord, idSize) == 0)
		{
			// place replacement
			memcpy(pNewData, rplWord, rplSize);
			pData += idSize;
			pNewData += rplSize;
		}
		else
		{
			// copy char
			*pNewData = *pData;
			pNewData++;
			pData++;
		}
	}
	delete[] data;
	data = newData;
	dataSize = newDataSize;

	recursive_block_decompress(data, dataSize);
}