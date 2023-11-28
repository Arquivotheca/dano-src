/*	$Id: CAlloca.h,v 1.1 1998/10/21 12:02:58 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 02/24/98 12:52:30
*/

const int kAllocaBufferSize = 4096;

class CAlloca {
public:
		CAlloca(int size);
		~CAlloca();
		
		operator char*();
		char& operator[] (int ix);
		
private:
		char fBuffer[kAllocaBufferSize];
		char *fPtr;
};

inline CAlloca::CAlloca(int size)
{
	if (size > kAllocaBufferSize)
		fPtr = (char *)malloc(size);
	else
		fPtr = fBuffer;
} /* CAlloca::CAlloca */

inline CAlloca::~CAlloca()
{
	if (fPtr != fBuffer)
		free(fPtr);
} /* CAlloca::~CAlloca */

inline CAlloca::operator char*()
{
	return fPtr;
} /* CAlloca::operator char* */

inline char& CAlloca::operator[] (int ix)
{
	return fPtr[ix];
} /* CAlloca::operator[] */
