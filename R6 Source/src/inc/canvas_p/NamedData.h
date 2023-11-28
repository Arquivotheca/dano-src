#ifndef NAMED_DATA_H
#define NAMED_DATA_H

#include <SupportDefs.h>
#include <String.h>

/*
	Interface: BNamedData
	
	BNamedData represents some data that can be associated with
	a name.  You give it a type, and name, and fill in the data.
	In the cases of string, RAW, ane MESSAGE types, the data is
	copied and owned by this object.
	
	You will use BNamedData in larger sets like taggedStructure to quickly
	and easily access data/value pairs without having to use
	a more heavy weight structure.
*/

class BNamedData
{
public:
				BNamedData();
				BNamedData(const BNamedData &);
				BNamedData(BNamedData *);
				BNamedData(const char *name, const int32 aType, 
						void *data, const int32 size); 
	virtual		~BNamedData();
	
		BNamedData & operator = (const BNamedData &);
		
				BString &	Name();
				void	GetData(void *data, int32 &aType);
				int32	DataType() const {return fDataType;};
				int32	DataSize() const {return fDataSize;};
				
				bool	AsBool();
				int8	AsInt8();
				int16	AsInt16();
				int32	AsInt32();
				int64	AsInt64();
				float	AsFloat();
				double	AsDouble();
				char *	AsString();
				void *	AsPointer();

			void	Print();
			
protected:
			void	BNamedData_Initialize(const char *name, const int32 aType, 
						void *data, const int32 size);
			
	int32	fDataType;
	union all_data_types {
		bool	fBool;
		int8	fInt8;
		int16	fInt16;
		int32	fInt32;
		int64	fInt64;
		float	fFloat;
		double	fDouble;
		char *	fString;
		void *	fPointer;
	} fData;
	int32	fDataSize;
	BString	fName;
		
private:
};

#endif
