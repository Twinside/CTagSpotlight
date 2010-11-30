
/*
*   Copyright (c) 2010, Vincent Berthoux
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for generating tags for Haskell
*   language files.
*/
/*
*   INCLUDE FILES
*/
#include "general.h"	/* must always come first */

#include <string.h>

#include "keyword.h"
#include "entry.h"
#include "options.h"
#include "read.h"
#include "routines.h"
#include "vstring.h"

/* To get rid of unused parameter warning in
 * -Wextra */
#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

typedef enum {
} haskellKind;

static kindOption HaskellKinds[] = {
	{TRUE, 'f', "funt", "Type definition of a function"},
	{TRUE, 'f', "funimplem", "Implementation of function"},
	{TRUE, 't', "Type", "Defintion of a type alias"},
	{TRUE, 'd', "Data", "Definition of a data tpe"},
	{TRUE, 'g', "GADT", "Definition of a generalized abstract data type"},
	{TRUE, 'c', "typeclass", "Typeclass"},
	{TRUE, 'm', "module", "module"},
	{TRUE, 'c', "constructor", "Data constructor"},
	{TRUE, 'a', "accessor", "Accessor"},
};

typedef enum {
    HsDATA,
    HsTYPE,
    HsINSTANCE,
    HsCLASS,
    HsWHERE,
    HsNEWDATA,
    HsMODULE,

	Tok_COMA,	/* ',' */
	Tok_PLUS,	/* '+' */
	Tok_MINUS,	/* '-' */
	Tok_PARL,	/* '(' */
	Tok_PARR,	/* ')' */
	Tok_CurlL,	/* '{' */
	Tok_CurlR,	/* '}' */
	Tok_SQUAREL,	/* '[' */
	Tok_SQUARER,	/* ']' */
	Tok_semi,	/* ';' */
	Tok_dpoint,	/* ':' */
	Tok_Sharp,	/* '#' */
	Tok_Backslash,	/* '\\' */
	Tok_EOL,	/* '\r''\n' */
	Tok_any,

	Tok_EOF	/* END of file */
} haskellKeyword;

typedef haskellKeyword haskellToken;

typedef struct sOBjcKeywordDesc {
	const char *name;
	haskellKeyword id;
} haskellKeywordDesc;


static const haskellKeywordDesc haskellKeywordTable[] = {
	{"typedef", HaskellTYPEDEF},
};

static langType Lang_Haskell;

/*//////////////////////////////////////////////////////////////////
//// lexingInit             */
typedef struct _lexingState {
	vString *name;	/* current parsed identifier/operator */
	const unsigned char *cp;	/* position in stream */
} lexingState;

static void initKeywordHash (void)
{
	const size_t count = sizeof (haskellKeywordTable) / sizeof (haskellKeywordDesc);
	size_t i;

	for (i = 0; i < count; ++i)
	{
		addKeyword (haskellKeywordTable[i].name, Lang_Haskell,
			(int) haskellKeywordTable[i].id);
	}
}

/*//////////////////////////////////////////////////////////////////////
//// Lexing                                     */
static boolean isNum (char c)
{
	return c >= '0' && c <= '9';
}

static boolean isLowerAlpha (char c)
{
	return c >= 'a' && c <= 'z';
}

static boolean isUpperAlpha (char c)
{
	return c >= 'A' && c <= 'Z';
}

static boolean isAlpha (char c)
{
	return isLowerAlpha (c) || isUpperAlpha (c);
}

static boolean isIdent (char c)
{
	return isNum (c) || isAlpha (c) || c == '_' || c == '\'';
}

static boolean isSpace (char c)
{
	return c == ' ' || c == '\t';
}

/* return true if it end with an end of line */
static void eatWhiteSpace (lexingState * st)
{
	const unsigned char *cp = st->cp;
	while (isSpace (*cp))
		cp++;

	st->cp = cp;
}

static void eatString (lexingState * st)
{
	boolean lastIsBackSlash = FALSE;
	boolean unfinished = TRUE;
	const unsigned char *c = st->cp + 1;

	while (unfinished)
	{
		/* end of line should never happen.
		 * we tolerate it */
		if (c == NULL || c[0] == '\0')
			break;
		else if (*c == '"' && !lastIsBackSlash)
			unfinished = FALSE;
		else
			lastIsBackSlash = *c == '\\';

		c++;
	}

	st->cp = c;
}

