#include "DataIOSink.h"

DataIOSink::DataIOSink(BDataIO *sink)
	: Pusher(), fIO(sink)
{
}


DataIOSink::~DataIOSink()
{
}

ssize_t 
DataIOSink::Write(const uint8 *buffer, ssize_t length, bool)
{
	// easy, just write the data to our fIO
	return fIO->Write(buffer, length);
}

