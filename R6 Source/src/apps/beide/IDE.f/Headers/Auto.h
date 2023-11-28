//========================================================================
//	Auto.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Simple stack=based class to remove a file pointed to by a BEntry when
//	the auto_Entry goes out of scope.

class auto_Entry
{
public:
								auto_Entry(
									BEntry*	inEntry)
								: fEntry(inEntry) {}
								~auto_Entry()
								{
									if (fEntry != NULL)
										fEntry->Remove();
								}
	void						release()
								{
									fEntry = NULL;
								}
private:
			BEntry*			fEntry;
};

