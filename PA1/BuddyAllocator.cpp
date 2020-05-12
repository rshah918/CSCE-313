#include "BuddyAllocator.h"
#include <iostream>
#include <cmath>
using namespace std;

BlockHeader::BlockHeader(){
	this->isfree = 1;
	this->block_size = 0;
	this->next = NULL;
}
LinkedList::LinkedList(){
	head = NULL;
}

void LinkedList::insert (BlockHeader* b){//push memory Block onto a LinkedList
		if(head == NULL){
			head = b;
      b->next = NULL;
		}
		else{
			BlockHeader* temp = head->next;
			head->next = b;
			b->next = temp;
		}
}

void LinkedList::remove (BlockHeader* b){
		BlockHeader* curr = head;
		if(head == b){
			head = head->next;
			b->next = NULL;
			return;
		}
		while(curr->next != NULL){//find the node before the target node
      if(curr->next == b){
        curr->next = curr->next->next;
      }
      else{
        curr = curr->next;
      }
		}
		b->next = NULL;
	}

int LinkedList::get_size(){
	BlockHeader* curr = head;
	int count = 0;
	while(curr){
		curr = curr->next;
		count++;
	}
	return count;
}

BuddyAllocator::BuddyAllocator (int basic_block_size, int total_memory_length){
  this->total_memory_size = powersofTwo(total_memory_length);
  this->basic_block_size = basic_block_size;
	void* allocated_memory = malloc(powersofTwo(total_memory_size));
  this->starting_address = (char*)allocated_memory;
	//initialize FreeList size
  FreeListLength = 1;
  int i = basic_block_size;
  while(i < total_memory_size){
    i = i * 2;
    FreeListLength++;
  }
  FreeList.resize(FreeListLength);
  //instantiate LinkedLists in each index of FreeList
  for(int i = 0; i < FreeListLength; i++){
    LinkedList newLL = LinkedList();
    FreeList[i] = newLL;
  }
  //add a free block with the size of total_memory_size in the begining of FreeList
  BlockHeader* newBlock = (BlockHeader*) allocated_memory;

	newBlock->next = NULL;
  newBlock->isfree = 1;
  newBlock->block_size = basic_block_size*pow(2,FreeListLength-1);
  FreeList[FreeListLength-1].insert(newBlock);
}

BuddyAllocator::~BuddyAllocator (){
	delete starting_address;
}

BlockHeader* BuddyAllocator::getbuddy(BlockHeader* addr){
  // given a block address, this function returns the address of its buddy
  //Find buddy address: do XOR with address and Block Size
  //(blockaddress-start) will cast to an int. make sure its a char pointer or else it will go wrong
	int offset = (int)((char *) addr - starting_address);
	int buddy_offset = offset^addr->block_size;
	BlockHeader* potentialBuddy = (BlockHeader*) (starting_address + buddy_offset);
	bool isbuddy = arebuddies(addr, potentialBuddy);// call arebuddies() to verify that its actually your buddy
	if(isbuddy){
    return potentialBuddy;
  }
  else{
    return NULL;
  }
}

bool BuddyAllocator::arebuddies(BlockHeader* block1, BlockHeader* block2){
  // checks whether the two blocks are buddies are not
	//verify that buddy is the same size, and is free.
	bool sameSize = false;
	bool blocksFree = false;
	if(block1-> block_size == block2->block_size){
		sameSize = true;
	}
	if(block2->isfree == 1){
		blocksFree = true;
	}
	if(sameSize && blocksFree){
			return true;
	}
	else{
		return false;
	}
}

BlockHeader* BuddyAllocator::merge(BlockHeader* block1, BlockHeader* block2){
  // this function merges the two blocks

  BlockHeader* leftBlock;
  BlockHeader* rightBlock;
  if(block1 > block2){//find the leftmost block
    leftBlock = block2;
    rightBlock = block1;
  }
  else{
    leftBlock = block1;
    rightBlock = block2;
  }
    //delete the 2 buddy blocks from their respective LL
    int indx = 0;
    int i = basic_block_size;
    //find the index of the correct LL in FreeList
    while(i != leftBlock->block_size && i <= total_memory_size){
      i = i*2;
      indx++;
    }
    FreeList.at(indx).remove(rightBlock);//remove buddy blocks
    FreeList.at(indx).remove(leftBlock);
  //merge blocks
  leftBlock->block_size = (leftBlock->block_size) * 2;
  leftBlock->isfree = 1;
  //add merged block to the new LL
  indx = getIndex(leftBlock->block_size);
  FreeList.at(indx).insert(leftBlock);
  return leftBlock;
}

