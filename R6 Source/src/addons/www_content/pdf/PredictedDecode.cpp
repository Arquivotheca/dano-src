#include "PredictedDecode.h"


PredictedDecode::PredictedDecode(Pusher *sink, uint32 predictor, uint32 columns, uint32 colors, uint32 bitsPerComponent)
	: Pusher(sink), m_predictor(predictor), m_columns(columns), m_colors(colors), m_bitsPerComponent(bitsPerComponent)
{
}


PredictedDecode::~PredictedDecode()
{
}

ssize_t 
PredictedDecode::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	return Pusher::SINK_FULL;
}

