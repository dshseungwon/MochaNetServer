#include "MMOPCH.h"


extern const char** __argv;
extern int __argc;
void OutputDebugString( const char* inString )
{
    printf( "%s", inString );
}

string StringUtils::GetCommandLineArg( int inIndex )
{
    if( inIndex < __argc )
    {
        return string( __argv[ inIndex ] );
    }
    
    return string();
}


string StringUtils::Sprintf( const char* inFormat, ... )
{
    //not thread safe...
    static char temp[ 4096 ];
    
    va_list args;
    va_start (args, inFormat );
    
    vsnprintf(temp, 4096, inFormat, args);
    return string( temp );
}

// void StringUtils::Log( const char* inFormat )
// {
//     OutputDebugString( inFormat );
//     OutputDebugString( "\n" );
// }

void StringUtils::Log( const char* inFormat, ... )
{
    //not thread safe...
    static char temp[ 4096 ];
    
    va_list args;
    va_start (args, inFormat );

    vsnprintf(temp, 4096, inFormat, args);

    OutputDebugString( temp );
    OutputDebugString( "\n" );
}

