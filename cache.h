#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
	int tag;		  // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	int data;		  // the actual data stored in the cache/memory
	bool valid;
	// add more things here if needed
	int block[4];
	int victimTag;
};

struct Stat
{
	int missL1;
	int missL2;
	int accL1;
	int accL2;
	int accVic;
	int missVic;
	// add more stat if needed. Don't forget to initialize!
};

struct addressInfo
{
	int index;
	int offset;
	int tag;
	int address;
	int victimPos;
	int L2Pos;
	int victimTag;
};

class cache
{
private:
	cacheBlock L1[L1_CACHE_SETS];				 // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row
	// Add your Victim cache here ...
	cacheBlock VC[VICTIM_SIZE];

	Stat myStat;
	// add more things here
	bool vcFull;

public:
	cache();
	void controller(bool MemR, bool MemW, int *data, int adr, int *myMem);
	// add more functions here ...
	addressInfo decode(bitset<32> adr);				   // decode into index, offset, and tag
	void setBlock(int *block, int adr);				   // sets indices for full block
	bool containsL1(addressInfo adrInfo);			   // checks if block exists in L1
	bool updateDataL1(addressInfo adrInfo, int *data); // updates data in L1
	bool containsVC(addressInfo *adrInfo);			   // checks if block exists in Victim Cache
	bool updateVC(addressInfo adrInfo, int *data);	   // updates data in Victim Cache
	bool containsL2(addressInfo *adrInfo);			   // checks if block exists in L2
	bool updateL2(addressInfo adrInfo, int *data);	   // updates data in L2
	bool victimFull();
	int findEvictVictimIndex();
	bool L2Full(int index);
	int findEvictL2Index(int index);

	// bool updateMainMem(int *data, int *myMem); // updates data in Main Memory
};
