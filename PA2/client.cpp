/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <fstream>
#include <string>

using namespace std;

void singleDataPoint(int person, double time, int ecg,FIFORequestChannel* chan){
  double timestamp = 0.000;
  timeval tim;
  double t1;
  double t2;
  gettimeofday(&tim, NULL);

  datamsg msg(person,time,ecg);
  double data;
  chan->cwrite ((void*)&msg, sizeof(msg)); //sending the message through the channel
  chan->cread((char*) &data, sizeof(double));//read the response, stored in "data"
  cout << "Buffer: " << data << endl;
  t2 = tim.tv_usec;
  cout << "Time Elapsed: "<< t2-t1 << endl;
}

void allDataPoints(FIFORequestChannel* chan){
  //requests an entire file 1 data point at a time
  fstream fout;
  fout.open("./received/x1.csv", ofstream::out);//open outfile

  double timestamp = 0.000;
  int column1 = 0;
  int column2 = 1;
  int column3 = 2;
  timeval tim;
  double t1;
  double t2;
  gettimeofday(&tim, NULL);
  t1 = tim.tv_usec;

  for(int i = 0; i < 15000; i++){
    double data1;
    double data2;
    double data3;

    //get first element in row with current timestamp
    data1 = timestamp;
    //get second element in row with current timestamp
    datamsg msg1(1,timestamp,column2);
    chan->cwrite((void*)&msg1, sizeof(msg1));
    chan->cread((char*) &data2, sizeof(double));
    //get third element in row with current timestamp
    datamsg msg2(1,timestamp,column3);
    chan->cwrite((void*)&msg2, sizeof(msg2));
    chan->cread((char*) &data3, sizeof(double));
    fout << setprecision(3) << fixed << data1 << ',' << data2 << ',' << data3 << "\n";
    timestamp = timestamp + 0.004;
  }
  gettimeofday(&tim, NULL);
  t2 = tim.tv_usec;
  cout << "Time Elapsed: "<< t2-t1 << endl;
}

__int64_t getFileSize(FIFORequestChannel* chan){
  filemsg msg = filemsg(0,0);
  char name[] = {'1','.','c','s','v'};
  //char name[] = {'1','0','0','.','d','a','t'};
    //char name[] = {'2','5','6','.','d','a','t'};
      //char name[] = {'1','0','0','0','.','d','a','t'};
  char* buf = new char[sizeof(filemsg) + sizeof(name) + 1];
  *(filemsg*)buf = msg;
  for(int i = 0; i < 8; i++){
    *(char*)(buf + sizeof(filemsg) + i) = name[i];
  }
  *(char*)(buf + sizeof(filemsg)+sizeof(name)) = '\0';
  chan->cwrite(buf, sizeof(filemsg) + sizeof(name) + 1);
  __int64_t file_size;
  chan->cread((char*)&file_size, sizeof(__int64_t));//write server response
  return file_size;
}

void fileRequest(string fileName, FIFORequestChannel* chan){
      double timestamp = 0.000;
      timeval tim;
      double t1;
      double t2;
      gettimeofday(&tim, NULL);

      char name[fileName.size()];
      //populate filename array from input string
      for(int i = 0; i < fileName.size(); i++){
        name[i] = fileName[i];
      }

      for(int i = 0; i < sizeof(name); i++){
        cout << name[i] << endl;
      }

      filemsg msg = filemsg(0,0);
      //send file request to get filesize
      char* buf = new char[sizeof(filemsg) + sizeof(name) + 1];
      *(filemsg*)buf = msg;
      //populate buffer
      for(int i = 0; i < sizeof(name); i++){
        *(char*)(buf + sizeof(filemsg) + i) = name[i];
      }
      *(char*)(buf + sizeof(filemsg)+sizeof(name)) = '\0';//null terminate buffer
      chan->cwrite(buf, sizeof(filemsg) + sizeof(name) + 1);//send server request
      __int64_t file_size;
      chan->cread((char*)&file_size, sizeof(__int64_t));//read file size

      //open output file
      ofstream outfile;
      outfile.open("received/1.csv");
      //calculate number of loop iterations
      __int64_t regularsize = (file_size/MAX_MESSAGE)*MAX_MESSAGE;
      __int64_t othersize = file_size%MAX_MESSAGE;
      int x;
      //use "sliding window" to copy buffer-sized file chunks to output file
      for(x = 0; x < regularsize; x +=MAX_MESSAGE){
        //build writebuffer
        filemsg file_request = filemsg(x, MAX_MESSAGE);
        char* write_buffer = new char(sizeof(filemsg)+sizeof(name)+1);
        *(filemsg*)write_buffer = file_request;
        //insert each char of the filename into the writebuffer
        for(int i = 0; i < sizeof(name); i ++){
          *(char*)(write_buffer + sizeof(filemsg) + i) = name[i];
        }
        *(char*)(write_buffer + sizeof(filemsg) + sizeof(filemsg)) = '\0';//null terminate buffer

        chan->cwrite(write_buffer, MAX_MESSAGE);
        char reponse_buffer[MAX_MESSAGE];
        chan->cread(reponse_buffer, sizeof(reponse_buffer));
        //write to output file
        outfile.write(reponse_buffer, sizeof(reponse_buffer));
      }
      //do last iteration of loop manually
        //last chunk of the file will be smaller than buffer size
      //build write buffer
      filemsg file_request = filemsg(x,othersize);
      char* write_buffer = new char[sizeof(filemsg) + sizeof(filemsg) + 1];
      *(filemsg*)write_buffer = file_request;
      for(int i = 0; i < sizeof(name); i++){
        *(char*)(write_buffer + sizeof(filemsg) + i ) = name[i];
      }
      *(char*)(write_buffer + sizeof(filemsg) + sizeof(filemsg)) = '\0';//null terminate buffer
      chan->cwrite(write_buffer, MAX_MESSAGE);
      char reponse_buffer[MAX_MESSAGE];
      chan->cread(reponse_buffer, sizeof(reponse_buffer));
      outfile.write(reponse_buffer, sizeof(reponse_buffer));
      //close output file
      outfile.close();
      t2 = tim.tv_usec;
      cout << "Time Elapsed: "<< t2-t1 << endl;
}

