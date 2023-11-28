//========================================================================
//	MPrefsContainer.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPREFSCONTAINER_H
#define _MPREFSCONTAINER_H

#include <Message.h>

class MBlockFile;

class MPrefsContainer
{
public:
								MPrefsContainer();
								~MPrefsContainer();

		void					GetData(
									BMessage&	inOutMessage) const;
		void					SetData(
									BMessage&	inOutMessage);
		void					ReadFromFile(
									MBlockFile & inFile);
		void					WriteToFile(
									MBlockFile & inFile) const;
		void					CopyTo(
									BMessage&	inTo) const;

		void					FillMessage(
									BMessage&		inMessage,
									uint32			inType) const;
		void					Flatten(
									char*&			outData,
									size_t&			outSize) const;
		void					UnFlatten(
									const char*		inData);

		bool					ValidateGenericData();
		void					RemoveData(
									const char *	inName);

		static void				CopyData(
									BMessage&	inFrom,
									BMessage&	inTo);
		static void				FillMessage(
									BMessage&		inFrom,
									BMessage&		inTo,
									uint32			inType);

private:

		BMessage*				fMessage;
};

#endif
