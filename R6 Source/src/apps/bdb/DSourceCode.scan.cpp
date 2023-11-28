/*	$Id: DSourceCode.scan.cpp,v 1.3 1999/02/03 08:30:03 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/19/98 19:30:25
*/

#include "bdb.h"
#include "DMessages.h"
#include "DSourceCode.h"

#include <MenuItem.h>
#include <ctype.h>

const int kMaxNameSize = 256;

inline bool isidentf(char c)	
{
	return (isalpha(c) || ((c) == '_'));
}

inline bool isident(char c)	
{
	return (isalnum(c) || ((c) == '_'));
}

bool is_template(const char *text);
const char *comment(const char *text, bool strippp = true);
const char *parens(const char *text, char open);
const char *skip(const char *text, char ch);
const char *skip_ne(const char *text, char ch);
const char *preprocessor(const char *text, int offset, BList& list);
const char *ident(const char *text, int offset, BList& list);
const char *i_extern(const char *text);
void parse(const char *text);

const char *skip(const char *text, char ch)
{
	while (*text)
	{
		if (*text == ch)
		{
			text++;
			break;
		}
		if (*text == '\\' && text[1] != 0)
			text++;
		text++;
	}

	return text;
} /* skip */

const char *skip_ne(const char *text, char ch)
{
	while (*text)
	{
		if (*text == ch)
		{
			text++;
			break;
		}
		text++;
	}

	return text;
} /* skip_ne */

const char *comment(const char *text, bool strippp)
{
	do
	{
		while (isspace (*text))
			text++;
		
		if (text[0] == '/')
		{
			if (text[1] == '*')
			{
				text += 2;

				do
				{
					text = skip_ne(text, '*');
					if (*text == '/')
					{
						text++;
						break;
					}
				}
				while (*text != '\0');
			}
			else if (text[1] == '/')
			{
				text = skip(text, '\n');
			}
			else
				break;
		}
		else if (*text == '#' && text[1] != '#' && strippp)
			text = skip(text, '\n');
		else
			break;
	}
	while (*text);

	return text;
} /* comment */

const char *parens(const char *text, char open)
{
	int c;
	char close;
	
	switch (open)
	{
		case '(':	close = ')'; break;
		case '{':	close = '}'; break;
		case '[':	close = ']'; break;
		default:	ASSERT(false); return text;
	}

	while (true)
	{
		text = comment(text);

		c = *text++;
		
		if (c == '\'') 
		{
			text = skip(text, '\'');
			continue;
		}
		
		if (c == '"') 
		{
			text = skip(text, '"');
			continue;
		}
		
		if (c == '#') 
		{
			text = skip(text, '\n');
			continue;
		}
		
		if (c == open)
		{
			text = parens(text, open);
			continue;
		}
		
		if (c == close)
			return text;
		
		if (c == '\0')
			return text - 1;
	}
} /* parens */

inline void name_append(const char*& text, char*& name, int& size)
{
	if (size < kMaxNameSize - 1)
	{
		*name++ = *text++;
		size++;
	}
	else
		text++;
} /* name_append */

const char *preprocessor(const char *text, int offset, BList& list)
{
	char nameBuf[kMaxNameSize], *name = nameBuf;
	int size = 0;
	
	while (isspace(*text))
		text++;
	
	if (strncmp(text, "pragma", 6) == 0)
	{
		text += 6;
		
		while (isspace(*text))
			text++;
			
		if (strncmp(text, "mark", 4) == 0)
		{
			text += 4;

			while (isspace(*text))
				text++;
			
			while (*text && *text != '\n')
				name_append(text, name, size);

			*name = 0;
			
			if (strcmp(nameBuf, "-") == 0)
				list.AddItem(new BSeparatorItem());
			else
			{
				BMessage *m = new BMessage(kMsgJumpToFunc);
				m->AddInt32("offset", offset);
				list.AddItem(new BMenuItem(nameBuf, m));
			}
		}
	}
	
	return skip(text, '\n');
} /* preprocessor */

const char *i_namespace(const char *text)
{
	if (isidentf(*text)) {
		while (isident(*text)) {
			text++;
		}
	}

	text = comment(text);
	if (*text == '{') {
		text++;
	}
	
	return text;
} /* i_namespace */

const char *i_extern(const char *text)
{
	if (*text == '"')
	{
		text = skip(text + 1, '"');
		text = comment(text);
		
		if (*text == '{')
			return text + 1;
	}
	
	return text;
} /* i_extern */

