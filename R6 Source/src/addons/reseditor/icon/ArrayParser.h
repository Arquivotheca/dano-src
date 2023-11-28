/***************************************************************************
//
//	File:			ArrayParser.h
//
//	Description:	BArrayParser class
//
//	Copyright 1999, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef ARRAY_PARSER_H
#define ARRAY_PARSER_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

#include <SupportDefs.h>
#include <Path.h>
#include <Archivable.h>
#include <DataIO.h>
#include <List.h>
#include <Locker.h>
#include <String.h>

#include <ErrorInfo.h>

#include <stdarg.h>

namespace BPrivate {
	class ArrayParserState;
}

using namespace BPrivate;

class BArrayParser
{
public:
	BArrayParser();
	virtual ~BArrayParser();
	
	status_t Init();
	status_t Run(const char* data, size_t size);

	// Call-backs when running.
	virtual status_t ReadArray(const void* data, size_t size,
							   size_t bytes_per_entry,
							   const char* identifier,
							   const BMessage& meta_data);
	virtual void Error(const ErrorInfo& info);
	virtual void Warn(const ErrorInfo& info);
	
	void AddError(const ErrorInfo& info);
	size_t CountErrors() const;
	const ErrorInfo& ErrorAt(size_t idx=0) const;
	
private:
	friend class BPrivate::ArrayParserState;
	
	void FreeData();
	
	BPrivate::ArrayParserState* fParser;
	
	int32 fErrorCount;
	ErrorInfo fError;
	ErrorInfo fNoError;
};

#endif
