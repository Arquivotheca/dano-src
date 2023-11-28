#ifndef _AUTOPTR_H
#define _AUTOPTR_H

// AutoPtr.h


template <class T>
class AutoPtr
{
public:
	AutoPtr() {
		fVal = new T;
		array = FALSE;
	}
	AutoPtr(long num) {
		fVal = new T[num];
		array = TRUE;
	}
	~AutoPtr() {
		if (array)
			delete[] fVal;
		else
			delete fVal;
	}
	T	*Value() {
		return fVal;
	}
	T&	operator* () {
		return *fVal;
	}
	T 	*operator& () {
		return fVal;
	}
private:
	T		*fVal;
	bool	array;
};

#endif
