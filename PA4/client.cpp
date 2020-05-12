#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <thread>
using namespace std;

FIFORequestChannel* create_new_channel(FIFORequestChannel* mainchan){
  char name[1024];
  MESSAGE_TYPE m = NEWCHANNEL_MSG;
  mainchan->cwrite(&m, sizeof(m));
  mainchan->cread(name, 1024);
  FIFORequestChannel* newchan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
  return newchan;
}
void patient_thread_function(int n, int pno, BoundedBuffer* request_buffer){
    /* What will the patient threads do?
    parse command line arguments, create patient threads that push to request buffer
      1 request per data point
    */
    datamsg d(pno, 0.0, 1);
    double resp = 0;
    for(int i = 0; i < n; i++){
      /*chan->cwrite(&d, sizeof(datamsg));
      chan->cread(&resp, sizeof(double));
      hc->update(pno, resp);*/
      request_buffer->push((char*) &d, sizeof(datamsg));
      d.seconds += 0.004;
    }
}

void worker_thread_function(FIFORequestChannel* chan, BoundedBuffer* request_buffer, HistogramCollection* hc, int mb){
    /*
		Functionality of the worker threads
    pop an instruction from the request buffer and make a request to the server
    push data to the BoundedBuffer, wait if full
    */
    char buf [1024];
    double resp = 0;

    char recvbuf[mb];
    while(true){
      request_buffer->pop(buf, 1024);
      MESSAGE_TYPE* m = (MESSAGE_TYPE*) buf;
      double resp = 0;
      if(*m == DATA_MSG){
        chan->cwrite(buf, sizeof(datamsg));
        chan->cread(&resp, sizeof(double));
        hc->update(((datamsg*)buf)->person, resp);
      }
      else if( *m == FILE_MSG){
        filemsg*  fm = (filemsg*) buf;
        string fname = (char*)(fm+1);
        int sz = sizeof(filemsg) + fname.size() + 1;
        chan->cwrite(buf, sz);
        chan->cread(recvbuf, mb);

        string recvfname = "recv/" + fname;

        FILE* fp = fopen(recvfname.c_str(), "r+");
        fseek(fp,fm->offset , SEEK_SET);
        fwrite(recvbuf, 1, fm->length, fp);
        fclose(fp);

      }
      else if(*m == QUIT_MSG){
        chan->cwrite(m, sizeof(MESSAGE_TYPE));
        delete chan;
        break;
      }
    }
}

void file_thread_function(string fname, BoundedBuffer* request_buffer, FIFORequestChannel* chan, int mb){
  //1. create the file
  string recvfname = "recv/" + fname;
  //make it as long as the original length
  char buf[1024];
  filemsg f(0,0);
  memcpy(buf,&f,sizeof(f));
  strcpy(buf + sizeof(f), fname.c_str());
  chan->cwrite(buf, sizeof(f) + fname.size() + 1);
  __int64_t filelength;
  chan->cread(&filelength, sizeof(filelength));

  FILE* fp = fopen(recvfname.c_str(), "w");
  fseek(fp, filelength, SEEK_SET);
  fclose(fp);
  //2. generate all the file messages
  filemsg* fm = (filemsg*) buf;
  __int64_t remlen = filelength;

  while(remlen > 0){
    fm->length = min(remlen, (__int64_t) mb);
    request_buffer->push(buf, sizeof(filemsg) + fname.size() + 1);
    fm->offset += fm->length;
    remlen -= fm->length;
  }

}

int main(int argc, char *argv[])
{
    string fname = "10.csv";
    int n = 15000;    //default number of requests per "patient"
    int p = 10;     // number of patients [1,15]
    int w = 20;    //default number of worker threads
    int b = 20; 	// default capacity of the request buffer, you should change this default
	  int m = MAX_MESSAGE; 	// default capacity of the message buffer
    bool fileCommand = false;
    srand(time_t(NULL));

    //command line arguments
    int arg;
    while((arg = getopt(argc, argv, "n:p:w:b:m:f:")) != -1){
      switch(arg){
        case 'n':
          n = atoi(optarg);
          break;
        case 'p':
          p = atoi(optarg);
          break;
        case 'w':
          w = atoi(optarg);
          break;
        case 'b':
          b = atoi(optarg);
          break;
        case 'm':
          m = atoi(optarg);
          break;
        case 'f':
          fname = optarg;
          fileCommand = true;
          break;
      }
    }

    int pid = fork();
    if (pid == 0){
		// pass message buffer capacity to server
        string mChar = to_string(m);
        string messageFlag = "-m " + mChar;
        execl ("server", "server", messageFlag.c_str() , (char *)NULL);
    }

	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;

  // making histograms, adding to histogram collection
  for(int i = 0; i < p; i++){
    Histogram* h = new Histogram(10, -2.0, 2.0);
    hc.add(h);
  }

  // make worker channels
  FIFORequestChannel* wchans[w];
  for(int i = 0; i< w; i++){
    wchans[i] = create_new_channel(chan);
  }
    struct timeval start, end;
    gettimeofday (&start, 0);

    /* Start all threads here */
    if(fileCommand == false){
      thread patient [p];
      for(int i = 0; i< p; i++){
        patient[i] = thread(patient_thread_function, n, i + 1, &request_buffer);
      }

      thread workers[w];
      for(int i = 0; i < w; i++){
        workers[i] = thread(worker_thread_function, wchans[i], &request_buffer, &hc, m);
      }

      for(int i = 0; i< p; i++){
        patient[i].join();
      }

      cout << "Patient threads/file thread finished" << endl;
      //push quit messages to kill workers
      for(int i = 0; i< w; i++){
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char*) &q, sizeof(q));
      }

      for(int i = 0; i< w; i++){
        workers[i].join();
      }
      cout << "Worker threads finished" << endl;
        gettimeofday (&end, 0);
    }
    else{
      thread filethread (file_thread_function, fname, &request_buffer, chan,m);

      thread workers[w];
      for(int i = 0; i < w; i++){
        workers[i] = thread(worker_thread_function, wchans[i], &request_buffer, &hc, m);
      }

      filethread.join();

      cout << "Patient threads/file thread finished" << endl;
      //push quit messages to kill workers
      for(int i = 0; i< w; i++){
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char*) &q, sizeof(q));
      }

      for(int i = 0; i< w; i++){
        workers[i].join();
      }
      cout << "Worker threads finished" << endl;
        gettimeofday (&end, 0);

    }
    // print the results
	   hc.print ();

    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;
    //clean up main channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;

}
