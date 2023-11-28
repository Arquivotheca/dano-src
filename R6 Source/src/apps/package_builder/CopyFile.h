//// CopyFile.h

#ifndef _COPYFILE_H
#define _COPYFILE_H


enum {
	M_UPDATE_TEMP = 'upTM'
};

long	CopyFile(BEntry *dst, BEntry *src, BMessenger updateMess);
// long	CopyBytes(BFile *dst, BFile *src, long count);

#endif
