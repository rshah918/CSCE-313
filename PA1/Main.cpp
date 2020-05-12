#include "Ackerman.h"
#include "BuddyAllocator.h"
#include <unistd.h>

void easytest(BuddyAllocator* ba){
  // be creative here
  // know what to expect after every allocation/deallocation cycle
  // here are a few examples
  // allocating a byte
  char* mem = (char *) ba->alloc(2007);
  ba->printlist();
  // now print again, how should the list look now
  ba->free(mem);// give back the memory you just allocated
  char* mem2 = (char *) ba->alloc(78643);
  //*mem2 = '2';
  ba->free(mem2);
  ba->printlist(); // shouldn't the list now look like as in the beginning
}

int main(int argc, char ** argv) {
  int arg;
  int b = 128;
  int s = 512*1024*128;

  int d;
  int e;
  s = 64000000;
  while((arg = getopt(argc, argv, "b:s:")) != -1){
    switch(arg){
      case 'b':
        b = atoi(optarg);

      case 's':
        s = atoi(optarg);
    }
  }
  int basic_block_size = b, memory_length = s;
  cout << b << " " << s << endl;
  // create memory manager
  BuddyAllocator * allocator = new BuddyAllocator(basic_block_size, memory_length);
  //easytest(allocator);

  // stress-test the memory manager, do this only after you are done with small test cases
  Ackerman* am = new Ackerman ();
  am->test(allocator); // this is the full-fledged test.

  // destroy memory manager
  delete allocator;
}
