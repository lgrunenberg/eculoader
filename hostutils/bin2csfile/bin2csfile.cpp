#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
using namespace std;

#include <cstdio>
#define len(a) (sizeof(a)/sizeof(*a))

string GetFileName(string f)
{
	int split = f.find_last_of('\\');
	f.erase(0, split + 1);
	return f;
}

// Love me some pointers! :D
string print(char *data)
{
	stringstream ss;

	// Space out
	ss << "            ";

	for (char i = 0; i < 8; i++)
	{
		ss << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char)data[i];
		ss << ", ";
	}

	ss << "\n";

	// Pre-clean for next frame
	for (char i = 0; i < 8; i++)
		data[i] = 0;

	return ss.str();
}


int main(int argc, char** argv)
{
	// string executable = argv[0];
	// executable = GetFileName(executable);

	if (argc < 3)
	{
		cout << "\nNot enough arguments!\n";
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
	string loadername = argv[2];
	string filename = loadername + ".cs";

	ifstream infile;
	infile.open(path.c_str(), ifstream::binary);

	infile.seekg(0, ifstream::end);
	streamoff Len = infile.tellg();
	infile.seekg(0, ifstream::beg);

	char *data = (char*)malloc((int)Len);
	if (!data)
	{
		cout << "\nCould not allocate buffer\n";
		return 1;
	}
	// char data[32768];
	char *dataptr = &data[0];

	infile.read(data, Len);
	infile.close();


	ofstream outfile(filename.c_str());

	outfile << "using System;\n";
	outfile << "using System.Collections.Generic;\n";
	outfile << "using System.Text;\n";
	outfile << "namespace TrionicCANLib\n{\n";
//  outfile << "    public class Bootloader_Test\n    {\n";
	outfile << "    public class " + loadername + "\n    {\n";//Bootloader_Test\n    {\n";
	outfile << "        private byte[] m_" + loadername +/*Bootloader_Test*/" =  {\n";

	int  Loc = 0;

	while (Loc < Len)
	{
		stringstream ss;
		int linelen = 8;
		int i;
		int left = (int)Len - Loc;
		if (linelen > left)
			linelen = left;
		Loc += linelen;
		// Space out
		ss << "            ";
		for (i = 0; i < linelen; i++)
		{
			ss << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char)*dataptr++;
			ss << ", ";
		}
		// Pad if need be
		for (i; i < 8; i++)
		{
			ss << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char)0;
			ss << ", ";
		}

		ss << "\n";

		outfile << (string)ss.str();
	}

	outfile << "        };\n\n";

	// Print out supportive code
	outfile << "        public byte[] " + loadername + "Bytes\n        {\n";
	outfile << "            get{ return m_" + loadername + "; }\n";
	// outfile << "        }\n\n";
	// outfile << "        public int BootloaderTestLength\n        {\n";
	// outfile << "            get{ return m_Bootloader_Test.Length; }\n";
	outfile << "        }\n    }\n}\n";

	outfile.close();
	std::free(data);

	return 0;
}



