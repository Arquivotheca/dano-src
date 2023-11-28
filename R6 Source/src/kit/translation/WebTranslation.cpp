
#include <Autolock.h>
#include <string.h>
#include <Application.h>
#include <stdlib.h>

#include "WebTranslation.h"


BRunningTranslation::BRunningTranslation(const char *input_mime, type_code output_type, BPositionIO *input, BPositionIO *output, BTranslatorRoster * roster, int32 id) :
	fSource(input),
	fDestination(output),
	fLocker("BRunningTranslation")
{
	fThread = 0;
	if (roster == 0) {
		roster = BTranslatorRoster::Default();
	}
	if (roster == 0) {
		fError = B_NO_INIT;
		return;
	}
	fRoster = roster;
	translator_id * list;
	int32 count;
	fError = roster->GetAllTranslators(&list, &count);
	if (fError < 0) {
		return;
	}
	fOutputType = output_type;
	if (id > 0) {
		fTranslator = id;
	}
	else for (int ix=0; ix<count; ix++) {
		const translation_format * fmt;
		int32 fcnt;
		fError = roster->GetInputFormats(list[ix], &fmt, &fcnt);
		if (fError < 0) {
			goto next_translator;
		}
		for (int iy=0; iy<fcnt; iy++) {
			const translation_format * ofmt;
			int32 ocnt;
			if (!strcasecmp(fmt[iy].MIME, input_mime)) {	//	this is a candidate
				fError = roster->GetOutputFormats(list[ix], &ofmt, &ocnt);
				if (fError < 0) {
					goto next_translator;
				}
				for (int iz=0; iz<ocnt; iz++) {
					if (!output_type || (ofmt[iz].type == output_type)) {
						fTranslator = list[ix];
						fError = 0;
						goto found_translator;
					}
				}
			}
		}
next_translator:
		const char * name, * info;
		int32 version;
		roster->GetTranslatorInfo(list[ix], &name, &info, &version);
		;
	}
	fError = B_NO_TRANSLATOR;
	return;
found_translator:
	delete[] list;
}


BRunningTranslation::~BRunningTranslation()
{
	if (fThread > 0) wait_for_thread(fThread, &fError);
}

status_t 
BRunningTranslation::ErrorCheck()
{
	return fError;
}

status_t
BRunningTranslation::Go()
{
	if (fError < 0) {
		return fError;
	}
	fThread = spawn_thread(translate_thread, "translate_thread", 10, this);
	if (fThread < 0) {
		fError = fThread;
		return fError;
	}
	fError = resume_thread(fThread);
	return fError;
}

status_t 
BRunningTranslation::WaitForCompletion()
{
	status_t s;
	status_t err = wait_for_thread(fThread, &s);
	if (err < 0) return err;
	return fError;
}

status_t 
BRunningTranslation::SetCompletionMessage(const BMessage &inMessage, const BMessenger &inTarget)
{
	fLocker.Lock();
	if (fThread < 0) {
		fLocker.Unlock();
		return fThread;
	}
	fCompletionMessage = inMessage;
	fCompletionMessenger = inTarget;
	fLocker.Unlock();
	return B_OK;
}

status_t
BRunningTranslation::translate_thread(void * arg)
{
	BRunningTranslation * x = (BRunningTranslation *)arg;
	status_t err = x->fRoster->Translate(x->fTranslator, x->fSource, 0, x->fDestination, x->fOutputType);
	x->fLocker.Lock();
	if (err < 0) {
		x->fError = err;
	}
	if (x->fCompletionMessenger.IsValid()) {
		x->fCompletionMessenger.SendMessage(&x->fCompletionMessage, be_app);
	}
	x->fThread = -1;
	x->fLocker.Unlock();
	return err;
}





