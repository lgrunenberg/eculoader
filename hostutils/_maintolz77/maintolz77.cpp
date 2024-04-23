#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdio>

using namespace std;

#define LZ_MAX_OFFSET 4096

static inline uint32_t LZ_StringCompare(uint8_t *str1, uint8_t *str2,
	uint32_t minlen, uint32_t maxlen)
{
	for ( ; (minlen < maxlen) && (str1[minlen] == str2[minlen]); minlen++);
	return minlen;
}

int LZ_Compress(uint8_t *in, uint8_t *out, uint32_t insize)
{
	// Do we have anything to compress?
	if (!insize)
		return 0;

	// Start of compression
	uint8_t *flags = (uint8_t*)&out[8];
	*flags = 0;
	uint8_t mask = 0x80;

	int32_t  bytesleft = insize;
	uint32_t inpos = 0;
	uint32_t outpos = 9;

	// Main compression loop
	do {
		// Determine most distant position
		int32_t maxoffset = inpos > LZ_MAX_OFFSET ? LZ_MAX_OFFSET : inpos;

		// Get pointer to current position
		uint8_t *ptr1 = (uint8_t *)&in[inpos];

		// Search history window for maximum length string match
		uint32_t bestlength = 2;
		uint32_t bestoffset = 0;

		for (int32_t offset = 3; offset <= maxoffset; offset++)
		{
			// Get pointer to candidate string
			uint8_t *ptr2 = (uint8_t *)&ptr1[-offset];

			// Quickly determine if this is a candidate (for speed)
			if ((ptr1[0] == ptr2[0]) &&
				(ptr1[bestlength] == ptr2[bestlength]))
			{
				// Determine maximum length for this offset
				// Count maximum length match at this offset
				uint32_t length = LZ_StringCompare(ptr1, ptr2, 0, ((inpos + 1) > 18 ? 18 : inpos + 1));

				// Better match than any previous match?
				if (length > bestlength)
				{
					bestlength = length;
					bestoffset = offset;
				}
			}
		}

		// Was there a good enough match?
		if (bestlength > 2)
		{
			*flags |= mask;
			mask >>= 1;

			out[outpos++] = (bestlength - 3) << 4 | (((bestoffset - 1) & 0xF00) >> 8);
			out[outpos++] = (bestoffset - 1) & 0xFF;
			inpos += bestlength;
			bytesleft -= bestlength;

			if (!mask)
			{
				mask = 0x80;
				flags = &out[outpos++];
				*flags = 0;
			}
		}
		else
		{
			mask >>= 1;
			out[outpos++] = in[inpos++];
			bytesleft--;

			if (!mask)
			{
				mask = 0x80;
				flags = &out[outpos++];
				*flags = 0;
			}
		}

	} while (bytesleft > 3);

	// Dump remaining bytes, if any
	while (inpos < insize)
	{
		mask >>= 1;
		out[outpos++] = in[inpos++];
		if (!mask)
		{
			mask = 0x80;
			flags = &out[outpos++];
			*flags = 0;
		}
	}

	while (outpos & 3)
		out[outpos++] = 0;

	*(uint32_t *)&out[4] = outpos; // Size of compressed data
	out[4] = outpos >> 24;
	out[5] = outpos >> 16;
	out[6] = outpos >> 8;
	out[7] = outpos;

	uint32_t checksum = 0;
	// Doing it this way means the sum should be 0 when verifying the compressed data
	for (uint32_t i = 4; i < outpos; i++)
		checksum += out[i];
	checksum = 0 - checksum;

	out[0] = checksum >> 24;
	out[1] = checksum >> 16;
	out[2] = checksum >>  8;
	out[3] = checksum;
	return outpos;
}

// LZ_Compress(uint8_t *in, uint8_t *out, uint32_t insize)

typedef struct {
	const uint32_t checksum;
	const uint32_t sizeIn;
} lzImage_t;


static bool lzExtract(const lzImage_t* img, uint8_t* out)
{
	uint32_t chksum = img->checksum;
	uint32_t insize = img->sizeIn;
	uint32_t outpos = 0, inpos = 0;
	const uint8_t* in = (uint8_t*)img;

	chksum = ((chksum << 24) | ((chksum >> 8) & 0xff) << 16 | ((chksum >> 16) & 0xff) << 8 | ((chksum >> 24)&0xff));
	insize = ((insize << 24) | ((insize >> 8) & 0xff) << 16 | ((insize >> 16) & 0xff) << 8 | ((insize >> 24)&0xff));

	for (uint32_t i = 4; i < insize; i++)
		chksum += in[i];

	if (chksum != 0)
	{
		cout << "Generated image has the wrong checksum\n";
		return false;
	}

	in += 8;
	insize -= 8;

	while (inpos < insize)
	{
		uint32_t flags = in[inpos++];

		for (uint32_t mask = 0x80; inpos < insize && mask; mask = (mask >> 1))
		{
			if (flags & mask)
			{
				uint32_t length = ((in[inpos] >> 4) & 15) + 3;
				uint32_t ofs = ((in[inpos] & 15) << 8 | in[inpos + 1]) + 1;

				inpos += 2;

				for (uint32_t i = 0; i < length; i++)
				{
					out[outpos] = out[outpos - ofs];
					outpos++;
				}
			}
			else
			{
				out[outpos++] = in[inpos++];
			}
		}
	}

	return true;
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		cout << "\nSpecify file in AND out!\n";
		return 1;
	}

	FILE* test;
	test = fopen(argv[1], "r");
	if (!test)
	{
		cout << "\nNo file!\n";
		return 1;
	}

	fclose(test);

	string path = argv[1];
	string filename = argv[2];

	ifstream infile;
	infile.open(path.c_str(), ifstream::binary);

	infile.seekg(0, ifstream::end);
	int Len = (int)infile.tellg();
	infile.seekg(0, ifstream::beg);

	char *dataIn = (char*)malloc(Len + 64);
	if (!dataIn)
	{
		cout << "\nCould not allocate file buffer!\n";
		return 1;
	}

	uint8_t* dataOut = (uint8_t*)malloc(Len * 2);
	if (!dataOut)
	{
		std::free(dataIn);
		cout << "\nCould not allocate compression buffer!\n";
		return 1;
	}

	uint8_t* dataCompare = (uint8_t*)malloc(Len + 64);
	if (!dataCompare)
	{
		std::free(dataOut);
		std::free(dataIn);
		cout << "\nCould not allocate compare buffer!\n";
		return 1;
	}

	memset(dataCompare, 0x55, Len + 64);

	infile.read(dataIn, Len);
	infile.close();

	uint32_t complen = LZ_Compress((uint8_t*)dataIn, dataOut, (unsigned int)Len);

	if ( !lzExtract((lzImage_t*)dataOut, dataCompare) )
	{
		cout << "\nCould not verify image!\n";
		std::free(dataIn);
		std::free(dataOut);
		std::free(dataCompare);
		return 1;
	}

	if (memcmp(dataIn, dataCompare, (size_t)Len) != 0)
	{
		cout << "\nExtracted iamge is not identical to the original!\n";
		std::free(dataIn);
		std::free(dataOut);
		std::free(dataCompare);
		return 1;
	}

	ofstream outfile(filename.c_str(), ofstream::binary);
	for (uint32_t i = 0; i < complen; i++)
		outfile << dataOut[i];

	outfile.close();

	std::free(dataIn);
	std::free(dataOut);
	std::free(dataCompare);

	return 0;
}