BlockHeader* BuddyAllocator::split (BlockHeader* block){
  // splits the given block by putting a new header halfway through the block
  int offset = ((block->block_size)/2);
	char* newaddr = (char*)block + offset;
  BlockHeader* newBlock = (BlockHeader*)newaddr;
	newBlock->next = NULL;
  //delete the original block from its current LL
	int indx = getIndex(block->block_size);
  FreeList.at(indx).remove(block);
  //adjust both blocks;
  block->block_size = block->block_size/2;
  newBlock->block_size = block->block_size;
  block->next = NULL;
  newBlock->next = NULL;
	block->isfree = 1;
	newBlock->isfree = 1;
  //add the 2 new blocks to its respective LL's
  indx = getIndex(block->block_size);

  FreeList.at(indx).insert(block);//insert block at correct index
  FreeList.at(indx).insert(newBlock);
  return block;
}

BlockHeader* BuddyAllocator::alloc(int length){
	length = length + sizeof(BlockHeader);//add size of blockheader
	if(!checkRequest(length)){//validate request
		return NULL;
	}
	int indx = getIndex(powersofTwo(max(length, basic_block_size)));//index of target size
	//get the next free memblock that we need to split down
	int block_to_split = indx;
	if(!LL_empty(FreeList.at(block_to_split))){//If LL has no free blocks
		//keep moving to a larger LL until one of them has a free memblock
		while(block_to_split < FreeListLength-1){
			block_to_split++;
			if(LL_empty(FreeList.at(block_to_split))){
				break;
			}
		}
	}
	if(!LL_empty(FreeList.at(block_to_split))){
		cout << "Not enough contigious free memory, sorry!" << endl;
		return NULL;
	}
	//find a free memblock to be split 
	BlockHeader* split_block = FreeList.at(block_to_split).head;
	while(split_block != NULL){
		if(split_block->isfree == 1){
			break;
		}
		split_block = split_block->next;
	}
	//split block until its size matches target
	while(split_block->block_size != (basic_block_size*pow(2,indx))){
		split_block = split(split_block);
	}

  split_block->isfree = 0;//allocated
  return split_block + 1;
}

void BuddyAllocator::free(void* a) {
	BlockHeader* block_to_free = (((BlockHeader*)a)-1); //subtract 16 bytes to get blockheader
	block_to_free->isfree = 1;//mark the block as free;
	BlockHeader* buddy;
	int index = getIndex(block_to_free->block_size);
	while(true){
		buddy = getbuddy(block_to_free); //find the address of buddy block
		if(buddy){
			block_to_free = merge(block_to_free, buddy);//merge both blocks if possible
			block_to_free->isfree = 1;
		}
		else{
			break;
		}
	}
}

int BuddyAllocator::getIndex(int size){
	int index = 0;
  int i = basic_block_size;
  //find the index of the correct LL in FreeList
  while(i != size && i < total_memory_size){
    i = i*2;
    index++;
  }
	return index;
}

bool BuddyAllocator::LL_empty(LinkedList ll){
	//Finds if the LinkedList has an unallocated memory block
	BlockHeader* curr = ll.head;
	bool isEmpty = false;
	if(ll.head == NULL){
		return false;
	}
	while(curr){
		if(curr->isfree == 1){
			isEmpty = true;
			break;
		}
		else{
			curr = curr->next;
		}
	}
	return isEmpty;

}

bool BuddyAllocator::checkRequest(int length){
	bool validRequest = true;
	int total_free_memory = 0;
	for (int i = 0; i < FreeList.size(); i++){//get total amount of free memory
		BlockHeader* b = FreeList[i].head;
		while (b){
			if(b->isfree == 1){
				total_free_memory += b->block_size;
			}
			b = b->next;
		}
	}
	if(length > total_free_memory){
		cout << "Out of memory! Oops!" << endl;
		validRequest = false;
	}
	if(length > total_memory_size){//edge case
		cout << "You are asking for too much memory! Very greedy.";
		validRequest = false;
	}
	return validRequest;
}

int BuddyAllocator::powersofTwo(int x){
	//get the closest larger power of 2 ->Got this technique from StackOverflow
	x = x - 1;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

void BuddyAllocator::printlist (){
  cout << "Printing the Freelist in the format \"[index] (block size) : # of blocks\"" << endl;
  int64_t total_free_memory = 0;

  for (int i = 0; i < FreeList.size(); i++){
    int blocksize = ((1<<i) * basic_block_size); // all blocks at this level are this size
    cout << "[" << i <<"] (" << blocksize << ") " <<": ";  // block size at index should always be 2^i * bbs
    int count = 0;
    BlockHeader* b = FreeList [i].head;

    // go through the list from head to tail and count
    while (b){
			if(b->isfree == 1){
      	total_free_memory += blocksize;
				count ++;
			}
      // block size at index should always be 2^i * bbs
      // checking to make sure that the block is not out of place
      if (b->block_size != blocksize){
        cerr << "ERROR:: Block is in a wrong list" << endl;
        cout << "EXPECTED SIZE: " << blocksize << endl;
        cout << "ACTUAL SIZE: " << b->block_size << endl;
        //exit (-1);
      }
      b = b->next;
    }
    cout << count << endl;
    cout << "Amount of available free memory: " << total_free_memory << " bytes" << endl;
  }
}
