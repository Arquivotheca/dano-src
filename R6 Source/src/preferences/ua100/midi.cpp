#include <unistd.h>
#include "midi.h"
#include "ua100.h"

static const uint8 DT1_template[16] = {
  0xf0, 0x41, 0x10, 0x00, 0x11, 0x12, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xf7, 0xf7, 0xf7, 0xf7
};

static const uint8 RQ1_template[16] = {
  0xf0, 0x41, 0x10, 0x00, 0x11, 0x11, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf7
};

static uint32
size_for(int32 address)
{
  if ((address & 0xfffff8ff) == 0x400000)
	if (address & 0x700)
	  return 2;
  return 1;
}

static int32
reader_thread(void* data)
{
  Reader* r = (Reader*) data;
  int fd = r->fd;
  r->step = 0;

  while (1) {
	uint8 x;
	if (read(fd, &x, 1) != 1)
	  return 0;
	if (acquire_sem(r->run_lock) != B_OK)
	  return 0;
	r->Parse(x);
	release_sem(r->run_lock);
  }
}

void
Reader::Run()
{
  run_lock = create_sem(1, "MIDI reader lock");
  run_thread = spawn_thread(reader_thread, "MIDI reader",
							B_NORMAL_PRIORITY, this);
  resume_thread(run_thread);
}

Reader::~Reader()
{
  acquire_sem(run_lock);
  delete_sem(run_lock);
}

void
Reader::Parse(uint8 x)
{
  if (x < 0xf8)
	if (step < 6) {
	  step = (x ==  DT1_template[step] ? step + 1 : 0);
	  address = 0;
	  value = 0;
	  sum = 0;
	}
	else if (step < 10) {
	  step = (x < 0x80 ? step + 1 : 0);
	  address = (address << 8) + x;
	  sum -= x;
	}
	else if (step < 10 + size_for(address)) {
	  step = (x < 0x80 ? step + 1 : 0);
	  value = (value << 8) + x;
	  sum -= x;
	}
	else if (step == 10 + size_for(address)) {
	  if (x == (sum & 0x7f))
		ParameterReceived();
	  step = 0;
	}
	else
	  step = 0;
}

void
Reader::ParameterReceived()
{
  //fprintf(stderr, "<= %x %d\n", address, value);

  // map UA-100 MODE to EFFECT MODE on input
  if (address == 0x400000)
	address = 0x404000;

  my_app->ReceiveControlValue(fd, address, value);
}

static uint8
checksum(uint8* sysex, int32 n)
{
  int32 cs = 0;
  for (int i = 0; i < n; i++)
	cs -= sysex[6 + i];
  return cs & 0x7f;
}

void
DT1(int fd, int32 address, int32 value)
{
  int32 size = size_for(address);
  int32 total_size = size + 12;
  uint8 sysex[16];

  //fprintf(stderr, "=> %x %d\n", address, value);
  memcpy(sysex, DT1_template, 16);
  address &= 0x7f7f7f7f;
  value &= 0x7f7f7f7f;

  sysex[9] = address;
  address >>= 8;
  sysex[8] = address;
  address >>= 8;
  sysex[7] = address;
  address >>= 8;
  sysex[6] = address;
  for (int i = 1; i <= size; i++) {
	sysex[10 + size - i] = value;
	value >>= 8;
  }
  sysex[total_size - 2] = checksum(sysex, total_size - 8);

  write(fd, sysex, total_size);
}

void
RQ1(int fd, int32 address)
{
  int32 size = size_for(address);
  uint8 sysex[16];

  //fprintf(stderr, "=> ? %x\n", address);
  memcpy(sysex, RQ1_template, 16);
  address &= 0x7f7f7f7f;
  size &= 0x7f7f7f7f;

  sysex[9] = address;
  address >>= 8;
  sysex[8] = address;
  address >>= 8;
  sysex[7] = address;
  address >>= 8;
  sysex[6] = address;
  sysex[13] = size;
  size >>= 8;
  sysex[12] = size;
  size >>= 8;
  sysex[11] = size;
  size >>= 8;
  sysex[10] = size;
  sysex[14] = checksum(sysex, 8);

  write(fd, sysex, 16);
}