const char *ident(const char *text, int offset, BList& list)
{
	const char *start = text, *id = start;
	char nameBuf[kMaxNameSize], *name = nameBuf;
	int size = 0;
	bool destructor = false;
	
	while (isident(*text))
		name_append(text, name, size);
	
	*name = 0;

	text = comment(text);
	
	if (strcmp(nameBuf, "extern") == 0) 
	{
		return i_extern(text);
	}
	
	if (strcmp(nameBuf, "namespace") == 0)
	{
		return i_namespace(text);
	}
	
	if (is_template(text))
	{
		name_append(text, name, size);
		
		text = comment(text);
		
		while (isident(*text))
		{
			while (isident(*text))
				name_append(text, name, size);
		
			text = comment(text);
			
			if (*text == ',')
			{
				name_append(text, name, size);
				text = comment(text);
			}
		}

		if (*text == '>')
		{
			name_append(text, name, size);
			
			text = comment(text);
		}
	}
	
	while (*text == ':' && text[1] == ':')
	{
		name_append(text, name, size);
		name_append(text, name, size);
		
		text = comment(text);
		
		id = name;
		
		if (*text == '~')
		{
			name_append(text, name, size);
			text = comment(text);
			destructor = true;
		}
		
		if (isidentf(*text))
			while (isident(*text))
				name_append(text, name, size);
		
		text = comment(text);
		
		if (is_template(text))
		{
			name_append(text, name, size);
			text = comment(text);
			
			while (isident(*text))
			{
				while (isident(*text))
					name_append(text, name, size);
				
				text = comment(text);
				
				if (*text == ',')
				{
					name_append(text, name, size);
					text = comment(text);
				}
			}
			
			if (*text == '>')
			{
				name_append(text, name, size);
				text = comment(text);
			}
		}
	}
	
	if (!destructor && strcmp(id, "operator") == 0)
	{
		if (*text == '(')
			name_append(text, name, size);
		
		text = comment(text);
		
		while (*text != '(' && size < kMaxNameSize)
		{
			if (isidentf(*text))
			{
				while (isident(*text))
					name_append(text, name, size);
			}
			else if (! isspace(*text))
			{
				while (! isspace(*text) && ! isidentf(*text) && *text != '(' &&
							! (*text == '/' && (text[1] == '*' || text[1] == '/')))
					name_append(text, name, size);
			}

			text = comment(text);
		}
	}
	
	*name = 0;
	
	if (*text == '(')
	{
		char match[256];
		long l = std::min((long)255, text - start);
		
		strncpy(match, start, l);
		match[l] = 0;
		
		text = parens(text + 1, '(');
		text = comment(text);
		
		if (*text == ':')
		{
			while (*text != '{' && *text != ';')
				text++;
			if (*text == '{')
			{
				text = parens(text + 1, '{');
				
				BMessage *m = new BMessage(kMsgJumpToFunc);
				m->AddInt32("offset", offset);
				list.AddItem(new BMenuItem(nameBuf, m));
			}
			if (*text == '\n')
				text++;
			return text;
		}
		
		if (*text == ';')
			return text + 1;
		
		if (isidentf(*text) || *text == '{')
		{
			BMessage *m = new BMessage(kMsgJumpToFunc);
			m->AddInt32("offset", offset);
			list.AddItem(new BMenuItem(nameBuf, m));

			text = skip_ne(text, '{');
			text = parens(text, '{');
			if (*text == '\n')
				text ++;
			return text;
		}
	}
	
	return text;
} /* ident */

void DSourceCode::ScanForFunctions(BList& list)
{
	const char *text = fText, *max = text + fLineBreaks.back();
	if (*max != 0)
		return;
	
	while (text < max)
	{
		text = comment(text, false);
		
		switch (*text)
		{
			case 0:
				return;
			case '\'':
				text = skip(text + 1, '\'');
				break;
			case '"':
				text = skip(text + 1, '"');
				break;
			case '(':
			case '{':
			case '[':
				text = parens(text + 1, *text);
				break;
			case '#':
				text = preprocessor(text + 1, text + 1 - fText, list);
				break;
			default:
				if (isidentf(*text))
					text = ident(text, text - fText, list);
				else
					text++;
				break;
		}
	}
} /* ScanForFunctions */

bool is_template(const char *text)
{
	if(*text == '<')
	{
		const char *p = comment(text + 1);
		return isidentf(*p);
	}

	return false;
} /* is_template */
