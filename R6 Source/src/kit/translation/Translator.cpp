/*	Copyright 1999 Be Incorporated. */

#include <Translator.h>

BTranslator::BTranslator()
{
	_mRefCount = 1;
}

BTranslator *
BTranslator::Acquire()
{
	atomic_add(&_mRefCount, 1);
	return this;
}

BTranslator *
BTranslator::Release()
{
	if (atomic_add(&_mRefCount, -1) == 1) {
		delete this;
		return NULL;
	}
	return this;
}

int32
BTranslator::ReferenceCount()
{
	return _mRefCount;
}

const char *
BTranslator::TranslatorName() const
{
	return NULL;
}

const char *
BTranslator::TranslatorInfo() const
{
	return NULL;
}

int32
BTranslator::TranslatorVersion() const
{
	return 0;
}

const translation_format *
BTranslator::InputFormats(
	int32 * out_count) const
{
	if (out_count) *out_count = 0;
	return NULL;
}

const translation_format *
BTranslator::OutputFormats(
	int32 * out_count) const
{
	if (out_count) *out_count = 0;
	return NULL;
}

status_t
BTranslator::Identify(	/*	required	*/
	BPositionIO * inSource,
	const translation_format * inFormat,	/*	can beNULL	*/
	BMessage * ioExtension,	/*	can be NULL	*/
	translator_info * outInfo,
	uint32 outType)
{
	return B_NO_TRANSLATOR;
}

status_t
BTranslator::Translate(	/*	required	*/
	BPositionIO * inSource,
	const translator_info * inInfo,
	BMessage * ioExtension,	/*	can be NULL	*/
	uint32 outType,
	BPositionIO * outDestination)
{
	return B_NO_TRANSLATOR;
}

status_t
BTranslator::MakeConfigurationView(	/*	optional	*/
	BMessage * ioExtension,	/*	can be NULL	*/
	BView * * outView,
	BRect * outExtent)
{
	return B_ERROR;
}


status_t
BTranslator::GetConfigurationMessage(	/*	optional	*/
	BMessage * ioExtension)
{
	return B_ERROR;
}


BTranslator::~BTranslator()	/* because it's ref counted */
{
}

status_t BTranslator::_Reserved_Translator_0(int32, void *) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_1(int32, void *) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_2(int32, void *) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_3(int32, void *) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_4(int32, void *) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_5(int32, void *) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_6(int32, void *) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_7(int32, void *) { return B_ERROR; }
