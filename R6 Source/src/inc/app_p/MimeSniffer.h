#include <String.h>
#include <DataIO.h>
#include <ObjectList.h>

#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING

struct entry_ref;

namespace BPrivate {

class Error {
public:
	Error(const char *message, int32 offset)
		:	error(message),
			offset(offset)
		{}

	virtual ~Error() {}

	int32 Offset() const
		{ return offset; }
	virtual const char *ErrorMessage() const
		{ return error; }

private:
	const char *error;
	int32 offset;
};

class Pattern : public BString {
	// BString does not handle strings with nulls inside well,
	// replace the calls that break;
public:
	Pattern(const BString &);
	Pattern(const Pattern &);
	Pattern(const char *, int32 length);
	
	void SetTo(const char *, int32 length);
};


class SniffBuffer {
	// Lazy-reading sniff buffer

public:
	SniffBuffer(BPositionIO *);
	SniffBuffer(const void *, int32 );
	virtual ~SniffBuffer();

	bool CaseSensitiveMatch(const BString &pattern, off_t offset) const;
	bool CaseSensitiveMatch(const char *, off_t offset) const;
	bool CaseInsensitiveMatch(const BString &pattern, off_t offset) const;
	bool CaseSensitiveMatchFirst(char, off_t offset) const;
	bool CaseSensitiveMatchWithMask(const BString &pattern, const BString &mask,
		off_t offset) const;

	void FillBufferIfNeeded(off_t size) const;
	uchar operator[](int32) const;
	off_t Size() const
		{ return targetSize; }

	const char *Buffer(off_t guaranteedSize) const;
		// map <guaranteedSize> bytes into memory for quick access

private:
	void FillBuffer(off_t size);
	
	mutable char *buffer;
	mutable off_t bufferSize;
	mutable bool fileEndHit;
	mutable BPositionIO *target;
	off_t targetSize;
	const void *externalBuffer;
};

class SnifferRuleBase {
public:
	SnifferRuleBase(const char *type, float matchLevel)
		:	type(type),
			matchLevel(matchLevel)
		{}
	
	virtual ~SnifferRuleBase() {}

	const char *Type() const
		{ return type.String(); }
	float MatchLevel() const
		{ return matchLevel; }

	virtual bool Match(const SniffBuffer *) const = 0;
	virtual void Dump(BString &dumpBuffer, bool ruleOnly) const;

private:
	BString type;
	float matchLevel;
};


class SimpleSnifferRule : public SnifferRuleBase {
	// pattern at a given offset
public:
	// pattern can contain zeroes in the middle of the string
	SimpleSnifferRule(const char *type, float matchLevel, const BString &pattern,
		off_t sniffOffsetStart = 0);

	virtual bool Match(const SniffBuffer *) const;
	virtual void Dump(BString &dumpBuffer, bool ruleOnly) const;

protected:
	Pattern pattern;
	off_t sniffOffsetStart;
};

class RangeSnifferRule : public SimpleSnifferRule {
	// pattern at a given range of offsets
public:
	// pattern can contain zeroes in the middle of the string
	RangeSnifferRule(const char *type, float matchLevel, const BString &pattern,
		off_t sniffOffsetStart, off_t sniffOffsetEnd);

	virtual bool Match(const SniffBuffer *) const;
	virtual void Dump(BString &dumpBuffer, bool ruleOnly) const;

protected:
	off_t sniffOffsetEnd;
};

class RangeSnifferRuleWithMask : public RangeSnifferRule {
	// pattern at a given range of offsets with a bitmask
public:
	RangeSnifferRuleWithMask(const char *type, float matchLevel,
		const BString &pattern, const BString &mask,
		off_t sniffOffsetStart, off_t sniffOffsetEnd);

