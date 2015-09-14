//============================================================================
// Name        : SecureDiskImage.cpp
// Author      : LYP
// Version     :
// Copyright   : Your copyright notice
// Description : Secure Disk Image
//============================================================================

#include <stdint.h>
#include <iostream>
#include <fstream>

#define SectorSize 512

using namespace std;

void EncryptSector(uint8_t *data) {
	uint8_t temp;
	for (int i = 0; i < SectorSize / 2; i++) {
		temp = data[i];
		data[i] = data[SectorSize - 1 -i];
		data[SectorSize - 1 -i] = temp;
	}
}

void DecryptSector(uint8_t *data) {
	uint8_t temp;
	for (int i = 0; i < SectorSize / 2; i++) {
		temp = data[i];
		data[i] = data[SectorSize - 1 -i];
		data[SectorSize - 1 -i] = temp;
	}
}

int main() {
	string input_file_name = "./system/disks/x86root.img";
	// cout << input_file_name << endl;

	string output_file_name = "./system/disks/x86root_secure.img";
	// cout << output_file_name << endl;

	ios::openmode input_mode = ios::in | ios::binary;
	ios::openmode output_mode = ios::out | ios::binary;

	fstream input_stream;
	input_stream.open(input_file_name.c_str(), input_mode);
	if(!input_stream.is_open()) {
		printf("Error opening input file : %s", input_file_name.c_str());
	}

	fstream output_stream;
	output_stream.open(output_file_name.c_str(), output_mode);
	if(!output_stream.is_open()) {
		printf("Error opening output file : %s", output_file_name.c_str());
	}

	input_stream.seekg(0, ios::end);
	streampos disk_size = input_stream.tellg();
	// printf("disk size is %d.\n", disk_size);

	uint8_t *data = new uint8_t[SectorSize];

	input_stream.seekg(0, ios::beg);
	output_stream.seekp(0, ios::beg);

	streampos i;
	for (i = 0; i < disk_size; i += SectorSize) {
		streampos input_pos = input_stream.tellg();
		input_stream.read((char *)data, SectorSize);
		int read_size = input_stream.tellg() - input_pos;
		// if (read_size != SectorSize) {
		//     printf("Can't read only %d of %d bytes at %d.\n", read_size, SectorSize, input_stream.tellg());
		// }
		//
		EncryptSector(data);
		//
		streampos output_pos = output_stream.tellp();
		output_stream.write((const char *)data, SectorSize);
		int write_size = output_stream.tellp() - output_pos;
		// if (write_size != SectorSize) {
		//     printf("Can't write only %d of %d bytes at %d.\n", read_size, SectorSize, output_stream.tellp());
		// }
	}
	// printf("read %d bytes.\n", i);
	printf("job done.\n");

	input_stream.close();
	output_stream.close();

	return 0;
}
