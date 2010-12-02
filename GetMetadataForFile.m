#include <stdio.h>
#import <CoreFoundation/CoreFoundation.h>
#import <CoreServices/CoreServices.h> 
#import <Foundation/Foundation.h>
#include "ctagSource/entry.h"
#include "ctagSource/parse.h"
#include "ctagSource/options.h"
#include "ctagSource/routines.h"
#include "ctagSource/keyword.h"
#include "ctagSource/read.h"

typedef enum KindIndex_t
{
    kindFunctions,
    kindClasses,
    kindMacros,
    kindMethods,
    kindConstructors,
    kindTypes,
    kindModules,
    kindGlobals,
    kindStructures,
    kindConstants,
    kindEnums,
    kindInterfaces,
    kindProperties,

    // must be the last one
    kindUNVALID
} KindIndex;

char*   kindPublicNames[] =
    { "com_ctags_functions"
    , "com_ctags_classes"
    , "com_ctags_macros"
    , "com_ctags_methods"
    , "com_ctags_constructors"
    , "com_ctags_types"
    , "com_ctags_modules"
    , "com_ctags_globals"
    , "com_ctags_structures"
    , "com_ctags_constants"
    , "com_ctags_enums" 
    , "com_ctags_interfaces"
    , "com_ctags_property"
    };

typedef struct LangAssoc_t
{
    char *utiName;
    char *langForceFlag;
} LangAssoc;

LangAssoc utiTranslation[] =
    { { "public.c-source", "--language-force=C" }
    , { "public.c-header", "--language-force=C" }
    , { "public.c-plus-plus-source", "--language-force=C++" }
    , { "public.c-plus-plus-header", "--language-force=C++" }
    , { "com.sun.java-source", "--language-force=Java" }
    , { "com.netscape.javascript-source", "--language-force=JavaScript" }
    , { "public.perl-script", "--language-force=Perl" }
    , { "public.python-script", "--language-force=Python" }
    , { "public.ruby-script", "--language-force=Ruby" }
    , { "public.php-script", "--language-force=PHP" }
    , { "public.shell-script", "--language-force=Sh" }
    , { "public.objective-c-source", "--language-force=ObjectiveC" }
    , { "public.csharp-source", "--language-sorce=C#" }
//public.lua-script Lua
//public.csh-script Sh
//public.pascal-source Pascal
//public.scheme-source Scheme
//public.erlang-source Erlang
//public.ocaml-source OCaml
//public.sml-source SML
//public.tcl-script Tcl
//public.verilog-source Verilog
//public.vhdl-source VHDL
//public.eiffel-source Eiffel
//public.lisp-source Lisp
//public.fortran-source Fotran
//public.assembly-source Asm
    };

char* findCTagLanguage( const char* const uti )
{
    int translationCount =
        sizeof( utiTranslation ) / sizeof( LangAssoc );

    for ( int i = 0; i < translationCount; i++ )
    {
        if ( strcmp( uti, utiTranslation[ i ].utiName ) == 0 )
            return utiTranslation[ i ].langForceFlag;
    }

    return NULL;
}

///////////////////////////////////////////////////
//// Define the mapping between CTag kind letter
////and the type showed in spotlight.
///////////////////////////////////////////////////
typedef struct TupleKind_t
{
    char c;
    KindIndex index;
} TupleKind;

TupleKind   cKindTable[] =
    { { 'c', kindClasses }
    , { 'd', kindMacros }
    , { 't', kindTypes }
    , { 's', kindStructures }
    , { 'v', kindGlobals }
	, { 'x', kindGlobals }
	, { 'e', kindConstants }
	, { 'f', kindFunctions }
	, { 'g', kindEnums }
    };

TupleKind   objcKindTable[] =
    { { 'i', kindInterfaces }
    , { 'm', kindMethods }
    , { 'f', kindFunctions }
    , { 'e', kindEnums }
    , { 'M', kindMacros }
    , { 'p', kindProperties }
    , { 's', kindStructures }
    , { 'v', kindGlobals }
    , { 'c', kindMethods }
    };
TupleKind   pythonKindTable[] =
    { { 'c', kindClasses }
    , { 'f', kindFunctions }
    , { 'm', kindMethods }
    , { 'v', kindGlobals }
    , { 'i', kindModules }
    };

TupleKind rubyKindTable[] =
    { { 'c', kindClasses }
    , { 'f', kindMethods }
    , { 'm', kindModules }
    };

