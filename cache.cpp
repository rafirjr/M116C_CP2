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
	this->myStat.missVic = 0;
	this->myStat.accVic = 0;

	this->vcFull = false;
}
void cache::controller(bool MemR, bool MemW, int *data, int adr, int *myMem)
{
	// add your code here
	addressInfo adrInfo; // Struct for storing index, offset, tag, and address
	cacheBlock tempCacheBlock;
	cacheBlock tempCacheBlock2;
	int block[4]; // byte addresses to update (block indices in MAIN MEMORY)
	setBlock(block, adr);

	bitset<32> address = bitset<32>(adr);
	adrInfo = decode(address); // Function to populate addressInfo struct

	// Victim Cache VARIABLES
	int vcIndex;
	int vcLRU;

	int L2index;

	if (MemR)
	{
		// cout << "LOAD" << endl;
		if (containsL1(adrInfo))
		{
			cout << "Block exists in L1" << endl;
			this->myStat.accL1++;
		}
		else if (containsVC(&adrInfo))
		{
			this->myStat.accVic++;
			this->myStat.missL1++; // Missed in L1
			cout << "Block exists in VC" << endl;
			// Need to move block into L1
			if (!L1[adrInfo.index].valid) // If empty available block in L1
			{
				L1[adrInfo.index].data = *data;
				L1[adrInfo.index].lru_position = 0;
				L1[adrInfo.index].tag = adrInfo.tag;
				L1[adrInfo.index].valid = true;
				// TODO add block of MEM to L1 cache

				VC[adrInfo.victimPos].valid = false;
				// TODO update LRU
				for (int i = 0; i < 4; i++)
				{
					if (VC[i].valid && VC[i].lru_position < VC[adrInfo.victimPos].lru_position)
						VC[i].lru_position++;
				}
			}
			else // Specified index is taken by another block
			{
				tempCacheBlock = L1[adrInfo.index]; // Need to move this down to VC
				L1[adrInfo.index].data = *data;
				// L1[adrInfo.index].lru_position = 0;
				L1[adrInfo.index].tag = adrInfo.tag;
				L1[adrInfo.index].valid = true;
				// TODO add block of MEM to L1 cache

				// add tempCacheBlock to VC here... (VC tag = L1->tag + L1->index)
				tempCacheBlock.lru_position = VICTIM_SIZE - 1;
				VC[adrInfo.victimPos] = tempCacheBlock;
				VC[adrInfo.victimPos].victimTag = tempCacheBlock.tag + adrInfo.index;
				for (int i = 0; i < 4; i++) // Reduces LRU position by 1 for all positions above current position
				{
					if (VC[i].valid && VC[i].lru_position > L1[adrInfo.index].lru_position && adrInfo.victimPos != i) // VC might not be full
						VC[i].lru_position--;
				}
				L1[adrInfo.index].lru_position = 0; // No need for lru_position in L1 cache
			}
		}
		else if (containsL2(&adrInfo))
		{
			this->myStat.missL1++;	// Miss in L1
			this->myStat.missVic++; // Miss in Victim Cache
			this->myStat.accL2++;
			cout << "Block exists in L2" << endl;

			// Take L2 cache block and store it in L1
			// Need to move block into L1
			if (!L1[adrInfo.index].valid) // If empty available block in L1
			{
				L1[adrInfo.index].data = *data;
				L1[adrInfo.index].lru_position = 0;
				L1[adrInfo.index].tag = adrInfo.tag;
				L1[adrInfo.index].valid = true;
				// TODO add block of MEM to L1 cache

				// TODO invalidate old block in L2 cache
				L2[adrInfo.index][adrInfo.L2Pos].valid = false;
				// Update L2 LRU
				for (int i = 0; i < 8; i++)
				{
					if (L2[adrInfo.index][i].valid && L2[adrInfo.index][i].lru_position < L2[adrInfo.index][adrInfo.L2Pos].lru_position)
						L2[adrInfo.index][i].lru_position++;
				}
			}
			else
			{
				tempCacheBlock = L1[adrInfo.index];
				L1[adrInfo.index].data = *data;
				L1[adrInfo.index].lru_position = 0;
				L1[adrInfo.index].tag = adrInfo.tag;
				L1[adrInfo.index].valid = true;
				L1[adrInfo.index].lru_position = 0; // No need for lru_position in L1 cache
				// TODO add block of MEM to L1 cache
				tempCacheBlock.lru_position = VICTIM_SIZE - 1;

				// add tempCacheBlock to VC
				if (victimFull())
				{
					// tempCacheBlock2 = findEvictVictim();
					vcIndex = findEvictVictimIndex();
					tempCacheBlock2 = VC[vcIndex];
					VC[vcIndex] = tempCacheBlock;
					VC[vcIndex].victimTag = tempCacheBlock.tag + adrInfo.index;
					for (int i = 0; i < 4; i++) // Reduces LRU position by 1
					{
						if (i != vcIndex)
							VC[i].lru_position--;
					}

					// TODO add tempCacheBlock2 to L2
					tempCacheBlock2.lru_position = 7;
					for (int i = 0; i < 8; i++)
					{
						if (L2[adrInfo.index][i].valid && i != adrInfo.L2Pos && L2[adrInfo.index][i].lru_position > L2[adrInfo.index][adrInfo.L2Pos].lru_position)
							L2[adrInfo.index][i].lru_position--;
					}
					L2[adrInfo.index][adrInfo.L2Pos] = tempCacheBlock2;
				}
				else // If victim cache is not full (DONT THINK THIS IS POSSIBLE)
				{
					for (int i = 0; i < 3; i++)
					{
						if (!VC[i].valid)
							vcIndex = i;
					}
					VC[vcIndex] = tempCacheBlock;
					VC[vcIndex].victimTag = tempCacheBlock.tag + adrInfo.index;
					for (int i = 0; i < 4; i++) // Reduces LRU position by 1
					{
						if (i != vcIndex)
							VC[i].lru_position--;
					}

					L2[adrInfo.index][adrInfo.L2Pos].valid = false;
					for (int i = 0; i < 8; i++)
					{
						if (L2[adrInfo.index][i].valid && L2[adrInfo.index][i].lru_position < L2[adrInfo.index][adrInfo.L2Pos].lru_position)
							L2[adrInfo.index][i].lru_position++;
					}
				}

				// adrInfo has L2 position
			}
		}
		else
		{
			cout << "Block only exists in Main Memory" << endl;
			this->myStat.missL1++;
			this->myStat.missVic++;
			this->myStat.missL2++;

			// Take Main Memory block and store it in L1
			// Need to move block into L1
			if (!L1[adrInfo.index].valid) // If empty available block in L1
			{
				L1[adrInfo.index].data = *data;
				L1[adrInfo.index].lru_position = 0;
				L1[adrInfo.index].tag = adrInfo.tag;
				L1[adrInfo.index].valid = true;
				// TODO add block of MEM to L1 cache
			}
			else
			{
				tempCacheBlock = L1[adrInfo.index];
				L1[adrInfo.index].data = *data;
				L1[adrInfo.index].lru_position = 0;
				L1[adrInfo.index].tag = adrInfo.tag;
				L1[adrInfo.index].valid = true;
				L1[adrInfo.index].lru_position = 0; // No need for lru_position in L1 cache
				// TODO add block of MEM to L1 cache

				tempCacheBlock.lru_position = VICTIM_SIZE - 1;

				// add tempCacheBlock to VC
				if (victimFull())
				{
					// tempCacheBlock2 = findEvictVictim();
					vcIndex = findEvictVictimIndex();
					tempCacheBlock2 = VC[vcIndex];
					VC[vcIndex] = tempCacheBlock;
					VC[vcIndex].victimTag = tempCacheBlock.tag + adrInfo.index;
					for (int i = 0; i < 4; i++) // Reduces LRU position by 1
					{
						if (i != vcIndex)
							VC[i].lru_position--;
					}

					if (L2Full(adrInfo.index))
					{
						L2index = findEvictL2Index(adrInfo.index);
						tempCacheBlock2.lru_position = 7;
						L2[adrInfo.index][L2index] = tempCacheBlock2;
						for (int i = 0; i < 8; i++)
						{
							if (i != L2index)
								L2[adrInfo.index][i].lru_position--;
						}
					}
					else // If L2 is not full
					{
						// TODO add tempCacheBlock2 to L2
						for (int i = 0; i < 8; i++)
						{
							if (!L2[adrInfo.index][i].valid)
								L2index = i;
						}

						tempCacheBlock2.lru_position = 7;
						for (int i = 0; i < 8; i++)
						{
							if (L2[adrInfo.index][i].valid && i != L2index) // && L2[adrInfo.index][i].lru_position > L2[adrInfo.index][adrInfo.L2Pos].lru_position)
								L2[adrInfo.index][i].lru_position--;
						}
						L2[adrInfo.index][L2index] = tempCacheBlock2;
					}
				}
				else // If victim cache is not full
				{
					for (int i = 0; i < 4; i++)
					{
						if (!VC[i].valid)
							vcIndex = i;
					}
					tempCacheBlock.lru_position = 3;
					VC[vcIndex] = tempCacheBlock;
					VC[vcIndex].victimTag = tempCacheBlock.tag + adrInfo.index;
					for (int i = 0; i < 4; i++) // Reduces LRU position by 1
					{
						if (i != vcIndex && VC[i].valid)
							VC[i].lru_position--;
					}
				}

				// adrInfo has L2 position
			}
		}
	}
	else if (MemW)
	{
		// cout << "STORE" << endl;
		if (containsL1(adrInfo)) // Check to see if data exists in L1 Direct Map
		{
			updateDataL1(adrInfo, data);
		}
		else if (containsVC(&adrInfo)) // Check to see if data exists in Victim Cache
		{
			updateVC(adrInfo, data);
		}
		else if (containsL2(&adrInfo)) // Check to see if data exists in L2
		{
			updateL2(adrInfo, data);
		}

		myMem[adr] = *data; // Update data in Main Memory
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
	bitset<30> victimTag;
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

	for (int i = 2; i < 32; i++)
	{
		victimTag[i - 2] = adr[i];
	}

	currBlk.address = adr.to_ulong();
	currBlk.index = index.to_ulong();
	currBlk.offset = offset.to_ulong();
	currBlk.tag = tag.to_ulong();
	currBlk.victimTag = victimTag.to_ulong();
	currBlk.victimPos = 0;
	currBlk.L2Pos = 0;

	return currBlk;
}

// Sets addresses to a block of memory (4 bytes, each memory index is 1 byte)
void cache::setBlock(int *block, int adr)
{
	int set = adr * 4;
	int temp = set % 4;
	temp = set - temp;

	for (int i = 0; i < 4; i++)
	{
		block[i] = temp + i;
		// cout << block[i] << endl;
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

// Checks if block exists in Victim Cache
bool cache::containsVC(addressInfo *adrInfo)
{
	for (int i = 0; i < VICTIM_SIZE; i++)
	{
		if (VC[i].valid && VC[i].victimTag == adrInfo->victimTag)
		{
			adrInfo->victimPos = i;
			return true;
		}
	}

	return false;
}

// Updates value and LRU position in Victim Cache
bool cache::updateVC(addressInfo adrInfo, int *data)
{
	VC[adrInfo.victimPos].data = *data;

	for (int i = 0; i < 4; i++) // Reduces LRU position by 1 for all positions above current position
	{
		if (VC[i].valid && i != adrInfo.victimPos && VC[i].lru_position > VC[adrInfo.victimPos].lru_position) // VC might not be full
			VC[i].lru_position--;
	}
	VC[adrInfo.victimPos].lru_position = VICTIM_SIZE - 1; // Updates LRU position of current block to Most Recently Used

	return true;
}

// Checks if block exists in L2
bool cache::containsL2(addressInfo *adrInfo)
{
	for (int i = 0; i < L2_CACHE_WAYS; i++)
	{
		if (L2[adrInfo->index][i].valid && L2[adrInfo->index][i].tag == adrInfo->tag)
		{
			adrInfo->L2Pos = i;
			return true;
		}
	}

	return false;
}

// Updates value and LRU position in L2
bool cache::updateL2(addressInfo adrInfo, int *data)
{
	L2[adrInfo.index][adrInfo.L2Pos].data = *data;

	for (int i = L2_CACHE_WAYS - 1; i > L2[adrInfo.index][adrInfo.L2Pos].lru_position; i--)
	{
		if (L2[adrInfo.index][i].valid && i != adrInfo.L2Pos && L2[adrInfo.index][i].lru_position > L2[adrInfo.index][adrInfo.L2Pos].lru_position) // L2 might not be full
			L2[adrInfo.index][i].lru_position--;
	}
	L2[adrInfo.index][adrInfo.L2Pos].lru_position = L2_CACHE_WAYS - 1;

	return true;
}

// Checks if Victim Cache is full
bool cache::victimFull()
{
	for (int i = 0; i < 4; i++)
	{
		if (!VC[i].valid)
		{
			return false;
		}
	}

	return true;
}

int cache::findEvictVictimIndex()
{
	for (int i = 0; i < 4; i++)
	{
		if (VC[i].lru_position == 0)
			return i;
	}

	return -1;
}

bool cache::L2Full(int index)
{
	for (int i = 0; i < 8; i++)
	{
		if (!L2[index][i].valid)
			return false;
	}

	return true;
}

int cache::findEvictL2Index(int index)
{
	for (int i = 0; i < 8; i++)
	{
		if (L2[index][i].lru_position == 0)
			return i;
	}

	return -1;
}

// Calculate L1 miss rate
float cache::calcL1MissRate()
{
	float res;
	// cout << myStat.missL1 << endl;
	// cout << myStat.accL1 << endl;
	res = ((static_cast<float>(myStat.missL1)) / (myStat.missL1 + myStat.accL1));
	// cout << res << endl;
	return res;
}

// Calculate L2 miss rate
float cache::calcL2MissRate()
{
	float res;
	res = ((static_cast<float>(myStat.missL2)) / (myStat.missL2 + myStat.accL2));
	return res;
}

// Calculate VC miss rate
float cache::calcVCMissRate()
{
	float res;
	res = ((static_cast<float>(myStat.missVic)) / (myStat.missVic + myStat.accVic));
	return res;
}

// Updates data in MainMemory
// bool cache::updateMainMem(int *data, int *myMem) // Updates data in Main Memory
// {
// 	for (int i = 0; i < 4; i++)
// 	{
// 		myMem[block[i]] = *data;
// 	}

// 	cout << myMem[block[0]] << endl;

// 	return true;
// }