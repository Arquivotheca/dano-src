/*	$Id: DExprParser.cpp,v 1.3 1999/05/11 21:31:04 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/15/99 10:46:09
*/

#include "bdb.h"
#include "DSymWorld.h"
#include "DVariable.h"
#include "DExprParser.h"
#include "endian.h"
#include "DBasicType.h"
#include "DStackFrame.h"
#include "dwarf2.h"
#include <ctype.h>
#include <typeinfo>

DVariable* DExprParser::Parse(const DStackFrame& frame)
{
	fExprIter = fExpr.begin();
	fFrame = &frame;
	
	fLookahead = Lex();
	fVariable = NULL;
	
	Expression();
	
	FailNilMsg(fVariable, "Not a valid expression");
	fVariable->SetName(fExpr);
	
	return fVariable;
} // DExprParser::Parse

#define UNDEFINED				-1
#define MAXTOKENLENGTH	255

void DExprParser::Restart(int& start, int& state)
{
	switch(start)
	{
		case 0:		start = 1;	break;
		case 1:		start = 10;	break;
		case 10:		start = 20;	break;
		case 20:		start = 30;	break;
		case 30:		start = 40;	break;
		default:		THROW(("Impossible start state"));
	}
	state = start;
} /* DExprParser::Restart */

#define RESTART			{ Restart(start, state); fExprIter = ss; *t = 0; te = t; continue; }
#define RETRACT			{ fExprIter--, *--te = 0; }
#define GETNEXTCHAR	(ch = *te++ = (fExprIter != fExpr.end() ? *fExprIter++ : 0), *te = 0, ch)

static inline bool ishexdigit(char ch)
{
	char lch = tolower(ch);
	return (isdigit(lch) || ((lch >= 'a') && (lch <= 'f'))) ? true : false;
}

int DExprParser::Lex()
{
	char ch, buf[MAXTOKENLENGTH + 1], *t = buf, *te, *tm;
	string::iterator ss;
	int token;
	int state, start;

	ss = fExprIter;
	*t = 0;
	te = t;
	tm = t + MAXTOKENLENGTH;
	
	token = UNDEFINED;
	state = start = 0;
	
	while (token == UNDEFINED)
	{
		if (te >= tm)
			THROW(("Token too long"));

		switch (state)
		{
		
	// skip white space
			case 0:
				if (isspace(GETNEXTCHAR))
				{
					ss++;
					te = t;
					continue;
				}
				else if (ch == 0)
					token = tokEnd;
				else
					RESTART;
				break;

	// match an identifier			
			case 1:
				if (isalpha(GETNEXTCHAR) || ch == '$' || ch == '_')
					state = 2;
				else
					RESTART;
				break;
			
			case 2:
				if (!isalnum(GETNEXTCHAR) && ch != '$' && ch != '_')
					state = 3;
				break;
			
			case 3:
				RETRACT;
				token = tokIdentifier;
				break;
			
	// match a floating point with all its bells and whistles
			case 10:
				if (isdigit(GETNEXTCHAR))
					state = 11;
				else
					RESTART;
				break;
			
			case 11:
				if (isdigit(GETNEXTCHAR))
					;
				else if (ch == '.')
					state = 12;
				else if (ch == 'e')
					state = 14;
				else
					RESTART;
				break;
			
			case 12:
				if (isdigit(GETNEXTCHAR))
					state = 13;
				else
					RESTART;
				break;
			
			case 13:
				if (isdigit(GETNEXTCHAR))
					;
				else if (ch == 'e')
					state = 14;
				else
					RESTART;
				break;
			
			case 14:
				if (GETNEXTCHAR == '+' || ch == '-')
					state = 15;
				else if (isdigit(ch))
					state = 16;
				else
					RESTART;
				break;
			
			case 15:
				if (isdigit(GETNEXTCHAR))
					state = 16;
				else
					RESTART;
				break;
			
			case 16:
				if (!isdigit(GETNEXTCHAR))
					state = 17;
				break;
			
			case 17:
				RETRACT;
				token = tokFloat;
				break;
		
	// match float without an exponent
			case 20:
				if (isdigit(GETNEXTCHAR))
					state = 21;
				else
					RESTART;
				break;
			
			case 21:
				if (isdigit(GETNEXTCHAR))
					;
				else if (ch == '.')
					state = 22;
				else
					RESTART;
				break;
			
			case 22:
				if (isdigit(GETNEXTCHAR))
					state = 23;
				else
					RESTART;
				break;
			
			case 23:
				if (! isdigit(GETNEXTCHAR))
					state = 24;
				break;
			
			case 24:
				RETRACT;
				token = tokFloat;
				break;
			
	// match integer
			case 30:
				if (GETNEXTCHAR == '0')		// possible hex constant?
				{
					if (GETNEXTCHAR == 'x')
						state = 33;		// process hex constant
					else
					{
						RETRACT;		// retract the not-an-x character and process it again as though it were the next digit
						state = 31;
					}
				}
				else if (isdigit(ch))
					state = 31;
				else
					RESTART;
				break;
			
			case 31:
				if (! isdigit(GETNEXTCHAR))
					state = 32;
				break;
			
			case 32:
				RETRACT;
				token = tokInt;
				break;

			case 33:		// first digit of hex constant after 0x -- must be in [0-9a-zA-Z]
				if (!ishexdigit(GETNEXTCHAR))
					THROW(("Illegal hex constant"));
				else
					state = 34;
				break;

			case 34:		// hex digits after first valid -- keep reading until non hex digit found
				if (! ishexdigit(GETNEXTCHAR))
					state = 35;
				break;

			case 35:		// after end of hex constant
				RETRACT;
				token = tokHex;
				break;

	// match rest
			case 40:
				GETNEXTCHAR;
				switch (ch)
				{
					case '<': {
							if (GETNEXTCHAR == '=')
								token = tokLE;
							else
							{
								RETRACT;
								token = '<';
							}
						}
						break;

					case '>': {
							if (GETNEXTCHAR == '=')
								token = tokGE;
							else
							{
								RETRACT;
								token = '>';
							}
						}
						break;

					case '=': {
						if (GETNEXTCHAR != '=')
							THROW(("No lvalue allowed in expression"));
						token = tokEQ;
						break;
					}
					
					case '-':
						if (GETNEXTCHAR == '>')
							token = tokPointer;
						else if (ch == '-')
							token = tokMinusMinus;
						else
						{
							RETRACT;
							token = '-';
						}
						break;
					
					case ':': {
						if (GETNEXTCHAR != ':')
						{
							RETRACT;
							token = ':';
						}
						else
							token = tokColonColon;
						break;
					}
					
					case '+':
						if (GETNEXTCHAR == '+')
							token = tokPlusPlus;
						else
						{
							RETRACT;
							token = '+';
						}
						break;
					
					case '|':
						if (GETNEXTCHAR == '|')
							token = tokLOr;
						else
						{
							RETRACT;
							token = '|';
						}
						break;
					
					case '&':
						if (GETNEXTCHAR == '&')
							token = tokLAnd;
						else
						{
							RETRACT;
							token = '&';
						}
						break;
					
					default:
						token = ch;
						break;
				}
		}
	}
	
	fToken = buf;
	
	char *e;
	
	if (token == tokFloat)
		fValueFloat = strtod(t, &e);
	else if (token == tokInt)
		fValueInt = strtol(t, &e, 10);
	else if (token == tokHex)
	{
		sscanf(t, "0x%Lx", &fValueInt);
		token = tokInt;
	}
	else if (token == tokIdentifier)
	{
		if (fToken == "void")
			token = tokVoid;
		else if (fToken == "char")
			token = tokChar;
		else if (fToken == "int")
			token = tokInt;
		else if (fToken == "long")
			token = tokLong;
		else if (fToken == "float")
			token = tokFloat;
		else if (fToken == "double")
			token = tokDouble;
		else if (fToken == "signed")
			token = tokSigned;
		else if (fToken == "unsigned")
			token = tokUnsigned;
		else if (fToken == "const")
			token = tokConst;
		else if (fToken == "this")
			token = tokThis;
	}
	
	return token;
} // DExprParser::lex

void DExprParser::Match(int token)
{
	if (token == fLookahead)
		fLookahead = Lex();
	else
		THROW(("mismatch"));
} // DExprParser::Match

// #pragma mark -
//	<expression> : <conditional-expression>

void DExprParser::Expression()
{
	ConditionalExpression();
} // DExprParser::Expression

//	<conditional-expression>
//			<logical-or-expression>
//			<logical-or-expression> ? <expression> : <conditional-expression>

void DExprParser::ConditionalExpression()
{
	LogicalOrExpression();
	
	if (fLookahead == '?')
	{
		if (!fVariable->Type()->IsBase())
			THROW(("Illegal operand"));
		
		uint32 c = GetIntValue();
		
		DVariable *r = NULL;
		
		Match ('?');
		
		Expression();
		
		if (c)
			r = fVariable;
		
		Match(':');
		
		ConditionalExpression();
		
		if (r)
			fVariable = r;
	}
} // DExprParser::ConditionalExpression

//	<logical-or-expression> : <logical-and-expression> <logical-or-expression'>
//	<logical-or-expression'> :
//		|| <logical-and-expression>
//		E

void DExprParser::LogicalOrExpression()
{
	LogicalAndExpression();

	if (fLookahead == tokLOr)
	{
		uint32 c = GetIntValue();
	
		while (fLookahead == tokLOr)
		{
			Match(tokLOr);
			LogicalAndExpression();
			c = c || GetIntValue();
		}
		
		fVariable = new DConstVariable("bool", new DIntType(4, false), &c);
	}
} // DExprParser::LogicalOrExpression

//	<logical-and-expression> : <inclusive-or-expression> <logical-and-expression'>
//	<logical-and-expression'> : 
//		&& <inclusive-or-expression>
//		E

void DExprParser::LogicalAndExpression()
{
	InclusiveOrExpression();
	
	if (fLookahead == tokLAnd)
	{
		uint32 c = GetIntValue();
		
		while (fLookahead == tokLAnd)
		{
			Match(tokLAnd);
			InclusiveOrExpression();
			c = c && GetIntValue();
		}

		fVariable = new DConstVariable("bool", new DIntType(4, false), &c);
	}
} // DExprParser::LogicalAndExpression

//	<inclusive-or-expression> : <exclusive-or-expression> <inclusive-or-expression'>
//	<inclusive-or-expression'> :
//		| <exclusive-or-expression>
//		E

void DExprParser::InclusiveOrExpression()
{
	ExclusiveOrExpression();
	
	if (fLookahead == '|')
	{
		uint32 t = GetIntValue();
		
		while (fLookahead == '|')
		{
			Match('|');
			ExclusiveOrExpression();
			t |= GetIntValue();
		}
		
		fVariable = new DConstVariable("int", new DIntType(4, false), &t);
	}
} // DExprParser::InclusiveOrExpression

//	<exclusive-or-expression> : <and-expression> <exclusive-or-expression'>
//	<exclusive-or-expression'> :
//		^ <and-expression>
//		E

void DExprParser::ExclusiveOrExpression()
{
	AndExpression();
	
	if (fLookahead == '^')
	{
		uint32 t = GetIntValue();
		
		while (fLookahead == '^')
		{
			Match('^');
			AndExpression();
			t ^= GetIntValue();
		}
		
		fVariable = new DConstVariable("int", new DIntType(4, false), &t);
	}
} // DExprParser::ExclusiveOrExpression

//	<and-expression> : <equality-expression> <and-expression'>
//	<and-expression'> :
//		& <equality-expression>
//		E

void DExprParser::AndExpression()
{
	EqualityExpression();
	
	if (fLookahead == '&')
	{
		uint32 t = GetIntValue();
		
		while (fLookahead == '&')
		{
			Match('&');
			EqualityExpression();
			t &= GetIntValue();
		}
		
		fVariable = new DConstVariable("int", new DIntType(4, false), &t);
	}
} // DExprParser::AndExpression

//	<equality-expression> : <relational-expression> <equality-expression'>
//	<equality-expression'> :
//		== <relational-expression>
//		!= <relational-expression>
//		E

void DExprParser::EqualityExpression()
{
	RelationalExpression();

	if (fLookahead == tokEQ || fLookahead == tokNE)
	{
		uint32 t = GetIntValue();

		while (fLookahead == tokEQ || fLookahead == tokNE)
		{
			int op = fLookahead;
			Match(fLookahead);
			RelationalExpression();
			uint32 rhs = GetIntValue();
			t = (op == tokEQ)
				? (t == rhs)
				: (t != rhs);
		}
		
		fVariable = new DConstVariable("int", new DIntType(4, false), &t);
	}
} // DExprParser::EqualityExpression

//	<relational-expression> : <shift-expression> <relational-expression'>
//	<relational-expression'> :
//		< <shift-expression>
//		> <shift-expression>
//		<= <shift-expression>
//		>= <shift-expression>
//		E

void DExprParser::RelationalExpression()
{
	ShiftExpression();
	
	if (fLookahead == '<' || fLookahead == '>' || fLookahead == tokLE || fLookahead == tokGE)
	{
		uint32 t = GetIntValue();

		while (fLookahead == '<' || fLookahead == '>' || fLookahead == tokLE || fLookahead == tokGE)
		{
			int op = fLookahead;
			Match(fLookahead);
			ShiftExpression();
			switch (op)
			{
				case '<':		t = (t < GetIntValue()); break;
				case '>':		t = (t > GetIntValue()); break;
				case tokLE:	t = (t <= GetIntValue()); break;
				case tokGE:	t = (t >= GetIntValue()); break;
			}
		}
		
		fVariable = new DConstVariable("int", new DIntType(4, false), &t);
	}
} // DExprParser::RelationalExpression

//	<shift-expression> : <additive-expression> <shift-expression'>
//	<shift-expression'> : 
//		<< <additive-expression>
//		>> <additive-expression>

void DExprParser::ShiftExpression()
{
	AdditiveExpression();

	if (fLookahead == tokSHL || fLookahead == tokSHR)
	{
		uint32 t = GetIntValue();

		while (fLookahead == tokSHL || fLookahead == tokSHR)
		{
			int op = fLookahead;
			Match(fLookahead);
			AdditiveExpression();
			switch (op)
			{
				case tokSHL:	t = (t << GetIntValue()); break;
				case tokSHR:	t = (t >> GetIntValue()); break;
			}
		}
		
		fVariable = new DConstVariable("int", new DIntType(4, false), &t);
	}
} // DExprParser::ShiftExpression

//	<additive-expression> : <multiplicative-expression> <additive-expression'>
//	<additive-expression'> :
//		+ <multiplicative-expression>
//		- <multiplicative-expression>

void DExprParser::AdditiveExpression()
{
	MultiplicativeExpression();
	
	while (fLookahead == '+' || fLookahead == '-')
	{
		int op = fLookahead;
		Match (fLookahead);

		// Save the left argument DVariable for later, since MultiplicativeExpression()
		// will call Lex() and cause fVariable to point to the right-hand DVariable.
		DVariable *var = fVariable;

		int32 left = GetIntValue();

		MultiplicativeExpression();

		int32 right = GetIntValue();
		if (op == '-') right *= -1;		// subtraction == addition of the inverse

		if (fVariable->Type()->IsPointer() && var->Type()->IsBase())
		{
			std::swap(fVariable, var);
			std::swap(left, right);
		}
		
		if (var->Type()->IsPointer() && fVariable->Type()->IsBase())
		{
			int32 offset = right;
			
			DType *t = var->Type()->Deref();
			offset *= t->Size();
			delete t;

			DLocationString loc;
			var->GetLocationString(loc);
			char *p = (char *)&offset;
			loc.push_back(DW_OP_const4s);
			loc.insert(loc.end(), p, p + 4);
			loc.push_back(DW_OP_plus);
			
			fVariable = new DVariable("", var->Type(), loc);
		}
		else
		{
			// straight addition of the left & right values
			int32 sum = left + right;
			fVariable = new DConstVariable("int", new DIntType(4, true), &sum);
		}
	}
} // DExprParser::AdditiveExpression

//	<multiplicative-expression> : <cast-expression> <multiplicative-expression'>
//	<multiplicative-expression'> :
//		* <cast-expression>
//		/ <cast-expression>
//		% <cast-expression>
//		E

void DExprParser::MultiplicativeExpression()
{
	CastExpression();

	while ((fLookahead == '*') || (fLookahead == '/') || (fLookahead == '%'))
	{
		int op = fLookahead;
		Match(fLookahead);

		int32 product = GetIntValue();
		CastExpression();
		int32 rhs = GetIntValue();

		if (op == '*')
			product *= rhs;
		else if (op == '/')
			product /= rhs;
		else		// op == '%'
			product %= rhs;

		fVariable = new DConstVariable("int", new DIntType(4, true), &product);
	}
} // DExprParser::MultiplicativeExpression

//	<cast-expression> :
//		<unary-expression>
//		( <type-name> ) cast-expression

void DExprParser::CastExpression()
{
	UnaryExpression();
} // DExprParser::CastExpression

//	<unary-expression> :
//		<postfix-expression>
//		<unary-operator> <cast-expression>
//		sizeof <unary-expression>
//		sizeof ( <type-name> )
//	<unary-operator> : one of
//		* & + - ! ~

void DExprParser::UnaryExpression()
{
	PostFixExpression();
} // DExprParser::UnaryExpression

//	<postfix-expression> : <primary-expression> <postfix-expression'>
//	<postfix-expression'> :
//		[ <expression> ]
//		. <name>
//		-> <name>
//		E

void DExprParser::PostFixExpression()
{
	PrimaryExpression();
	
	while (true)
	{
		switch (fLookahead)
		{
			case '.':
				if (! (fVariable->Type()->IsStruct() || fVariable->Type()->IsUnion()))
					THROW(("Expected a struct type"));
				Match('.');
				fVariable = fVariable->GetMemberByName(fToken.c_str());
				Match(tokIdentifier);
				break;
			
			case tokPointer:
				if (! fVariable->Type()->IsPointer()) THROW(("Expected pointer type"));
				Match(tokPointer);
				fVariable = (*fVariable->member_begin())->GetMemberByName(fToken.c_str());
				Match(tokIdentifier);
				break;
			
			case '[': {
				DVariable *a = fVariable;
				DType *eType = a->Type()->GetElementType();
				int32 index;
				
				ptr_t addr;
				
				if (a->Type()->IsArray())
					addr = a->GetLocation(Frame());
				else if (a->Type()->IsPointer())
					a->GetValue(Frame(), addr);
				else
					THROW(("Must be array or pointer type"));
				
				Match('[');
				Expression();
				DIntType intType(32, true);
				fVariable = Promote(fVariable, &intType);
				uint32 s = sizeof(index);
				fVariable->GetValue(Frame(), &index, s);
				Match(']');

				index *= eType->Size();

				char *t;

				DLocationString loc;
				loc.push_back(DW_OP_addr);
				t = (char *)&addr;
				loc.insert(loc.end(), t, t + 4);
				loc.push_back(DW_OP_const4s);
				t = (char *)&index;
				loc.insert(loc.end(), t, t + 4);
				loc.push_back(DW_OP_plus);

				fVariable = new DVariable("", eType, loc);
				break;
			}
			
			case '(':
				THROW(("No function calls allowed"));
			
			case tokPlusPlus:
			case tokMinusMinus:
				THROW(("Unsupported operator"));
			
			default:
				return;
		}
	}
} // DExprParser::PostFixExpression

//	<primary-expression> :
//		<literal>
//		this
//		:: identifier
//		:: <qualified-name>
//		( <expression> )
//		<name>
//		<register-name>

void DExprParser::PrimaryExpression()
{
	switch (fLookahead)
	{
		case tokInt:
			fVariable = new DConstVariable("int", new DIntType(8, true), &fValueInt);
			Match(tokInt);
			break;
		
		case tokFloat:
			fVariable = new DConstVariable("float", new DFloatType(8), &fValueFloat);
			Match(tokFloat);
			break;
		
		case tokIdentifier:
			fVariable = Frame().GetVariable(fToken.c_str());
			Match(tokIdentifier);
			break;
		
		case tokThis:
		{
			Match(tokThis);
			fVariable = Frame().GetVariable("this");
			break;
		}
		
//		case tokColonColon:
//			Match(tokColonColon);
//			break;
		
		case '(':
			Match('(');
			Expression();
			Match(')');
			break;
		
		case 0:
			break;
		
		default:
			THROW(("Syntax error"));
	}
} // DExprParser::PrimaryExpression

//	<type-name> : <type-specifier-list> <abstract-declarator>(opt)

void DExprParser::TypeName()
{
} // DExprParser::TypeName

//	<abstract-declarator>:
//		<ptr-operator> <abstract-declarator>(opt)
//		( <abstract-declarator> )

void DExprParser::AbstractDeclarator()
{
} // DExprParser::AbstractDeclarator

//	<ptr-operator> : one of * &

void DExprParser::PointerOperator()
{
} // DExprParser::PointerOperator

//	<type-specifier-list> : <type-specifier> <type-specifier-list>(opt)
//	<type-specifier> : <simple-type-name>

void DExprParser::TypeSpecifierList()
{
} // DExprParser::TypeSpecifierList

//	<simple-type-name> : 
//		<complete-class-name>
//		<qualified-type-name>
//		char
//		short
//		int
//		long
//		signed
//		unsigned
//		float
//		double
//		void

void DExprParser::SimpleTypeName()
{
} // DExprParser::SimpleTypeName

//	<qualified-type-name> :
//		<typedef-name>
//		<class-name> :: <qualified-class-name>

void DExprParser::QualifiedTypeName()
{
} // DExprParser::QualifiedTypeName

//	<complete-class-name> :
//		<qualified-class-name>
//		:: <qualified-class-name>

void DExprParser::CompleteClassName()
{
} // DExprParser::CompleteClassName

//	<qualified-class-name> :
//		<class-name>
//		<class-name> :: <qualified-class-name>

void DExprParser::QualifiedClassName()
{
} // DExprParser::QualifiedClassName

//	<qualified-name> : <qualified-class-name> :: <name>

void DExprParser::QualifiedName()
{
} // DExprParser::QualifiedName

//	<class-name> : identifier

void DExprParser::ClassName()
{
} // DExprParser::ClassName

//	<typedef-name> : identifier

void DExprParser::TypedefName()
{
} // DExprParser::TypedefName

//	<name> : 
//		identifier
//		<qualified-name>

void DExprParser::Name()
{
} // DExprParser::Name

// #pragma mark -
DVariable* DExprParser::Promote(DVariable* var, DType *type)
{
	ASSERT(type->IsBase());
	
	if (typeid(*type) == typeid(*var->Type()) &&
		type->Size() >= var->Type()->Size())
	{
		if (typeid(*type) == typeid(DIntType))
		{
			if (var->Type()->Size() == type->Size())
				return var;

			uint64 v;
			uint32 s;
			string name;
			
			var->GetValue(Frame(), &v, s);
			var->GetName(name);

#if __LITTLE_ENDIAN
// this should be fixed!!!!!!!!!!!!  no signed<->unsigned conversion yet
			return new DConstVariable(name, type->Clone(), &v);
#else
#	error
#endif
		}
	}
	
	if (var->Type()->IsBase() &&
		(var->Type()->Encoding() == encSigned || var->Type()->Encoding() == encUnsigned) &&
		var->Type()->Size() <= type->Size())
	{
		if (var->Type()->Size() == type->Size())
			return var;

		uint64 v;
		uint32 s;
		string name;
		
		var->GetValue(Frame(), &v, s);
		var->GetName(name);

#if __LITTLE_ENDIAN
// this should be fixed!!!!!!!!!!!!  no signed<->unsigned conversion yet
		return new DConstVariable(name, type->Clone(), &v);
#else
#	error
#endif
	}
	
	THROW(("Cannot promote to this type"));
} // DExprParser::Promote

uint32 DExprParser::GetIntValue()
{
	try
	{
		uint32 v, s;
		DIntType intType(32, false);
		
		DVariable *var = Promote(fVariable, &intType);
		s = sizeof(uint32);
		var->GetValue(Frame(), &v, s);
		delete var;
		
		return v;
	}
	catch (...)
	{
		throw;
	}
} // DExprParser::GetIntValue
