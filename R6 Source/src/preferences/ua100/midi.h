#ifndef UMIDI_H
#define UMIDI_H

#include <OS.h>

void DT1(int fd, int32 address, int32 data);
void RQ1(int fd, int32 address);

struct Reader
{
  Reader(int fd) : fd(fd) {}
  virtual ~Reader();

  virtual void Parse(uint8);
  virtual void ParameterReceived();
  virtual void Run();

  int fd;
  thread_id run_thread;
  sem_id run_lock;
  uint32 step;
  int32 address;
  int32 value;
  int32 sum;
};

#endif
