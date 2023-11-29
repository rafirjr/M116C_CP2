#include "cache.h"
#include <bitset>

cache::cache()
{
	for (int i = 0; i < L1_CACHE_SETS; i++)
		L1[i].valid = false;
	for (int i = 0; i < L2_CACHE_SETS; i++)
		for (int j = 0; j < L2_CACHE_WAYS; j++)
			L2[i][j].valid = false;

	// Do the same for Victim Cache ...
	for (int i = 0; i < VICTIM_SIZE; i++)
		VC[i].valid = false;

	this->myStat.missL1 = 0;
	this->myStat.missL2 = 0;
	this->myStat.accL1 = 0;
	this->myStat.accL2 = 0;

	// Add stat for Victim cache ...
}
void cache::controller(bool MemR, bool MemW, int *data, int adr, int *myMem)
{
	// add your code here
	addressInfo adrInfo; // Struct for storing index, offset, tag, and address
	int block[4];		 // byte addresses to update (block indices in MAIN MEMORY)
	setBlock(block, adr);

	bitset<32> address = bitset<32>(adr);
	adrInfo = decode(address); // Function to populate addressInfo struct

	if (MemR)
	{
		// cout << "LOAD" << endl;
	}
	else if (MemW)
	{
		// cout << "STORE" << endl;
		if (containsL1(adrInfo)) // Check to see if data exists in L1 Direct Map
		{
			updateDataL1(adrInfo, data);
		}
		//  else if (containsVC())
		//  {
		//  	// update LRU position
		//  }
		//  else if (containsL2())
		//  {
		//  	// update LRU position
		//  }

		// updateMainMem();
	}
}

addressInfo cache::decode(bitset<32> adr)
{
	// L1 index is 4 bits
	// L1 offset is 2 bits
	// L1 tag is 26 bits

	// Victim index is 0 bits
	// Victim offset is 2 bits
	// Tag is 30 bits

	// L2 index is 4 bits
	// L2 offset is 2 bits
	// L2 tag is 26 bits

	bitset<2> offset;
	bitset<4> index;
	bitset<26> tag;
	addressInfo currBlk;

	for (int i = 0; i < 2; i++)
	{
		offset[i] = adr[i]; // storing offset
	}
	for (int i = 2; i < 6; i++)
	{
		index[i - 2] = adr[i];
	}
	for (int i = 6; i < 32; i++)
	{
		tag[i - 6] = adr[i];
	}

	currBlk.address = adr.to_ulong();
	currBlk.index = index.to_ulong();
	currBlk.offset = offset.to_ulong();
	currBlk.tag = tag.to_ulong();

	return currBlk;
}

void cache::setBlock(int *block, int adr)
{
	int temp = adr % 4;
	temp = adr - temp;

	for (int i = 0; i < 4; i++)
	{
		block[i] = temp + i;
		cout << block[i] << endl;
	}
}

// Checks if block exists in L1 cache
bool cache::containsL1(addressInfo adrInfo)
{
	int index = adrInfo.index;
	if (L1[index].valid)
	{
		if (adrInfo.tag == L1[index].tag)
		{
			return true;
		}
	}

	return false;
}

// Updates value in L1 cache
bool cache::updateDataL1(addressInfo adrInfo, int *data)
{
	cout << "Data before SW: " << L1[adrInfo.index].data << endl;
	L1[adrInfo.index].data = *data;
	cout << "Data after SW: " << L1[adrInfo.index].data << endl;
	return true;
}