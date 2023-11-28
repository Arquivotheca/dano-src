// ----------------------------------------------------------------------------------------------
/* 
	SpyData 
	 
	Copyright (c) 2001 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis
			March 1, 2001
 
	Global repository for data in Spy. 
*/ 
// ----------------------------------------------------------------------------------------------

#ifndef SPYDATA_H
#define SPYDATA_H

#include <experimental/MultiLocker.h>
#include <Message.h>


#define B_SPY_DATA_MESSAGE	'_SDM'
#define B_SPY_MODE_CHANGED	'_SMC'

/*! Some constants for standard data. */
#define B_SPY_MODE					"be:Spy-O-Matic.SpyMode"
#define B_SPY_REMOTE_MESSAGE_ORIGIN	"be:Spy-O-Matic.RemoteMessageOrigin"
#define B_SPY_REMOTE_MESSAGE_TARGET	"be:Spy-O-Matic.RemoteMessageTarget"

#include <String.h>

namespace Spy {

/*!
	Global repository for persistent data in the Spy-O-Matic system.	
 */


class SpyData
{
	public:
		/*!
			Instantiate this to get to the persistent data.
		 */
		SpyData();
		~SpyData();
		
		/*!
			\param Name The identifier for the data.
			\param Value The data itself
			
		 	Set and Get functions, these are thread safe and all that.
		 */
		// Automagic locking functions.
		void		SetString(const BString& Name, const BString& Value);
		void		SetInt32(const BString& Name, int32 Value);
		void		SetFloat(const BString& Name, float Value);
		
		BString		GetString(const BString& Name);
		int32		GetInt32(const BString& Name);
		float		GetFloat(const BString& Name);
		

		/*!
			\returns The BMessage where the data is stored for unsafe access.
			
			Use this if you need all the data. Make sure you use the manual locking functions.
		 */
		// unlocked access.
		BMessage&	DataMessage();


		/*!
			Manual locking mechanisim, not exactly complex. Use this with the DataMessage() function.
		 */		
		// Manual Message locking.
		void		LockMessage();
		void		UnlockMessage();

	private:
	
		static BMultiLocker sLock;
		static BMessage sDataMessage;

};	// SpyData

enum SpyMode
{
	smRemote,
	smLocal
};

}	// Spy

#endif

