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