static void eatComment (lexingState * st)
{
	boolean unfinished = TRUE;
	boolean lastIsStar = FALSE;
	const unsigned char *c = st->cp + 2;

	while (unfinished)
	{
		/* we've reached the end of the line..
		 * so we have to reload a line... */
		if (c == NULL || *c == '\0')
		{
			st->cp = fileReadLine ();
			/* WOOPS... no more input...
			 * we return, next lexing read
			 * will be null and ok */
			if (st->cp == NULL)
				return;
			c = st->cp;
		}
		/* we've reached the end of the comment */
		else if (*c == '/' && lastIsStar)
			unfinished = FALSE;
		else
		{
			lastIsStar = '*' == *c;
			c++;
		}
	}

	st->cp = c;
}

static void readIdentifier (lexingState * st)
{
	const unsigned char *p;
	vStringClear (st->name);

	/* first char is a simple letter */
	if (isAlpha (*st->cp) || *st->cp == '_')
		vStringPut (st->name, (int) *st->cp);

	/* Go till you get identifier chars */
	for (p = st->cp + 1; isIdent (*p); p++)
		vStringPut (st->name, (int) *p);

	st->cp = p;

	vStringTerminate (st->name);
}

/* read the @something directives */
static void readIdentifierHaskellDirective (lexingState * st)
{
	const unsigned char *p;
	vStringClear (st->name);

	/* first char is a simple letter */
	if (*st->cp == '@')
		vStringPut (st->name, (int) *st->cp);

	/* Go till you get identifier chars */
	for (p = st->cp + 1; isIdent (*p); p++)
		vStringPut (st->name, (int) *p);

	st->cp = p;

	vStringTerminate (st->name);
}

/* The lexer is in charge of reading the file.
 * Some of sub-lexer (like eatComment) also read file.
 * lexing is finished when the lexer return Tok_EOF */
static haskellKeyword lex (lexingState * st)
{
	int retType;

	/* handling data input here */
	while (st->cp == NULL || st->cp[0] == '\0')
	{
		st->cp = fileReadLine ();
		if (st->cp == NULL)
			return Tok_EOF;

		return Tok_EOL;
	}

	if (isAlpha (*st->cp))
	{
		readIdentifier (st);
		retType = lookupKeyword (vStringValue (st->name), Lang_ObjectiveC);

		if (retType == -1)	/* If it's not a keyword */
		{
			return HaskellIDENTIFIER;
		}
		else
		{
			return retType;
		}
	}
	else if (*st->cp == '@')
	{
		readIdentifierHaskellDirective (st);
		retType = lookupKeyword (vStringValue (st->name), Lang_ObjectiveC);

		if (retType == -1)	/* If it's not a keyword */
		{
			return Tok_any;
		}
		else
		{
			return retType;
		}
	}
	else if (isSpace (*st->cp))
	{
		eatWhiteSpace (st);
		return lex (st);
	}
	else
		switch (*st->cp)
		{
		case '(':
			st->cp++;
			return Tok_PARL;

		case '\\':
			st->cp++;
			return Tok_Backslash;

		case '#':
			st->cp++;
			return Tok_Sharp;

		case '/':
			if (st->cp[1] == '*')	/* ergl, a comment */
			{
				eatComment (st);
				return lex (st);
			}
			else if (st->cp[1] == '/')
			{
				st->cp = NULL;
				return lex (st);
			}
			else
			{
				st->cp++;
				return Tok_any;
			}
			break;

		case ')':
			st->cp++;
			return Tok_PARR;
		case '{':
			st->cp++;
			return Tok_CurlL;
		case '}':
			st->cp++;
			return Tok_CurlR;
		case '[':
			st->cp++;
			return Tok_SQUAREL;
		case ']':
			st->cp++;
			return Tok_SQUARER;
		case ',':
			st->cp++;
			return Tok_COMA;
		case ';':
			st->cp++;
			return Tok_semi;
		case ':':
			st->cp++;
			return Tok_dpoint;
		case '"':
			eatString (st);
			return Tok_any;
		case '+':
			st->cp++;
			return Tok_PLUS;
		case '-':
			st->cp++;
			return Tok_MINUS;

		default:
			st->cp++;
			break;
		}

	/* default return if nothing is recognized,
	 * shouldn't happen, but at least, it will
	 * be handled without destroying the parsing. */
	return Tok_any;
}

/*//////////////////////////////////////////////////////////////////////
//// Parsing                                    */
typedef void (*parseNext) (vString * const ident, haskellToken what);

/********** Helpers */
/* This variable hold the 'parser' which is going to
 * handle the next token */
parseNext toDoNext;

/* Special variable used by parser eater to
 * determine which action to put after their
 * job is finished. */
parseNext comeAfter;

