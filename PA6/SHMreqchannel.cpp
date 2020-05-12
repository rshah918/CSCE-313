#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "common.h"
#include "SHMreqchannel.h"
#include <string>
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   S H M R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

SHMRequestChannel::SHMRequestChannel(const string _name, const Side _side, int _bufs):RequestChannel(_name, _side), buffersize(_bufs){
  s1 = "/bb_" + _name + "1";
  s2 = "/bb_" + _name + "2";

  if(_side == SERVER_SIDE){
    b1 = new SMBB(s1, buffersize);
    b2 = new SMBB(s2, buffersize);
  }
  else{
    b1 = new SMBB(s2, buffersize);
    b2 = new SMBB(s1, buffersize);
  }
}

SHMRequestChannel::~SHMRequestChannel(){
  delete b1;
  delete b2;
}

int SHMRequestChannel::cread(void* buf, int len){
	return b1->pop((char*) buf, len);
}

int SHMRequestChannel::cwrite(void* buf, int len){
	return b2->push((char*) buf, len);
}