TupleKind   csharpKindTable[] =
    { { 'c', kindClasses }
    , { 'g', kindEnums }
    , { 's', kindStructures }
    , { 't', kindTypes }
    , { 'n', kindModules }
    , { 'i', kindInterfaces }
    , { 'p', kindProperties }
    };

TupleKind javaKindTable[] =
    { { 'c', kindClasses }
    , { 'e', kindEnums }
    , { 'm', kindMethods }
    , { 'p', kindModules }
    , { 'i', kindInterfaces }
    };

TupleKind phpKindTable[] =
    { { 'c', kindClasses }
    , { 'i', kindInterfaces }
    , { 'd', kindConstants }
    , { 'f', kindFunctions }
    };

TupleKind shKindTable[] =
    { { 'f', kindFunctions } };

TupleKind javascriptKindTable[] =
    { { 'f', kindFunctions }
    , { 'c', kindClasses }
    , { 'm', kindMethods }
    , { 'p', kindProperties }
    , { 'v', kindGlobals }
    };

TupleKind perlKindTable[] =
    { { 'c', kindConstants }
    , { 'p', kindModules }
    , { 's', kindFunctions }
    };

void fillTable( const TupleKind def[]
              , const size_t defSize
              , KindIndex /*OUT*/ assoc[] )
{
    for (int i = 0; i < sizeof(char) * 8; ++i)
        assoc[i] = kindUNVALID;

    for (int i = 0; i < defSize; ++i)
        assoc[ def[i].c ] = def[i].index;
}

/**
 * Type holding a mapping between a language ctags
 * letter kind and spotlight kinds.
 */
typedef struct AssocKindFiller_t
{
    TupleKind   *assoc;
    size_t      assocElemCount;
} AssocKindFiller;

#define LANGTABLE(x) \
    { x, sizeof(x) / sizeof(TupleKind) }

#define NULL_LANG { NULL, 0 }

AssocKindFiller assocBuilders[] =
    { NULL_LANG // AntParser
    , NULL_LANG // AsmParser
    , NULL_LANG // AspParser
    , NULL_LANG // AwkParser
    , NULL_LANG // BasicParser
    , NULL_LANG // BetaParser
    , LANGTABLE(cKindTable) // CParser
    , LANGTABLE(cKindTable) // CppParser
    , LANGTABLE(csharpKindTable) // CsharpParser
    , NULL_LANG // CobolParser
    , NULL_LANG // DosBatchParser
    , NULL_LANG // EiffelParser
    , NULL_LANG // ErlangParser
    , NULL_LANG // FlexParser
    , NULL_LANG // FortranParser
    , NULL_LANG // HtmlParser
    , LANGTABLE(javaKindTable)// JavaParser
    , LANGTABLE(javascriptKindTable)// JavaScriptParser
    , NULL_LANG // LispParser
    , NULL_LANG // LuaParser
    , NULL_LANG // MakefileParser
    , NULL_LANG // MatLabParser
    , LANGTABLE(objcKindTable) // ObjcParser 
    , NULL_LANG // OcamlParser
    , NULL_LANG // PascalParser
    , LANGTABLE(perlKindTable)// PerlParser
    , LANGTABLE(phpKindTable)// PhpParser
    , LANGTABLE(pythonKindTable) // PythonParser
    , NULL_LANG // RexxParser
    , LANGTABLE(rubyKindTable) // RubyParser
    , NULL_LANG // SchemeParser
    , LANGTABLE(shKindTable) // ShParser
    , NULL_LANG // SlangParser
    , NULL_LANG // SmlParser
    , NULL_LANG // SqlParser
    , NULL_LANG // TclParser
    , NULL_LANG // TexParser
    , NULL_LANG // VeraParser
    , NULL_LANG // VerilogParser
    , NULL_LANG // VhdlParser
    , NULL_LANG // VimParser
    , NULL_LANG // YaccParser
    };

typedef struct MetaDataContext_t
{
    CFMutableArrayRef       kindLists[ kindUNVALID ];
    KindIndex               kindTable[ sizeof(char) * 8 ];
} MetaDataContext;

MetaDataContext ctxt;

