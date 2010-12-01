#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h> 
#include "ctagSource/entry.h"
#include "ctagSource/parse.h"
#include "ctagSource/options.h"
#include "ctagSource/routines.h"

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
    };

void allocateKinds()
{
}

void cleanupAllocatedKinds()
{
}

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
    , NULL_LANG // CppParser
    , NULL_LANG // CsharpParser
    , NULL_LANG // CobolParser
    , NULL_LANG // DosBatchParser
    , NULL_LANG // EiffelParser
    , NULL_LANG // ErlangParser
    , NULL_LANG // FlexParser
    , NULL_LANG // FortranParser
    , NULL_LANG // HtmlParser
    , NULL_LANG // JavaParser
    , NULL_LANG // JavaScriptParser
    , NULL_LANG // LispParser
    , NULL_LANG // LuaParser
    , NULL_LANG // MakefileParser
    , NULL_LANG // MatLabParser
    , NULL_LANG // ObjcParser 
    , NULL_LANG // OcamlParser
    , NULL_LANG // PascalParser
    , NULL_LANG // PerlParser
    , NULL_LANG // PhpParser
    , NULL_LANG // PythonParser
    , NULL_LANG // RexxParser
    , NULL_LANG // RubyParser
    , NULL_LANG // SchemeParser
    , NULL_LANG // ShParser
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
    FILE*       logging;
} MetaDataContext;

MetaDataContext ctxt;

void LogInit( MetaDataContext  *ctxt )
{
    ctxt->logging = fopen("/Users/vince/Desktop/log.debug.txt", "w");
}

void LogStr( MetaDataContext  *ctxt, const char* str )
{
    fprintf( ctxt->logging, "%s", str );
    fflush( ctxt->logging );
}

void LogEnd( MetaDataContext *ctxt )
{
    fclose( ctxt->logging );
    ctxt->logging = NULL;
}

Boolean prepareContext( const char* filePath
                      , MetaDataContext *ctxt )
{
    int languageIndex = getFileLanguage (filePath);
    // prepare the association table used for the file
    // language.
    AssocKindFiller filler =
        assocBuilders[ languageIndex ];

    if (filler.assoc == NULL)
        return FALSE;

    fillTable( filler.assoc
             , filler.assocElemCount
             , ctxt->kindTable );

    for (int i = 0; i < kindUNVALID; ++i)
        ctxt->kindLists[ i ] =
            CFArrayCreateMutable( NULL, 5, NULL );
        
    LogInit( ctxt );
    
    return TRUE;
}

void cleanupContext( MetaDataContext *ctxt )
{
    for (int i = 0; i < kindUNVALID; ++i)
        CFRelease(ctxt->kindLists[ i ]);
    LogEnd( ctxt );
}

void dumpContextInAttributes
             ( const MetaDataContext  *ctxt
			 , CFMutableDictionaryRef attributes )
{ 
    for (int i = 0; i < kindUNVALID; ++i)
    {
        CFStringRef key =
            CFStringCreateWithCString( NULL
                                     , kindPublicNames[ i ]
                                     , kCFStringEncodingUTF8 );
        CFDictionaryAddValue( attributes
                            , key
                            , ctxt->kindLists[ i ]
                            );
        CFRelease( key );
    }
}

void initCTags( char* filepath )
{
	cookedArgs *args;
    char *argv[] = { filepath, NULL };
    setExecutableName( "CTagSpotlight" );

	args = cArgNewFromArgv (argv);
	checkRegex ();

	initializeParsing ();
	initOptions ();
	readOptionConfiguration ();
	parseOptions (args);
	checkOptions ();
}

Boolean GetMetadataForURL
        ( void* thisInterface
        , CFMutableDictionaryRef attributes
        , CFStringRef contentTypeUTI
        , CFURLRef urlForFile )
{
    const int FILEPATH_BUFFERSIZE = 1024;
    char    filePath[ FILEPATH_BUFFERSIZE ];


    CFStringRef ref = 
        CFURLCopyFileSystemPath( urlForFile
                               , kCFURLPOSIXPathStyle );
    CFStringGetCString( ref
                      , filePath
                      , FILEPATH_BUFFERSIZE
                      , kCFStringEncodingUTF8 ); 

    CFRelease( ref );
    initCTags( filePath );
    prepareContext( filePath, &ctxt );

    // delete parser definition.

    parseFile( filePath );
    dumpContextInAttributes( &ctxt, attributes );

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

