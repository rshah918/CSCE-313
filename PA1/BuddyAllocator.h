#ifndef _BuddyAllocator_h_                   // include file only once
#define _BuddyAllocator_h_
#include <iostream>
#include <vector>
using namespace std;
typedef unsigned int uint;

class BlockHeader{//This class represents a single memory block of a certain size
public:
	int isfree; //1 if block is free, 0 if not
	int block_size;  // size of the block
	BlockHeader* next; // pointer to the next block

	BlockHeader();
};

class LinkedList{
	// this is a special linked list that is made out of BlockHeader s.
public:
	BlockHeader* head;
	//treat like a stack, push new nodes to the head
	LinkedList();
	void insert (BlockHeader* b);

	void remove (BlockHeader* b);

	int get_size();
};

class BuddyAllocator{
private:
	vector<LinkedList> FreeList;
	int FreeListLength;
	int basic_block_size;
	int total_memory_size;
	char* starting_address;

public:
	BlockHeader* getbuddy (BlockHeader * addr);

	bool arebuddies (BlockHeader* block1, BlockHeader* block2);

	BlockHeader* merge (BlockHeader* block1, BlockHeader* block2);

	BlockHeader* split (BlockHeader* block);

	int getIndex(int size);

	bool checkRequest(int length);

	bool LL_empty(LinkedList ll);

	int powersofTwo(int size);

public:
	BuddyAllocator (int basic_block_size = 128, int total_memory_size = 512);

	~BuddyAllocator();


	BlockHeader* alloc(int length);

	void free(void* _a);
	/* Frees the section of physical memory previously allocated
	   using alloc(). */

	void printlist ();
	/* Mainly used for debugging purposes and running short test cases */
	/* This function prints how many free blocks of each size belong to the allocator
	at that point. It also prints the total amount of free memory available just by summing
	up all these blocks.
	Aassuming basic block size = 128 bytes):

	[0] (128): 5
	[1] (256): 0
	[2] (512): 3
	[3] (1024): 0
	....
	....
	 which means that at this point, the allocator has 5 128 byte blocks, 3 512 byte blocks and so on.*/
};

#endif