BProgressiveInputStream::BProgressiveInputStream(off_t knownSize) :
	fLock("ProgressiveInputStream")
{
	fKnownSize = knownSize;
	fAmtSem = create_sem(0, "waiting for PutChunk()");
	fCurAmt = 0;
	fData = 0;
	fSize = 0;
	fPosition = 0;
}


BProgressiveInputStream::~BProgressiveInputStream()
{
	free(fData);
	fData = 0;
	delete_sem(fAmtSem);
}

ssize_t 
BProgressiveInputStream::ReadAt(off_t pos, void *buffer, size_t size)
{
	fLock.Lock();
	if (fKnownSize >= 0 && fKnownSize <= pos) {
		fLock.Unlock();
		return 0;
	}
	if (pos+size > fCurAmt) {
		fLock.Unlock();
		status_t err = acquire_sem_etc(fAmtSem, (pos+size)-fCurAmt, 0, 0);
		if (err == B_OK) {	//	OK, we got what we asked for
			fLock.Lock();
			fCurAmt = pos+size;
			memcpy(buffer, ((char *)fData)+pos, size);
			fLock.Unlock();
			return size;
		}
		else if (err != B_BAD_SEM_ID) {
			return err;
		}
		else {	//	last buffer was put in while we waited, so sem was deleted
			if (!fData) {
				return B_ERROR;	//	like, blocking when deleted
			}
			fLock.Lock();
			if (pos >= fKnownSize) {
				fLock.Unlock();
				return 0;
			}
			size_t copy = fKnownSize-pos;
			if (copy > size) copy = size;
			memcpy(buffer, ((char *)fData)+pos, copy);
			fLock.Unlock();
			return copy;
		}
	}
	memcpy(buffer, ((char *)fData)+pos, size);
	fLock.Unlock();
	return size;
}

ssize_t 
BProgressiveInputStream::WriteAt(off_t pos, const void *buffer, size_t size)
{
	return EPERM;
}

off_t 
BProgressiveInputStream::Seek(off_t position, uint32 seek_mode)
{
	fLock.Lock();
	switch (seek_mode) {
	case 0:
		if (position < 0) {
			fLock.Unlock();
			return B_BAD_VALUE;
		}
		fPosition = position;
		break;
	case 1:
		if (position + fPosition < 0) {
			fLock.Unlock();
			return B_BAD_VALUE;
		}
		fPosition += position;
		break;
	case 2:
		if (fKnownSize < 0) {
			fLock.Unlock();
			//	wait until size is known
			(void)KnownSize(true);
			fLock.Lock();
		}
		if (fKnownSize + position < 0) {
			fLock.Unlock();
			return B_BAD_VALUE;
		}
		fPosition = fKnownSize + position;
		break;
	default:
		fLock.Unlock();
		return B_BAD_VALUE;
	}
	off_t ret = fPosition;
	fLock.Unlock();
	return ret;
}

off_t 
BProgressiveInputStream::Position() const
{
	return fPosition;
}

status_t 
BProgressiveInputStream::SetSize(off_t)
{
	return EPERM;
}

off_t
BProgressiveInputStream::KnownSize(bool syncWait)
{
	if (!syncWait) {
		return fKnownSize < 0;
	}
	if (fKnownSize < 0) {
		status_t err = acquire_sem_etc(fAmtSem, LONG_MAX, 0, 0);
		if (err < 0) return err;
	}
	return fKnownSize;
}

status_t 
BProgressiveInputStream::PutChunk(const void *data, size_t size, bool lastChunk)
{
	fLock.Lock();
	void * nx = realloc(fData, fSize+size);
	if (nx == 0) {
		fLock.Unlock();
		return B_NO_MEMORY;
	}
	memcpy(((char *)nx)+fSize, data, size);
	fData = nx;
	fSize += size;
	if (lastChunk) {
		fKnownSize = fSize;
		delete_sem(fAmtSem);
		fAmtSem = -1;
	}
	else {
		release_sem_etc(fAmtSem, size, 0);
	}
	fLock.Unlock();
	return B_OK;
}