void newChannelTest(FIFORequestChannel* chan){
  MESSAGE_TYPE message = NEWCHANNEL_MSG;
  chan->cwrite(&message, sizeof(MESSAGE_TYPE));
  char buffer[MAX_MESSAGE];
  chan->cread(buffer, sizeof(MAX_MESSAGE));
  for(int i = 0; i < MAX_MESSAGE; i++){
    if(buffer[i] == NULL){
      buffer[i] = '\0';
    }
  }
  FIFORequestChannel new_channel = FIFORequestChannel(buffer, FIFORequestChannel::CLIENT_SIDE);
  cout << "New Channel Name: " << buffer << endl;
  datamsg msg(1,0,1);
  new_channel.cwrite((void*)&msg, sizeof(msg));
  double response;
  new_channel.cread((char*)&response, sizeof(double));
  cout << "Server response on new channel: " << response << endl;
  //closing the channel
  MESSAGE_TYPE m = QUIT_MSG;
  new_channel.cwrite (&m, sizeof (MESSAGE_TYPE));
}

void closeChannel(FIFORequestChannel* chan){
  cout << "Closing the Channel: " << endl;
  //cout << "Client-side is done and exited" << endl;
  MESSAGE_TYPE m = QUIT_MSG;
  chan->cwrite (&m, sizeof (MESSAGE_TYPE));
}

int main(int argc, char *argv[]){
  //Arguments: -p(person) -t(time) -e(ecg dataval)
  //-m max memory
  //-f filename
  //-c newchannel
  int person = 10;
  double time = 30;
  int ecg = 2;
  int arg;
  string fileName;
  bool requestSingleDataPoint = true;
  bool requestAllDataPoints = false;
  bool executeFileRequest = false;
  bool createNewChannel = false;
  //parse arguments
  while((arg = getopt(argc, argv, "p:t:e:f:c")) != -1){
    switch(arg){
      case 'p':
        person = atoi(optarg);
        requestSingleDataPoint = true;
        break;
      case 't':
        time = atof(optarg);
        requestSingleDataPoint = true;
        break;
      case 'e':
        ecg = atoi(optarg);
        requestSingleDataPoint = true;
        break;
      case 'f':
        cout << "HELLO" << endl;
        fileName = optarg;
        executeFileRequest = true;
        requestSingleDataPoint = false;
        break;
      case 'c':
        createNewChannel = true;
        break;
    }

  }
  //run server in child process
  int pid = fork();
  if(pid == 0){
    execv("server", {});
  }
  srand(time_t(NULL));//artificial delay
  FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

  if(requestSingleDataPoint){
    singleDataPoint(person, time, ecg, &chan);
  }
  if(requestAllDataPoints){
    allDataPoints(&chan);
  }
  if(executeFileRequest){
    fileRequest(fileName, &chan);
  }
  if(createNewChannel){
    newChannelTest(&chan);
  }
  // closing the channel
  closeChannel(&chan);
}
