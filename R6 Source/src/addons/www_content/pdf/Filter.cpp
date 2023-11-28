#include "Filter.h"

BInputFilter::BInputFilter(BInputFilter *source)
	: m_source(source)
{
}


BInputFilter::~BInputFilter()
{
	// take our source with us
	delete m_source;
}

ssize_t 
BInputFilter::Read(void *buffer, size_t size)
{
	return m_source ? m_source->Read(buffer, size) : ERROR;
}

void 
BInputFilter::Reset(void)
{
	if (m_source) m_source->Reset();
}

BInputFilter *
BInputFilter::SetSource(BInputFilter *new_source)
{
	BInputFilter *old_source = m_source;
	m_source = new_source;
	return old_source;
}

const BInputFilter *
BInputFilter::GetSource(void)
{
	return m_source;
}


BOutputFilter::BOutputFilter(BOutputFilter *sink)
	: m_sink(sink)
{
}


BOutputFilter::~BOutputFilter()
{
	delete m_sink;
}

ssize_t 
BOutputFilter::Write(void *buffer, size_t size)
{
	return m_sink ? m_sink->Write(buffer, size) : ERROR;
}

void 
BOutputFilter::Reset(void)
{
	if (m_sink) m_sink->Reset();
}

BOutputFilter *
BOutputFilter::SetSink(BOutputFilter *new_sink)
{
	BOutputFilter *old_sink = m_sink;
	m_sink = new_sink;
	return old_sink;
}

const BOutputFilter *
BOutputFilter::GetSink(void)
{
	return m_sink;
}
