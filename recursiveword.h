//-----------------------------------------------------------------------------
// recursiveword.h
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

#define MAX_WORD_SIZE 256

#define byte unsigned char

struct Word
{
	Word(const byte* word, byte wordSize, size_t occurence) :
		wordSize(wordSize), occurence(occurence)
	{
		memcpy(this->word, word, wordSize);
	}
	Word()
	{
	}
	byte word[MAX_WORD_SIZE];
	byte wordSize;

	size_t occurence;
};

void recursive_block_compress(byte*& data, size_t& dataSize);
void recursive_block_decompress(byte*& data, size_t& dataSize);