/*	$Id: DExprParser.h,v 1.2 1999/05/05 19:48:37 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/15/99 10:46:03
*/

#ifndef DEXPRPARSER_H
#define DEXPRPARSER_H

class DExprParser
{
  public:
	DExprParser(const char *expr)
		: fExpr(expr)
	{}
	
	DVariable* Parse(const DStackFrame& frame);
	
  private:
	
	enum DPToken
	{
		tokEnd = 256,
		tokIdentifier,
		tokVoid,
		tokChar,
		tokInt,
		tokLong,
		tokHex,
		tokFloat,
		tokDouble,
		tokSigned,
		tokUnsigned,
		tokConst,
		tokThis,
		tokLAnd,
		tokLOr,
		tokSHL,
		tokSHR,
		tokGE,
		tokLE,
		tokEQ,
		tokNE,
		tokPointer,
		tokColonColon,
		tokPlusPlus,
		tokMinusMinus
	};
	
	int Lex();
	void Restart(int& start, int& state);
	void Match(int token);
	
	void Expression();
	void ConditionalExpression();
	void LogicalOrExpression();
	void LogicalAndExpression();
	void InclusiveOrExpression();
	void ExclusiveOrExpression();
	void AndExpression();
	void EqualityExpression();
	void RelationalExpression();
	void ShiftExpression();
	void AdditiveExpression();
	void MultiplicativeExpression();
	void CastExpression();
	void UnaryExpression();
	void PostFixExpression();
	void PrimaryExpression();
	void TypeName();
	void AbstractDeclarator();
	void PointerOperator();
	void TypeSpecifierList();
	void SimpleTypeName();
	void QualifiedTypeName();
	void CompleteClassName();
	void QualifiedClassName();
	void QualifiedName();
	void ClassName();
	void TypedefName();
	void Name();
	
	string fExpr;
	string::iterator fExprIter;
	
	const DStackFrame *fFrame;
	
	const DStackFrame& Frame() const { return *fFrame; }

		// for the lexical analyzer:
	int fLookahead;
	string fToken;
	uint64 fValueInt;
	double fValueFloat;
	ptr_t fAddr;
	
	DType				*fType;
	DVariable				*fVariable;
	
	DVariable* Promote(DVariable* var, DType *type);
	uint32 GetIntValue();					// return the current value casted to an uint32
};

#endif