Boolean prepareContext( const char* filePath
                      , MetaDataContext *ctxt )
{
    int languageIndex = getFileLanguage (filePath);

    if (languageIndex < 0)
        return FALSE;

    // prepare the association table used for the file
    // language.
    AssocKindFiller filler =
        assocBuilders[ languageIndex ];

    if (filler.assoc == NULL)
        return FALSE;

    fillTable( filler.assoc
             , filler.assocElemCount
             , ctxt->kindTable );

    // the callback is used to ensure that CFObject
    // are retained in the object
    for (int i = 0; i < kindUNVALID; ++i)
        ctxt->kindLists[ i ] =
            CFArrayCreateMutable( NULL, 0
                                , &kCFTypeArrayCallBacks );
        
    return TRUE;
}

void cleanupContext( MetaDataContext *ctxt )
{
    for (int i = 0; i < kindUNVALID; ++i)
        CFRelease(ctxt->kindLists[ i ]);
}

void dumpContextInAttributes
             ( const MetaDataContext  *ctxt
			 , CFMutableDictionaryRef attributes )
{ 
    for (int i = 0; i < kindUNVALID; ++i)
    {
        if ( CFArrayGetCount( ctxt->kindLists[i] ) == 0 )
            continue;

        CFStringRef key =
            CFStringCreateWithCString( NULL
                                     , kindPublicNames[i]
                                     , kCFStringEncodingUTF8 );
        CFDictionaryAddValue( attributes
                            , key
                            , ctxt->kindLists[i]
                            );
        CFRelease( key );
    }
}

Boolean GetMetadataForURL
        ( void* thisInterface
        , CFMutableDictionaryRef attributes
        , CFStringRef contentTypeUTI
        , CFURLRef urlForFile )
{
    const int FILEPATH_BUFFERSIZE = 1024;
    const int UTI_BUFFERSIZE = 128;
    char    filePath[ FILEPATH_BUFFERSIZE ];
    char    utiBuffer[ UTI_BUFFERSIZE ];

    assert( sizeof(kindPublicNames) / sizeof(char*) == kindUNVALID );

    CFStringRef ref = 
        CFURLCopyFileSystemPath( urlForFile
                               , kCFURLPOSIXPathStyle );
    CFStringGetCString( ref
                      , filePath
                      , FILEPATH_BUFFERSIZE
                      , kCFStringEncodingUTF8 ); 

    CFRelease( ref );

    
    ////////////////////////////////////////////
    //// Spotlight can give us precise file
    //// format information, so we force
    //// CTags language acordingly.
    ////////////////////////////////////////////
    char *argv[] = { NULL, NULL, NULL };

    CFStringGetCString( contentTypeUTI
                      , utiBuffer
                      , UTI_BUFFERSIZE
                      , kCFStringEncodingUTF8 );

    char *mayFlag = findCTagLanguage( utiBuffer );
    if ( mayFlag != NULL )
    {
        argv[0] = mayFlag;
        argv[1] = filePath;
    }
    else argv[0] = filePath;

    ////////////////////////////////////////////
    //// CTags initialization
    ////////////////////////////////////////////
	cookedArgs *args;
    setExecutableName( "CTagSpotlight" );

	args = cArgNewFromArgv (argv);
	checkRegex ();

	initializeParsing ();
	initOptions ();
	readOptionConfiguration ();
	parseOptions (args);
	checkOptions ();

    // My init
    prepareContext( filePath, &ctxt );

    ////////////////////////////////////////////
    //// Real work
    ////////////////////////////////////////////
    parseFile( filePath );
    dumpContextInAttributes( &ctxt, attributes );

    ////////////////////////////////////////////
    //// Cleanup
    ////////////////////////////////////////////
    // CTags
	cArgDelete (args);
	freeKeywordTable ();
	freeRoutineResources ();
	freeSourceFileResources ();
	freeTagFileResources ();
	freeOptionResources ();
	freeParserResources ();
	freeRegexResources ();

    // Me
    cleanupContext( &ctxt );

    return TRUE;
}

extern void notifySystemTagEntry (const tagEntryInfo *const tag)
{
    KindIndex bucketIndex = ctxt.kindTable[ tag->kind ];

    // if the kind char is not mappend, drop it
    if (bucketIndex == kindUNVALID)
        return;

    // if mapped, drop it in the good kind bucket
    CFStringRef tagName =
        CFStringCreateWithCString( NULL, tag->name
                                 , kCFStringEncodingUTF8 );

    CFArrayAppendValue( ctxt.kindLists[ bucketIndex ]
                      , tagName
                      );

    CFRelease( tagName );
}