	virtual bool Match(const SniffBuffer *) const;
	virtual void Dump(BString &dumpBuffer, bool ruleOnly) const;

protected:
	Pattern mask;
};
 

class SingleRangeMultiplePatternSnifferRule : public SnifferRuleBase {
	// multiple OR'd patterns at single offset range
public:
	SingleRangeMultiplePatternSnifferRule(const char *type, float matchLevel,
		BObjectList<Pattern> *, off_t sniffOffsetStart, off_t sniffOffsetEnd,
		bool caseInsensitive);
	virtual ~SingleRangeMultiplePatternSnifferRule();

	virtual bool Match(const SniffBuffer *) const;
	virtual void Dump(BString &dumpBuffer, bool ruleOnly) const;

protected:
	BObjectList<Pattern> *patternList;
	off_t sniffOffsetStart;
	off_t sniffOffsetEnd;
	bool caseInsensitive;
};

class OptimizedSingleRangeMultiplePatternSnifferRule
	: public SingleRangeMultiplePatternSnifferRule {
	// multiple OR'd patterns at single offset range, first character matches,
	// allowing for an optimized match
public:
	OptimizedSingleRangeMultiplePatternSnifferRule(const char *type, float matchLevel,
		BObjectList<Pattern> *, off_t sniffOffsetStart, off_t sniffOffsetEnd,
		bool caseInsensitive);

	virtual bool Match(const SniffBuffer *) const;

protected:
	char firstPatternCharacter;
};

class PatternAtOffsetRange {
public:
	PatternAtOffsetRange(const BString &pattern, off_t start, off_t end)
		:	pattern(pattern),
			start(start),
			end(end)
		{}

	const Pattern &PatternValue() const
		{ return pattern; }

	const off_t &Start() const
		{ return start; }

	const off_t &End() const
		{ return end; }

protected:
	Pattern pattern;
	off_t start;
	off_t end;
};

class MultiplePatternSnifferRule : public SnifferRuleBase {
	// multiple OR'd patterns at multiple offset ranges
public:
	MultiplePatternSnifferRule(const char *type, float matchLevel,
		BObjectList<PatternAtOffsetRange> *, bool caseInsensitive);
	virtual ~MultiplePatternSnifferRule();

	virtual bool Match(const SniffBuffer *) const;
	virtual void Dump(BString &dumpBuffer, bool ruleOnly) const;

protected:
	BObjectList<PatternAtOffsetRange> *patternList;
	bool caseInsensitive;
};

class AlgorithmicSniffer : public SnifferRuleBase {
	// pattern at a given range of offsets
public:
	// pattern can contain zeroes in the middle of the string
	AlgorithmicSniffer(const char *type, float matchLevel,
		bool (*)(const SniffBuffer *, off_t, off_t),
		off_t , off_t );

	virtual bool Match(const SniffBuffer *) const;
	virtual void Dump(BString &dumpBuffer, bool ruleOnly) const;

protected:
	bool (*matcher)(const SniffBuffer *buffer, off_t rangeStart, off_t rangeEnd);
	off_t rangeStart;
	off_t rangeEnd;
};


class Error;

class Sniffer {
public:
	Sniffer(bool readRulesFromMIMEDatabase);
	Sniffer(const entry_ref *ruleFile);
	
	void AddRules(const entry_ref *ruleFile);
	void AddRulesFromMIMEDatabase();
	void DumpRules();
	void DumpRule(const char *type, BString &rule);
	
	const SnifferRuleBase *SniffFile(const entry_ref *) const;
	const SnifferRuleBase *SniffMemory(void *, size_t) const;
	const SnifferRuleBase *Sniff(SniffBuffer *) const;

	static bool CheckRule(const char *rule, BString *error = NULL);
		// do a syntax check on a new rule, return possible errors
	
	const BObjectList<SnifferRuleBase> *Rules() const
		{ return &rules; }

	void RuleChanged(const char *type, const char *rule);

	static bool LooksLikeText(const SniffBuffer *buffer, off_t rangeStart, off_t rangeEnd);
		// this algoritmic sniffer call needs to be used separately
	
private:

	Sniffer();
	// rule parser stuff

	class RuleScanner {
	public:
		enum TokenType {
			kUnknown,
			kEnd,
			kControlToken,
			kOptionToken,
			kStringLiteral
		};
		
		RuleScanner(const char *stream, size_t streamLength);
		TokenType GetNextToken() throw (Error);
		TokenType ExpectNextToken(const char *noTokenMessage) throw (Error);
		void ExpectNextToken(TokenType type, const char *wrongTokenMessage) throw (Error);
		void ExpectNextToken(TokenType type, const char *tokenValue,
			const char *wrongTokenMessage) throw (Error);

		int32 ExpectInt32(const char *missingMessage, const char *badFormatMessage) throw (Error);

		const char *TokenValue() const
			{ return tokenValue; }
		int32 TokenLength() const
			{ return length; }
		TokenType Token() const
			{ return currentToken; }

		int32 Offset() const
			{ return offset; }

		const char *Stream() const
			{ return stream; }
		
		void PrintScanError(const Error *error) const;
		void PrintScanError(const Error *error, BString &buffer) const;
	private:
		TokenType ReturnToken(int32 tokenOffset, int32 tokenEnd);
		TokenType ReturnToken(TokenType, int32 tokenOffset, int32 tokenEnd);
		void SkipComment();
	
		const char *stream;
		size_t streamLength;
		int32 offset;
		int32 tokenStart;
		BString deescapedToken;
		bool deescapingToken;
		
		const char *tokenValue;
		int32 length;

		TokenType currentToken;
	};

	SnifferRuleBase *GetRule(RuleScanner *);
	SnifferRuleBase *GetRule(const char *type, RuleScanner *);
	SnifferRuleBase *GetRuleCommon(const char *type, RuleScanner *) const throw (Error);
	void CheckCurrentToken(const Sniffer::RuleScanner *, RuleScanner::TokenType,
		const char *expectedValue, const char *error) const throw (Error);

	bool ParseSniffRange(RuleScanner *, off_t &start, off_t &end) const throw (Error);

	bool AddRuleUnique(SnifferRuleBase *newRule);
		// check if type not in list yet, insert by match level

	BObjectList<SnifferRuleBase> rules;
	
	friend class ParseError;
	friend class ScanError;	
};

const off_t kBufferReadMin = 1024;
	// reading any less than this will not make much of a difference
	// (in best case the data will be in the fs cache but the resulting
	// memcopy will still probably be faster than the system call

const off_t kBufferReadSlop = 1024;
	// if we read more data, read this much

const off_t kBufferReadMax = 10*1024;
	// guard against over-zealous sniff rules, don't read any more than this

inline void 
SniffBuffer::FillBufferIfNeeded(off_t size) const
{
	if (size > bufferSize && size <= targetSize && size <= kBufferReadMax)
		// we are either fine, have the whole file or reached the
		// limit ammout of file we are ready to read
		const_cast<SniffBuffer *>(this)->FillBuffer(size);
}

inline bool 
SniffBuffer::CaseSensitiveMatchFirst(char ch, off_t offset) const
{
	off_t size = offset + 1;
	if (size > bufferSize && size <= targetSize && size <= kBufferReadMax)
		// we are either fine, have the whole file or reached the
		// limit ammout of file we are ready to read
		const_cast<SniffBuffer *>(this)->FillBuffer(size);
	
	return buffer && buffer[offset] == ch;
}

inline uchar 
SniffBuffer::operator[](int32 offset) const
{
	FillBufferIfNeeded(offset + 1);
	ASSERT(buffer);
	return buffer ? buffer[offset] : '\0';
}

inline const char *
SniffBuffer::Buffer(off_t guaranteedSize) const
{
	FillBufferIfNeeded(guaranteedSize);
	ASSERT(buffer);
	return buffer;
}


} // namespace BPrivate

#endif	/* _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING */
