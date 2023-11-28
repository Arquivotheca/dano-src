/************************************************************************/
/*	Project...:	C++ and ANSI-C Compiler Environment						*/
/*	Name......:	Unmangle.c												*/
/*	Purpose...: C++ name unmangler										*/
/*  Copyright.: ©Copyright 1995 by metrowerks inc. All rights reserved. */
/************************************************************************/

#include "Unmangle.h"
#include <string.h>
#include <setjmp.h>

#if __MWERKS__>=0x1000
	#if !__option(bool)
		#define true			1
		#define false			0
		typedef unsigned char	bool;
	#endif
#elif __MWERKS__
	#define true	1
	#define false	0
	typedef unsigned char bool;
#endif

#define MAXUSERID	256						//	max length of a user name

typedef struct Buffer {						//	string buffer
	char		*append;					//	pointer to append position					
	size_t		free;						//	number of free bytes in buffer
}	Buffer;

static jmp_buf		unmangle_jmpbuf;		//	unmangler error return buffer

	//	forward function declaration

static const char *Unmangle_Type(Buffer *buffer,const char *mangled_name);

/****************************************************************/
/* Purpose..: Recover after parse error							*/
/* Input....: ---												*/
/* Returns..: --- (does not return)								*/
/****************************************************************/
static void Unmangle_Error(void)
{
	longjmp(unmangle_jmpbuf,1);
}

/****************************************************************/
/* Purpose..: Initialize the buffer structure					*/
/* Input....: pointer to buffer									*/
/* Input....: pointer to character buffer						*/
/* Input....: size of character buffer							*/
/* Returns..: ---												*/
/****************************************************************/
static void Unmangle_BufferInit(Buffer *buffer,char *charbuf,size_t size)
{
	buffer->append=charbuf; buffer->free=size-1;	//	save one byte for 0 termination
}

/****************************************************************/
/* Purpose..: Append a single character to buffer string		*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: the character										*/
/* Returns..: ---												*/
/****************************************************************/
static void Unmangle_BufferAppendChar(Buffer *buffer,char c)
{
	if(buffer && buffer->free>0) { *buffer->append++=c; buffer->free--; }
}

/****************************************************************/
/* Purpose..: Append a single character to buffer string		*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: pointer to string									*/
/* Returns..: ---												*/
/****************************************************************/
static void Unmangle_BufferAppendString(Buffer *buffer,const char *string)
{
	if(buffer)
	{
		size_t	size=strlen(string);
	
		if(buffer->free<size) size=buffer->free;
		memcpy(buffer->append,string,size);
		buffer->append+=size; buffer->free-=size;
	}
}

/****************************************************************/
/* Purpose..: Terminate a buffer string							*/
/* Input....: pointer to buffer									*/
/* Returns..: ---												*/
/****************************************************************/
static void Unmangle_BufferTerminate(Buffer *buffer)
{
	*buffer->append=0; buffer->free=0;
}

/****************************************************************/
/* Purpose..: Check if mangled_name is a special name			*/
/* Input....: pointer to buffer									*/
/* Input....: pointer to mangled name							*/
/* Returns..: pointer to mangled name after special name or 0L	*/
/****************************************************************/
static const char *Unmangle_SpecialName(Buffer *buffer,const char *mangled_name)
{
	const char	*name;
	bool		is_conversion=false;

	switch(*mangled_name++)
	{
	case 'a':
		switch(*mangled_name++)
		{
		case 'a':
			switch(*mangled_name++)
			{
			case '_':	mangled_name--; name="operator &&"; break;
			case 'd':	name="operator &="; break;
			default:	return 0;
			}
			break;

		case 'd':
			switch(*mangled_name++)
			{
			case '_':	mangled_name--; name="operator &"; break;
			case 'v':	name="operator /="; break;
			default:	return 0;
			}
			break;

		case 'e':
			switch(*mangled_name++)
			{
			case 'r':	name="operator ^="; break;
			default:	return 0;
			}
			break;

		case 'l':
			switch(*mangled_name++)
			{
			case 's':	name="operator <<="; break;
			default:	return 0;
			}
			break;

		case 'm':
			switch(*mangled_name++)
			{
			case 'd':	name="operator %="; break;
			case 'i':	name="operator -="; break;
			case 'u':	name="operator *="; break;
			default:	return 0;
			}
			break;

		case 'o':
			switch(*mangled_name++)
			{
			case 'r':	name="operator |="; break;
			default:	return 0;
			}
			break;

		case 'p':
			switch(*mangled_name++)
			{
			case 'l':	name="operator +="; break;
			default:	return 0;
			}
			break;

		case 'r':
			switch(*mangled_name++)
			{
			case 's':	name="operator >>="; break;
			default:	return 0;
			}
			break;

		case 's':	name="operator ="; break;
		default:	return 0;
		}
		break;

	case 'c':
		switch(*mangled_name++)
		{
		case 'l':	name="operator ()"; break;
		case 'm':	name="operator ,"; break;
		case 'o':	name="operator ~"; break;
		case 't':	name="!"; break;
		default:	return 0;
		}
		break;

	case 'd':
		switch(*mangled_name++)
		{
		case 'l':	name="operator delete"; break;
		case 't':	name="~"; break;
		case 'v':	name="operator /"; break;
		default:	return 0;
		}
		break;

	case 'e':
		switch(*mangled_name++)
		{
		case 'q':	name="operator =="; break;
		case 'r':	name="operator ^"; break;
		default:	return 0;
		}
		break;

	case 'g':
		switch(*mangled_name++)
		{
		case 'e':	name="operator >="; break;
		case 't':	name="operator >"; break;
		default:	return 0;
		}
		break;

	case 'l':
		switch(*mangled_name++)
		{
		case 'e':	name="operator <="; break;
		case 's':	name="operator <<"; break;
		case 't':	name="operator <"; break;
		default:	return 0;
		}
		break;

	case 'm':
		switch(*mangled_name++)
		{
		case 'd':	name="operator %"; break;
		case 'i':	name="operator -"; break;
		case 'l':	name="operator *"; break;
		case 'm':	name="operator --"; break;
		default:	return 0;
		}
		break;

	case 'n':
		switch(*mangled_name++)
		{
		case 'e':	name="operator !="; break;
		case 't':	name="operator !"; break;
		case 'w':	name="operator new"; break;
		default:	return 0;
		}
		break;

	case 'o':
		switch(*mangled_name++)
		{
		case 'r':	name="operator |"; break;
		case 'o':	name="operator ||"; break;
		case 'p':
			Unmangle_BufferAppendString(buffer,"operator ");
			mangled_name=Unmangle_Type(buffer,mangled_name);
			is_conversion=true; break;

		default:	return 0;
		}
		break;

	case 'p':
		switch(*mangled_name++)
		{
		case 'l':	name="operator +"; break;
		case 'p':	name="operator ++"; break;
		default:	return 0;
		}
		break;

	case 'r':
		switch(*mangled_name++)
		{
		case 'f':	name="operator ->"; break;
		case 's':	name="operator >>"; break;
		case 'm':	name="operator ->*"; break;
		default:	return 0;
		}
		break;

	case 'v':
		switch(*mangled_name++)
		{
		case 'c':	name="operator []"; break;
		default:	return 0;
		}
		break;

	default:
		return 0;
	}

	if(!is_conversion)
	{
		if(*mangled_name==0)
		{
			switch(*name)
			{
			case '!':	name="constructor declaration"; break;
			case '~':	name="destructor declaration"; break;
			}
		}
		else if(*mangled_name++!='_' || *mangled_name++!='_') return 0;

		Unmangle_BufferAppendString(buffer,name);
	}
	else if(*mangled_name++!='_' || *mangled_name++!='_') return 0;

	Unmangle_BufferTerminate(buffer);
	return mangled_name;
}

/****************************************************************/
/* Purpose..: Append a template parameter list					*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: pointer to template parameter list				*/
/* Returns..: pointer to first char after template param list	*/
/****************************************************************/
static const char *Unmangle_TemplateParams(Buffer *buffer,const char *param_list)
{
	Unmangle_BufferAppendChar(buffer,'<');
	for(;;)
	{
		switch(*param_list)
		{
		case '=':
		case '&':
			if(*param_list++=='&') Unmangle_BufferAppendChar(buffer,'&');
			for(;;)
			{
				switch(*param_list)
				{
				case 0:
					Unmangle_Error();

				case ',':
				case '>':
					break;

				default:
					Unmangle_BufferAppendChar(buffer,*param_list++);
					continue;
				}
				break;
			}
			break;

		default:
			param_list=Unmangle_Type(buffer,param_list); break;
		}
		if(*param_list=='>') { param_list++; break; }
		if(*param_list++!=',') Unmangle_Error();
		Unmangle_BufferAppendString(buffer,", ");
	}
	Unmangle_BufferAppendChar(buffer,'>');
	return param_list;
}

/****************************************************************/
/* Purpose..: Append a class name								*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: pointer to class name								*/
/* Input....: length of class name								*/
/* Input....: true: unmangle template parameters				*/
/* Returns..: pointer to first char after class name			*/
/****************************************************************/
static const char *Unmangle_ClassName(Buffer *buffer,const char *class_name,size_t length,bool template_param)
{
	const char	*cptr;
	char		c;

	while(length-->0)
	{
		if((c=*class_name++)==0) Unmangle_Error();
		if(c=='<')
		{
			cptr=Unmangle_TemplateParams(template_param?buffer:0,class_name);
			if((length-=cptr-class_name)==0) return cptr;
			Unmangle_Error();
		}
		else Unmangle_BufferAppendChar(buffer,c);
	}
	return class_name;
}

/****************************************************************/
/* Purpose..: Append a (qualified) class name					*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: pointer to class name								*/
/* Input....: true: append constructor/destructor name			*/
/* Input....: true: append '~' before last class name			*/
/* Returns..: pointer to first char after class name			*/
/****************************************************************/
static const char *Unmangle_QClassName(Buffer *buffer,const char *class_name,bool con_des,bool is_destr)
{
	const char	*lastname;
	size_t		n,lastn;
	int			i,nqual;
	char		c;

	if(*class_name=='Q')
	{
		class_name++; nqual=*class_name++-'0';
		if(nqual<1 || nqual>9) Unmangle_Error();
	}
	else nqual=1;

	for(i=0; i<nqual; i++)
	{
		if(i && buffer) Unmangle_BufferAppendString(buffer,"::");
		if((c=*class_name)<'0' || c>'9') Unmangle_Error();
		for(n=0; (c=*class_name)>='0' && c<='9'; class_name++) n=n*10+c-'0';
		if(n<=0) Unmangle_Error();
		lastname=class_name; lastn=n;
		class_name=Unmangle_ClassName(buffer,class_name,n,true);
	}

	if(con_des && buffer)
	{
		if(is_destr)	Unmangle_BufferAppendString(buffer,"::~");
		else			Unmangle_BufferAppendString(buffer,"::");
		(void)Unmangle_ClassName(buffer,lastname,lastn,false);
	}

	return class_name;
}

/****************************************************************/
/* Purpose..: Unmangle modifiers								*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: pointer to mangled name							*/
/* Returns..: pointer to first char after modifiers				*/
/****************************************************************/
static const char *Unmangle_Modifiers(Buffer *buffer,const char *mangled_name)
{
	const char	*modifier;
	bool		is_first;

	for(is_first=true;;is_first=false,mangled_name++)
	{
		switch(*mangled_name)
		{
		case 'U':	modifier="unsigned"; break;
		case 'C':	modifier="const"; break;
		case 'V':	modifier="volatile"; break;
		case 'S':	modifier="signed"; break;
		default:	return mangled_name;
		}
		if(!is_first) Unmangle_BufferAppendChar(buffer,' ');
		Unmangle_BufferAppendString(buffer,modifier); 
	}
}

/****************************************************************/
/* Purpose..: Unmangle a pointer type							*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: pointer to mangled name							*/
/* Returns..: pointer to first char after type					*/
/****************************************************************/
static const char *Unmangle_PointerType(Buffer *buffer,const char *mangled_name)
{
	const char	*modifiers;

	modifiers=mangled_name; mangled_name=Unmangle_Modifiers(0L,mangled_name);
	if(modifiers==mangled_name) modifiers=0L;

	switch(*mangled_name)
	{
	case 'P':
		mangled_name=Unmangle_PointerType(buffer,mangled_name+1);
		if(buffer)
		{
			Unmangle_BufferAppendChar(buffer,'*');
			if(modifiers) (void)Unmangle_Modifiers(buffer,modifiers);
		}
		return mangled_name;

	case 'R':
		mangled_name=Unmangle_PointerType(buffer,mangled_name+1);
		if(buffer)
		{
			Unmangle_BufferAppendChar(buffer,'&');
			if(modifiers) (void)Unmangle_Modifiers(buffer,modifiers);
		}
		return mangled_name;

	case 'M':
		mangled_name=Unmangle_QClassName(buffer,mangled_name+1,false,false);
		if(buffer)
		{
			Unmangle_BufferAppendString(buffer,"::*");
			if(modifiers) (void)Unmangle_Modifiers(buffer,modifiers);
		}
		return mangled_name;
	}
	return mangled_name;
}

/****************************************************************/
/* Purpose..: Unmangle an array type							*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: pointer to mangled name							*/
/* Input....: pointer to pointer type name (or 0L)				*/
/* Returns..: pointer to first char after type					*/
/****************************************************************/
static const char *Unmangle_ArrayType(Buffer *buffer,const char *mangled_name,const char *pointertype)
{
	const char	*f_typename;

	f_typename=++mangled_name;
	for(;;)
	{
		switch(*mangled_name++)
		{
		case '0':case '1':case '2':case '3':case '4':
		case '5':case '6':case '7':case '8':case '9':
			continue;

		case '_':
			if(*mangled_name=='A') { mangled_name++; continue; }
			break;

		default:
			Unmangle_Error();
		}
		break;
	}

	mangled_name=Unmangle_Type(buffer,mangled_name);

	if(buffer)
	{
		if(pointertype)
		{
			Unmangle_BufferAppendString(buffer," (");
			Unmangle_PointerType(buffer,pointertype);
			Unmangle_BufferAppendChar(buffer,')');
		}
		Unmangle_BufferAppendChar(buffer,'[');
		for(;;)
		{
			switch(*f_typename)
			{
			case '0':case '1':case '2':case '3':case '4':
			case '5':case '6':case '7':case '8':case '9':
				Unmangle_BufferAppendChar(buffer,*f_typename++);
				continue;

			case '_':
				if(f_typename[1]=='A')
				{
					Unmangle_BufferAppendString(buffer,"][");
					f_typename+=2; continue;
				}
				break;

			default:
				Unmangle_Error();
			}
			break;
		}
		Unmangle_BufferAppendChar(buffer,']');
	}

	return mangled_name;
}

/****************************************************************/
/* Purpose..: Unmangle a function type							*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: pointer to mangled name							*/
/* Input....: pointer to pointer type name (or 0L)				*/
/* Returns..: pointer to first char after type					*/
/****************************************************************/
static const char *Unmangle_FunctionType(Buffer *buffer,const char *mangled_name,const char *pointertype)
{
	const char	*f_typename;
	
	f_typename=++mangled_name;
	while(*mangled_name!='_') mangled_name=Unmangle_Type(0L,mangled_name);
	++mangled_name;
	mangled_name=Unmangle_Type(buffer,mangled_name);
	if(buffer)
	{
		if(pointertype)
		{
			Unmangle_BufferAppendString(buffer," (");
			Unmangle_PointerType(buffer,pointertype);
			Unmangle_BufferAppendChar(buffer,')');
		}
		Unmangle_BufferAppendChar(buffer,'(');
		if(*f_typename!='_') for(;;)
		{
			f_typename=Unmangle_Type(buffer,f_typename);
			if(*f_typename=='_') break;
			Unmangle_BufferAppendString(buffer,", ");
		}
		Unmangle_BufferAppendChar(buffer,')');
	}
	return mangled_name;
}

/****************************************************************/
/* Purpose..: Unmangle a pointer-to-member type					*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: pointer to mangled name							*/
/* Input....: pointer to modifiers (or 0L)						*/
/* Input....: one of: { '*', '&', 'M' }							*/
/* Returns..: pointer to first char after type					*/
/****************************************************************/
static const char *Unmangle_Pointer(Buffer *buffer,const char *mangled_name,const char *modifiers,char refptr)
{
	const char	*pointername;
	const char	*f_typename;

	pointername=mangled_name;
	f_typename=Unmangle_PointerType(0L,pointername);
	switch(f_typename[0])
	{
	case 'A':
		if(modifiers) pointername=modifiers;
		return Unmangle_ArrayType(buffer,f_typename,pointername);

	case 'F':
		if(modifiers) pointername=modifiers;
		return Unmangle_FunctionType(buffer,f_typename,pointername);
	}

	if(refptr=='M')
	{
		mangled_name=Unmangle_QClassName(0L,pointername+1,false,false);
		mangled_name=Unmangle_Type(buffer,mangled_name);
		if(buffer)
		{
			Unmangle_BufferAppendChar(buffer,' ');
			(void)Unmangle_QClassName(buffer,pointername+1,false,false);
			Unmangle_BufferAppendString(buffer,"::*");
			if(modifiers) (void)Unmangle_Modifiers(buffer,modifiers);
		}
	}
	else
	{
		mangled_name=Unmangle_Type(buffer,mangled_name+1);
		if(buffer)
		{
			Unmangle_BufferAppendChar(buffer,refptr);
			if(modifiers) (void)Unmangle_Modifiers(buffer,modifiers);
		}
	}

	return mangled_name;
}

/****************************************************************/
/* Purpose..: Unmangle a type									*/
/* Input....: pointer to buffer (or 0L)							*/
/* Input....: pointer to mangled name							*/
/* Returns..: pointer to first char after type					*/
/****************************************************************/
static const char *Unmangle_Type(Buffer *buffer,const char *mangled_name)
{
	const char	*f_typename;
	const char	*modifiers;

	modifiers=mangled_name; mangled_name=Unmangle_Modifiers(0L,mangled_name);
	if(modifiers==mangled_name) modifiers=0L;

	switch(*mangled_name)
	{
	case 'b':	f_typename="bool"; break;
	case 'v':	f_typename="void"; break;
	case 'c':	f_typename="char"; break;
	case 's':	f_typename="short"; break;
	case 'i':	f_typename="int"; break;
	case 'l':	f_typename="long"; break;
	case 'x':	f_typename="long long"; break;
	case 'f':	f_typename="float"; break;
	case 'D':	f_typename="short double"; break;
	case 'd':	f_typename="double"; break;
	case 'r':	f_typename="long double"; break;

	case 'P':	return Unmangle_Pointer(buffer,mangled_name,modifiers,'*');
	case 'R':	return Unmangle_Pointer(buffer,mangled_name,modifiers,'&');
	case 'M':	return Unmangle_Pointer(buffer,mangled_name,modifiers,'M');
	case 'A':	return Unmangle_ArrayType(buffer,mangled_name,0L);
	case 'F':	return Unmangle_FunctionType(buffer,mangled_name,0L);

	case '0':case '1':case '2':case '3':case '4':
	case '5':case '6':case '7':case '8':case '9':
	case 'Q':	//	member function / variable
		if(modifiers)
		{
			(void)Unmangle_Modifiers(buffer,modifiers);
			Unmangle_BufferAppendChar(buffer,' ');
		}
		return Unmangle_QClassName(buffer,mangled_name,false,false);

	default:	Unmangle_Error();
	}

	if(modifiers)
	{
		(void)Unmangle_Modifiers(buffer,modifiers);
		Unmangle_BufferAppendChar(buffer,' ');
	}
	Unmangle_BufferAppendString(buffer,f_typename);
	return mangled_name+1;
}

/****************************************************************/
/* Purpose..: Unmangle a function argument list					*/
/* Input....: pointer to buffer									*/
/* Input....: pointer to mangled name							*/
/* Returns..: ---												*/
/****************************************************************/
static void Unmangle_FuncArgList(Buffer *buffer,const char *mangled_name)
{
	bool		is_volatile,is_const;

	is_volatile=is_const=false;
	if(*mangled_name=='C') { mangled_name++; is_const=true; }
	if(*mangled_name=='V') { mangled_name++; is_volatile=true; }
	if(*mangled_name++!='F') Unmangle_Error();

	Unmangle_BufferAppendChar(buffer,'(');
	if(mangled_name[0]!='v' || mangled_name[1]!=0)
	for(;;)
	{
		if(mangled_name[0]=='e')
		{
			if(mangled_name[1]!=0) Unmangle_Error();
			Unmangle_BufferAppendString(buffer,"...");
			break;
		}
		mangled_name=Unmangle_Type(buffer,mangled_name);
		if(mangled_name[0]==0) break;
		Unmangle_BufferAppendChar(buffer,',');
	}
	Unmangle_BufferAppendChar(buffer,')');
	if(is_const) Unmangle_BufferAppendString(buffer," const");
	if(is_volatile) Unmangle_BufferAppendString(buffer," volatile");
	Unmangle_BufferTerminate(buffer);
}

/****************************************************************/
/* Purpose..: Unmangle a C++ class/template name				*/
/* Input....: pointer to mangled name							*/
/* Input....: pointer to unmangled name buffer					*/
/* Input....: number of bytes in buffer							*/
/* Returns..: ---												*/
/****************************************************************/
void MWUnmangleClassName(const char *mangled_name,char *unmangled_name,size_t buffersize)
{
	Buffer		outbuffer;
	const char	*cptr;
	bool		is_templ;

	if(mangled_name[0]=='Q' && mangled_name[1]>='1' && mangled_name[1]<='9')
	{	//	qualifed class name?
		if(!setjmp(unmangle_jmpbuf))
		{	//	get variable/function name
			Unmangle_BufferInit(&outbuffer,unmangled_name,buffersize);
			(void)Unmangle_QClassName(&outbuffer,mangled_name,false,false);
			Unmangle_BufferTerminate(&outbuffer); return;
		}
	}

	for(cptr=mangled_name,is_templ=false;; cptr++)
	{
		switch(*cptr)
		{
		case 0:		break;
		case '<':	is_templ=true; continue;
		default:	continue;
		}
		break;
	}

	if(is_templ && !setjmp(unmangle_jmpbuf))
	{	//	get variable/function name
		Unmangle_BufferInit(&outbuffer,unmangled_name,buffersize);
		(void)Unmangle_ClassName(&outbuffer,mangled_name,cptr-mangled_name,true);
		Unmangle_BufferTerminate(&outbuffer); return;
	}

	//	return unmangled name
	strncpy(unmangled_name,mangled_name,buffersize);
	unmangled_name[buffersize-1]=0;
}

/****************************************************************/
/* Purpose..: Unmangle a C++ name								*/
/* Input....: pointer to mangled name							*/
/* Input....: pointer to unmangled name buffer					*/
/* Input....: number of bytes in buffer							*/
/* Returns..: ---												*/
/****************************************************************/
extern void MWUnmangle(const char *mangled_name,char *unmangled_name,size_t buffersize)
{
	switch(mangled_name[0])
	{
	case '.':
		mangled_name++; break;			//	strip PowerPC function prefix "."

	case '_':
		switch(mangled_name[1])
		{
		case '@':
		case '#':
		case '%':
			mangled_name+=2; break;		//	strip CFM68K function prefix "_@" / "_#" / "_%" 
		}
		break;
	}

	if(!setjmp(unmangle_jmpbuf))
	{	//	get variable/function name
		Buffer		namebuffer,outbuffer;
		char		name[MAXUSERID];
		const char	*parse_ptr;
		char		c;

		parse_ptr=mangled_name;
		Unmangle_BufferInit(&namebuffer,name,MAXUSERID);
		Unmangle_BufferInit(&outbuffer,unmangled_name,buffersize);

		if(parse_ptr[0]!='_' || parse_ptr[1]!='_' || (parse_ptr=Unmangle_SpecialName(&namebuffer,parse_ptr+2))==0)
		{	//	get regular variable/function name
			bool	has_type=false;

			for(parse_ptr=mangled_name; *parse_ptr=='_'; parse_ptr++)
			{	//	skip all leading '_'s
				Unmangle_BufferAppendChar(&namebuffer,'_'); 
			}
			while((c=*parse_ptr++)!=0)
			{
				if(c=='_' && *parse_ptr=='_')
				{	// "__" terminates name
					while(*++parse_ptr=='_')
					{	//	for name like test_()
						Unmangle_BufferAppendChar(&namebuffer,'_');
					}
					has_type=true; break;
				}
				Unmangle_BufferAppendChar(&namebuffer,c);
			}
			Unmangle_BufferTerminate(&namebuffer);
			if(!has_type)
			{
				Unmangle_BufferAppendString(&outbuffer,name);
				Unmangle_BufferTerminate(&outbuffer); return;
			}
		}

		switch(*parse_ptr)
		{
		case 'F':	//	plain function
			if(name[1]==0 && (name[0]=='!' || name[0]=='~')) break;	//	error
			Unmangle_BufferAppendString(&outbuffer,name);
			Unmangle_FuncArgList(&outbuffer,parse_ptr); return;

		case '0':case '1':case '2':case '3':case '4':
		case '5':case '6':case '7':case '8':case '9':
		case 'Q':	//	member function / variable
			if(name[1]==0 && (name[0]=='!' || name[0]=='~'))
			{
				parse_ptr=Unmangle_QClassName(&outbuffer,parse_ptr,true,name[0]=='~');
			}
			else
			{
				parse_ptr=Unmangle_QClassName(&outbuffer,parse_ptr,false,false);
				Unmangle_BufferAppendString(&outbuffer,"::");
				Unmangle_BufferAppendString(&outbuffer,name);
				if(*parse_ptr==0) { Unmangle_BufferTerminate(&outbuffer); return; }
			}
			Unmangle_FuncArgList(&outbuffer,parse_ptr); return;
		}
	}

	//	return unmangled name
	strncpy(unmangled_name,mangled_name,buffersize);
	unmangled_name[buffersize-1]=0;
}
