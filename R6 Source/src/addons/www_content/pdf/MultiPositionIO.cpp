#include "MultiPositionIO.h"
#include <unistd.h>
#include <stdio.h>
#include <Debug.h>

MultiPositionIOServer::MultiPositionIOServer(BPositionIO *source)
 : fSource(source)
#if !defined(NDEBUG)
	,fClients(0)
#endif
{
}


MultiPositionIOServer::~MultiPositionIOServer()
{
#if !defined(NDEBUG)
	printf("~MultiPositionIOServer fClients: %ld\n", fClients);
	ASSERT(fClients == 0);
#endif
}

MultiPositionIOClient *MultiPositionIOServer::GetClient()
{
	return new MultiPositionIOClient(this, fSource);
}

MultiPositionIOClient::MultiPositionIOClient(MultiPositionIOServer *server, BPositionIO *source)
 : fServer(server), fSource(source), fPos(0)
{
#if !defined(NDEBUG)
	if (fServer->Lock())
	{
		fServer->fClients++;
		fServer->Unlock();
	}
#endif
}

#if !defined(NDEBUG)
MultiPositionIOClient::~MultiPositionIOClient()
{
	if (fServer->Lock())
	{
		fServer->fClients--;
		fServer->Unlock();
	}
}
#endif

ssize_t MultiPositionIOClient::Read(void *buffer, size_t size)
{
	ssize_t ret = B_ERROR;
	ret = ReadAt(fPos, buffer, size);
//	printf("MultiIO::Read() fPos: %Ld size: %ld ret: %ld", fPos, size, ret);
	if (ret >0)
		fPos = Seek(fPos + ret, SEEK_SET);
//	printf("new fPos: %Ld\n", fPos);
	return ret;
}

ssize_t MultiPositionIOClient::Write(const void *buffer, size_t size)
{
	ssize_t ret = B_ERROR;
	ret = WriteAt(fPos, buffer, size);
	if (ret >0)
		fPos = Seek(fPos + ret, SEEK_SET);
	fServer->Unlock();
	return ret;
}

ssize_t MultiPositionIOClient::ReadAt(off_t pos, void *buffer, size_t size)
{
	ssize_t ret = B_ERROR;

	if(fServer->Lock())
	{
		ret = fSource->ReadAt(pos, buffer, size);
		fServer->Unlock();
	}

	return ret;
}

ssize_t MultiPositionIOClient::WriteAt(off_t pos, const void *buffer, size_t size)
{
	ssize_t ret = B_ERROR;

	if(fServer->Lock())
	{
		ret = fSource->WriteAt(pos, buffer, size);
		fServer->Unlock();
	}

	return ret;
}

off_t MultiPositionIOClient::Seek(off_t position, uint32 seek_mode)
{
//	printf("MultiPIO::Seek(%Ld, %ld)", position, seek_mode);
	if (fServer->Lock()) {
		fPos = fSource->Seek(position, seek_mode);
		fServer->Unlock();
	}
	else fPos = 0;
//	printf("fPos: %Ld\n", fPos);
	return fPos;
}

off_t MultiPositionIOClient::Position() const
{
	return fPos;
}

status_t MultiPositionIOClient::SetSize(off_t size)
{
	status_t	ret;

	if(fServer->Lock())
	{
		ret = fSource->SetSize(size);
		fServer->Unlock();
	}
	else
		ret = B_ERROR;

	return ret;
}

#if defined TESTING

#include <File.h>
#include <stdio.h>
#include <OS.h>
#include <stdlib.h>
#include <time.h>

// compile with: gcc -DTESTING MultiPositionIO.cpp -lbe

int32 func(void *arg)
{
	MultiPositionIOClient *cli = (MultiPositionIOClient *)arg;

#define CYCLES 1000000

	off_t	mypos = 0;

	unsigned long int next = time(0L);

	for(int i = 0; i < CYCLES; i++)
	{
		// wondering why I inlined rand() here?
		// simple: libc's global lock was the
		// main source of locks while testing :)
		next = next * 1103515245 + 12345;
		int rand1 = ((next >> 16) & 0x7FFF);
		next = next * 1103515245 + 12345;
		int rand2 = ((next >> 16) & 0x7FFF);
		next = next * 1103515245 + 12345;
		int rand3 = ((next >> 16) & 0x7FFF);

		switch(int(((float)rand1 / (float)RAND_MAX) * 5))
		{
			case 0 :
				// test current position
				if(cli->Position() != mypos)
					printf("Position mismatch: %ld vs %ld\n", mypos, cli->Position());
				break;
			case 1 :
				// test seek position
				{
				off_t randompos = (int)(((float)rand2 / (float)RAND_MAX) * 256);
				off_t newpos = cli->Seek(randompos, SEEK_SET);
				if(newpos != randompos)
					printf("Seek mismatch: %Ld vs %Ld\n", randompos, newpos);
				mypos = randompos;
				}
				break;
			case 2 :
				// test Read
				{
				unsigned char ch;
				ssize_t ret = cli->Read(&ch, 1);
				if(ret > B_OK && ch != mypos)
					printf("Read mismatch: %d vs %d\n", mypos, ch);
				if(ret > B_OK)
					mypos += ret;
				}
				break;
			case 3 :
				// test ReadAt
				{
				off_t randompos = (int)(((float)rand2 / (float)RAND_MAX) * 256);
				unsigned char ch;
				cli->ReadAt(randompos, &ch, 1);
				if(ch != randompos)
					printf("ReadAt mismatch: %Ld vs %d\n", randompos, ch);
				}
				break;
			case 4 :
				// do nothing
				break;
		}
		snooze((bigtime_t)(((float)rand3 / (float)RAND_MAX) * 10000LL));
	}

	return 0;
}

main()
{
//	BFile f("test.file", O_RDWR | O_CREAT | O_TRUNC);
	BMallocIO f;

	for(int i = 0; i < 256; i++)
	{
		unsigned char ch = i & 255;
		f.Write(&ch, 1);
	}

	MultiPositionIOServer m(&f);

#define THREADS 32

	pid_t thr[THREADS];
	for(int j = 0; j < THREADS; j++)
	{
		MultiPositionIOClient *cli = m.GetClient();
		char buf[16];
		sprintf(buf, "test%d", j);
		resume_thread(thr[j] = spawn_thread(func, buf, B_NORMAL_PRIORITY, (void *)cli));
	}

	int32 junk;
	for(int k = 0; k < THREADS; k++)
	{
		wait_for_thread(thr[k], &junk);
	}

	return 0;
}

#endif