/* Used by some parsers detecting certain token
 * to revert to previous parser. */
parseNext fallback;


/********** Grammar */
static void globalScope (vString * const ident, haskellToken what);
static void parseMethods (vString * const ident, haskellToken what);
static void parseImplemMethods (vString * const ident, haskellToken what);
static vString *tempName = NULL;
static vString *parentName = NULL;
static haskellKind parentType = K_INTERFACE;

/* used to prepare tag for OCaml, just in case their is a need to
 * add additional information to the tag. */
static void prepareTag (tagEntryInfo * tag, vString const *name, haskellKind kind)
{
	initTagEntry (tag, vStringValue (name));
	tag->kindName = HaskellKinds[kind].name;
	tag->kind = HaskellKinds[kind].letter;

	if (parentName != NULL)
	{
		tag->extensionFields.scope[0] = HaskellKinds[parentType].name;
		tag->extensionFields.scope[1] = vStringValue (parentName);
	}
}

void pushEnclosingContext (const vString * parent, haskellKind type)
{
	vStringCopy (parentName, parent);
	parentType = type;
}

void popEnclosingContext ()
{
	vStringClear (parentName);
}

/* Used to centralise tag creation, and be able to add
 * more information to it in the future */
static void addTag (vString * const ident, int kind)
{
	tagEntryInfo toCreate;
	prepareTag (&toCreate, ident, kind);
	makeTagEntry (&toCreate);
}

haskellToken waitedToken, fallBackToken;

/* Ignore everything till waitedToken and jump to comeAfter.
 * If the "end" keyword is encountered break, doesn't remember
 * why though. */
static void tillToken (vString * const UNUSED (ident), haskellToken what)
{
	if (what == waitedToken)
		toDoNext = comeAfter;
}

static void tillTokenOrFallBack (vString * const UNUSED (ident), haskellToken what)
{
	if (what == waitedToken)
		toDoNext = comeAfter;
	else if (what == fallBackToken)
	{
		toDoNext = fallback;
	}
}

static void ignoreBalanced (vString * const UNUSED (ident), haskellToken what)
{
	static int count = 0;

	switch (what)
	{
	case Tok_PARL:
	case Tok_CurlL:
	case Tok_SQUAREL:
		count++;
		break;

	case Tok_PARR:
	case Tok_CurlR:
	case Tok_SQUARER:
		count--;
		break;

	default:
		/* don't care */
		break;
	}

	if (count == 0)
		toDoNext = comeAfter;
}

static void parseFields (vString * const ident, haskellToken what)
{
	switch (what)
	{
	case Tok_CurlR:
		toDoNext = &parseMethods;
		break;

	case Tok_SQUAREL:
	case Tok_PARL:
		toDoNext = &ignoreBalanced;
		comeAfter = &parseFields;
		break;

		// we got an identifier, keep track
		// of it
	case HaskellIDENTIFIER:
		vStringCopy (tempName, ident);
		break;

		// our last kept identifier must be our
		// variable name =)
	case Tok_semi:
		addTag (tempName, K_FIELD);
		vStringClear (tempName);
		break;

	default:
		/* NOTHING */
		break;
	}
}


/*////////////////////////////////////////////////////////////////
//// Deal with the system                                       */

static void findHaskellTags (void)
{
	vString *name = vStringNew ();
	lexingState st;
	haskellToken tok;

	parentName = vStringNew ();
	tempName = vStringNew ();
	fullMethodName = vStringNew ();
	prevIdent = vStringNew ();

	st.name = vStringNew ();
	st.cp = fileReadLine ();
	toDoNext = &globalScope;
	tok = lex (&st);
	while (tok != Tok_EOF)
	{
		(*toDoNext) (st.name, tok);
		tok = lex (&st);
	}

	vStringDelete (name);
	vStringDelete (parentName);
	vStringDelete (tempName);
	vStringDelete (fullMethodName);
	vStringDelete (prevIdent);
	parentName = NULL;
	tempName = NULL;
	prevIdent = NULL;
	fullMethodName = NULL;
}

static void haskellInitialize (const langType language)
{
	Lang_Haskell = language;
	initKeywordHash ();
}

extern parserDefinition *HaskellParser (void)
{
	static const char *const extensions[] = { "hs", "hs-boot", NULL };
	parserDefinition *def = parserNew ("Haskell");
	def->kinds = HaskellKinds;
	def->kindCount = KIND_COUNT (HaskellKinds);
	def->extensions = extensions;
	def->parser = findHaskellTags;
	def->initialize = haskellInitialize;

	return def;
}
