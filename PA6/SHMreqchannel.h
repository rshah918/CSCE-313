#ifndef _SHMreqchannel_H_
#define _SHMreqchannel_H_

#include "common.h"
#include <sys/mman.h>
#include <semaphore.h>
#include "Reqchannel.h"
#include <fcntl.h>
#include <string>

class SMBB{
private:
	/*  The current implementation uses named pipes. */
	string name;
	sem_t* producerdone;
  sem_t* consumerdone;
  int shmfd;
  char* data;
  int buffersize;

public:

	SMBB(string _name, int _bufsz):buffersize(_bufsz), name(_name){
    //create the shared memory object
    shmfd = shm_open(name.c_str(), O_RDWR|O_CREAT, 0644);
    if(shmfd < 0){
      perror("SHM create error");
      exit(0);
    }
    //initialize the size of the shared memory object
    ftruncate(shmfd, buffersize);
    //map the new slab of shared memory to this process's virtual address space
    data = (char*) mmap(NULL, buffersize, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0);
    if(!data){
      perror("Map Error");
      exit(0);
    }
    //we want multiple processes to access the same semaphores
    //a Named semaphore is needed
    producerdone = sem_open((name + "1").c_str(), O_CREAT, 0644, 0);
    consumerdone = sem_open((name + "2").c_str(), O_CREAT, 0644, 1);//init to 1 to allow consumer to run first
  }


	~SMBB(){
    //deallocate shared memory and the semaphores
    munmap(data, buffersize);//remove the mapping from the page table
    close(shmfd);//close the file descriptor
    shm_unlink(name.c_str());//destroy the hared memory segment
    sem_close(producerdone);//close the named semaphore's reference
    sem_close(consumerdone);//close the named semaphore's reference
    sem_unlink((name + "1").c_str());//destroy the named semaphore
    sem_unlink((name + "2").c_str());//destroy the named semaphore
  }

	int push(char* msg, int len){
    sem_wait(consumerdone);
    memcpy(data, msg, len);
    sem_post(producerdone);
    return len;
  }

	int pop(char* msg, int len){
    sem_wait(producerdone);
    memcpy(msg, data, len);
    sem_post(consumerdone);
    return len;
  }
};

class SHMRequestChannel: public RequestChannel{
private:
  SMBB* b1;
  SMBB* b2;
  string s1, s2;
  int buffersize;
public:
  SHMRequestChannel(const string _name, const Side _side, int _bufs);

  ~SHMRequestChannel();

  int cread(void* buf, int len);

  int cwrite(void* buf, int len);
};

#endif
