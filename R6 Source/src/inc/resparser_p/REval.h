#ifndef _R_EVAL_H
#define _R_EVAL_H

#include <RSymbol.h>

namespace BPrivate {

struct RElem;
class ResourceParserState;

enum REvalType {
	retValue,
	retOperator,
	retFunction,
	retIdentifier,
	retOffset
};

enum REvalOp {
	reoPlus, reoMinus, reoMultiply, reoDivide, reoModulus, reoNegate, 
	reoSHL, reoSHR, reoBitAnd, reoBitOr, reoBitFlip, reoNot, reoAnd, reoOr, 
	reoLT, reoGT, reoLE, reoGE, reoEQ, reoNE, reoXPwrY, reoFlip
};

enum RFuncs {
	refCountOf,
	refCopyBits
};

BRef<RSymbol> REvalOperation(ResourceParserState* parser, REvalOp op,
							 RSymbol* left, RSymbol* right);

class REval
{
public:
	virtual ~REval()
	{
	}
	
	virtual BRef<RSymbol> Evaluate(ResourceParserState* parser) = 0;
	
	REvalType Type() const		{ return fType; }
	
protected:
	REval(REvalType type)
		: fType(type)
	{
	}

private:
	REvalType fType;
};

class RBinaryOp : public REval
{
public:
	RBinaryOp(REval* a, REval* b, REvalOp op)
		: REval(retOperator),
		  fLeft(a), fRight(b), fOperator(op)
	{
	}
	virtual ~RBinaryOp()
	{
		delete fLeft;
		delete fRight;
	}
	
	virtual BRef<RSymbol> Evaluate(ResourceParserState* parser);

private:
	REval *fLeft, *fRight;
	REvalOp fOperator;
};

class RUnaryOp : public REval
{
public:
	RUnaryOp(REval* a, REvalOp op)
		: REval(retOperator),
		  fLeft(a), fOperator(op)
	{
	}
	virtual ~RUnaryOp()
	{
		delete fLeft;
	}
	
	virtual BRef<RSymbol> Evaluate(ResourceParserState* parser);

private:
	REval *fLeft;
	REvalOp fOperator;
};

class RValueOp : public REval
{
public:
	RValueOp(RSymbol* value)
		: REval(retValue),
		  fValue(value)
	{
	}
	virtual ~RValueOp()
	{
	}
	
	virtual BRef<RSymbol> Evaluate(ResourceParserState* parser);

private:
	BRef<RSymbol> fValue;
};

}	// namespace BPrivate

using namespace BPrivate;

#endif
