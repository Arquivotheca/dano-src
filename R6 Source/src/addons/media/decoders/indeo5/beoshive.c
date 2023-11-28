/*
 * HIVE callbacks for beos
 */

#include <malloc.h>
#include <string.h>

#include <OS.h>

#undef MIN
#undef MAX

#include "datatype.h"
#include "pia_main.h"

int64 _imp__QueryPerformanceCounter()
{
    return system_time() * 300;
}
int64 _imp__QueryPerformanceFrequency()
{
    return 300000000LL;
}


/*
 * init
 */

void
HiveInitDecoderPersistentData()
{
}

/*
 * time
 */

U32
HiveReadTime( void )
{
	return system_time();
}

/*
 * allocation
 */

PVOID_GLOBAL
HiveAllocSharedPtr( U32 uSize, PU8 pc8Name, PPIA_Boolean pbExists )
{
	PVOID_GLOBAL ret = (PVOID_GLOBAL) malloc(uSize);
	*pbExists = FALSE;
	return ret;
}

PIA_RETURN_STATUS
HiveFreeSharedPtr( PVOID_GLOBAL p )
{
	free(p);
	return PIA_S_OK;
}

PVOID_GLOBAL
HiveGlobalAllocPtr( U32 uSize, PIA_Boolean bZeroInit )
{
	PVOID_GLOBAL ret = (PVOID_GLOBAL) malloc(uSize);
	if (bZeroInit)
		memset(ret, 0, uSize);
	return ret;
}

PIA_RETURN_STATUS
HiveGlobalFreePtr( PVOID_GLOBAL p )
{
	free(p);
	return PIA_S_OK;
}

NPVOID_LOCAL
HiveLocalAllocPtr( U32 uSize, PIA_Boolean bZeroInit)
{
	NPVOID_LOCAL ret = (NPVOID_LOCAL) malloc(uSize);
	if (bZeroInit)
		memset(ret, 0, uSize);
	return ret;
}

PIA_RETURN_STATUS
HiveLocalFreePtr( NPVOID_LOCAL p )
{
	free(p);
	return PIA_S_OK;
}

PIA_RETURN_STATUS
HiveGlobalPtrCheck( PVOID_GLOBAL vgpPtr, U32 uLength )
{
	if(vgpPtr == NULL)
		return PIA_S_ERROR;
	return PIA_S_OK;
}

/*
 * syncronization
 */

MUTEX_HANDLE
HiveCreateMutex( PU8 pc8Name)
{
	sem_id sem = create_sem(1, (const char *)pc8Name);
	if(sem < 0)
		return NULL;
	else
		return (MUTEX_HANDLE)sem;
}

extern PIA_RETURN_STATUS
HiveFreeMutex( MUTEX_HANDLE muHandle )
{
	status_t err;
	if(muHandle == NULL)
		return PIA_S_ERROR;
	err = delete_sem((sem_id)muHandle);
	return err == B_NO_ERROR ? PIA_S_OK : PIA_S_ERROR;
}

PIA_RETURN_STATUS
HiveBeginCriticalSection(MUTEX_HANDLE muHandle, U32 uTimeout)
{
	status_t err;
	if(muHandle == NULL)
		return PIA_S_ERROR;
	err = acquire_sem_etc((sem_id)muHandle, 1, uTimeout != 0 ? B_TIMEOUT : 0, (bigtime_t)uTimeout * 1000);
	if(err == B_TIMED_OUT)
		return PIA_S_TIMEOUT;
	else if(err == B_NO_ERROR)
		return PIA_S_OK;
	else
		return PIA_S_ERROR;
}

PIA_RETURN_STATUS
HiveEndCriticalSection( MUTEX_HANDLE muHandle )
{
	status_t err;
	if(muHandle == NULL)
		return PIA_S_ERROR;
	err = release_sem((sem_id)muHandle);
	return err == B_NO_ERROR ? PIA_S_OK : PIA_S_ERROR;
}
