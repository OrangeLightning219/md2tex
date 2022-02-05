#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/utils.h"

void PrintArgumentsInfo()
{
    printf( "The arguments should be:\n" );
    printf( "1. Name of the Markdown file to convert\n" );
    printf( "2. Output file name\n" );
}

int ReadLine( char *fileBuffer, char *lineBuffer, int fileOffset )
{
    int length = 0;
    while ( fileBuffer[ fileOffset + length ] != '\n' )
    {
        ++length;
    }

    ++length; // include '\n'
    memcpy( lineBuffer, &fileBuffer[ fileOffset ], length );
    return length;
}

int EatLeadingSpaces( char *line )
{
    int result = 0;
    while ( line[ result ] == ' ' )
    {
        ++result;
    }
    return result;
}

inline void IndentFile( FILE *file, int level )
{
    for ( int i = 0; i < level; ++i )
    {
        fprintf( file, "    " );
    }
}

int main( int argc, char **argv )
{
    if ( argc < 3 )
    {
        printf( "Provided too few arguments.\n" );
        PrintArgumentsInfo();
        return -1;
    }
    else if ( argc > 3 )
    {
        printf( "Provided too many arguments.\n" );
        PrintArgumentsInfo();
        return -1;
    }

    char *inputFileName = argv[ 1 ];
    char *outputFileName = argv[ 2 ];

    FILE *inputFile;
    fopen_s( &inputFile, inputFileName, "rt" );

    if ( !inputFile )
    {
        printf( "Failed to open file: %s.\n", inputFileName );
        return -1;
    }

    fseek( inputFile, 0, SEEK_END );
    int inputFileSize = ( int ) ftell( inputFile );
    fseek( inputFile, 0, SEEK_SET );

    if ( inputFileSize == 0 )
    {
        printf( "Input file size is 0.\n" );
        fclose( inputFile );
        return -1;
    }

    char *inputFileText = ( char * ) malloc( ( inputFileSize + 1 ) * sizeof( char ) );
    defer { free( inputFileText ); };
    int countRead = ( int ) fread( inputFileText, sizeof( char ), inputFileSize, inputFile );

    if ( countRead < inputFileSize )
    {
        inputFileSize = countRead;
        inputFileText = ( char * ) realloc( inputFileText, countRead + 1 );
    }

    inputFileText[ countRead ] = '\0';
    fclose( inputFile );

    FILE *outputFile;
    fopen_s( &outputFile, outputFileName, "w" );

    if ( !outputFile )
    {
        printf( "Failed to create file: %s\n", outputFileName );
        return -1;
    }

    defer { fclose( outputFile ); };

    char *lineBuffer = ( char * ) malloc( 200 * sizeof( char ) );
    defer { free( lineBuffer ); };

    int inputFileCursor = 0;
    int lineLength = -1;
    int lineCount = 0;
    int currentIndentationLevel = 0;
    int previousIndentationLevel = -1;
    bool listOpened = false;
    int openedLists = 0;
    while ( inputFileCursor < inputFileSize )
    {
        lineLength = ReadLine( inputFileText, lineBuffer, inputFileCursor );
        inputFileCursor += lineLength;
        lineBuffer[ lineLength ] = '\0';
        ++lineCount;
        // printf( "%s", lineBuffer );

        int lineCursor = EatLeadingSpaces( lineBuffer );
        currentIndentationLevel = lineCursor / 4;
        // printf( "Line: %d, leading spaces: %d\n", lineCount, lineCursor );
        bool skipLine = false;
        while ( lineBuffer[ lineCursor ] && !skipLine )
        {
            char currentChar = lineBuffer[ lineCursor ];
            skipLine = false;
            switch ( currentChar )
            {
                case '\n':
                {
                    if ( lineLength == 1 && listOpened )
                    {
                        for ( int i = 0; i < openedLists + 1; ++i )
                        {
                            IndentFile( outputFile, openedLists - i );
                            fprintf( outputFile, "\\end{itemize}\n" );
                        }
                        listOpened = false;
                        openedLists = 0;
                    }
                    fprintf( outputFile, "%c", currentChar );
                }
                break;

                case '#':
                {
                    if ( lineBuffer[ lineCursor + 1 ] == '#' && lineBuffer[ lineCursor + 2 ] == '#' && lineBuffer[ lineCursor + 3 ] == ' ' )
                    {
                        // subsubsection
                        lineBuffer[ lineLength - 1 ] = '\0';
                        fprintf( outputFile, "\\subsubsection{" );
                        fprintf( outputFile, "%s", &lineBuffer[ lineCursor + 4 ] );
                        fprintf( outputFile, "}\n" );
                        skipLine = true;
                    }
                    else if ( lineBuffer[ lineCursor + 1 ] == '#' && lineBuffer[ lineCursor + 2 ] == ' ' )
                    {
                        // subsection
                        lineBuffer[ lineLength - 1 ] = '\0';
                        fprintf( outputFile, "\\subsection{" );
                        fprintf( outputFile, "%s", &lineBuffer[ lineCursor + 3 ] );
                        fprintf( outputFile, "}\n" );
                        skipLine = true;
                    }
                    else if ( lineBuffer[ lineCursor + 1 ] == ' ' )
                    {
                        // section
                        lineBuffer[ lineLength - 1 ] = '\0';
                        fprintf( outputFile, "\\section{" );
                        fprintf( outputFile, "%s", &lineBuffer[ lineCursor + 2 ] );
                        fprintf( outputFile, "}\n" );
                        skipLine = true;
                    }
                    else
                    {
                        fprintf( outputFile, "%c", currentChar );
                    }
                }
                break;

                case '*':
                {
                    if ( currentIndentationLevel == lineCursor / 4 ) // first character in line
                    {
                        if ( !listOpened )
                        {
                            IndentFile( outputFile, currentIndentationLevel );
                            fprintf( outputFile, "\\begin{itemize}\n" );
                            listOpened = true;
                            fprintf( outputFile, "\\item%s", &lineBuffer[ lineCursor + 1 ] );
                            skipLine = true;
                        }
                        else
                        {
                            if ( currentIndentationLevel > previousIndentationLevel )
                            {
                                IndentFile( outputFile, currentIndentationLevel );
                                fprintf( outputFile, "\\begin{itemize}\n" );
                                ++openedLists;
                            }
                            else if ( currentIndentationLevel < previousIndentationLevel )
                            {
                                IndentFile( outputFile, previousIndentationLevel );
                                fprintf( outputFile, "\\end{itemize}\n" );
                                --openedLists;
                            }
                            IndentFile( outputFile, currentIndentationLevel );
                            fprintf( outputFile, "\\item%s", &lineBuffer[ lineCursor + 1 ] );
                            skipLine = true;
                        }
                    }
                    else if ( lineBuffer[ lineCursor + 1 ] == '*' )
                    {
                        fprintf( outputFile, "\\textbf{" );
                        lineCursor += 2;
                        while ( !( lineBuffer[ lineCursor ] == '*' && lineBuffer[ lineCursor + 1 ] == '*' ) )
                        {
                            fprintf( outputFile, "%c", lineBuffer[ lineCursor ] );
                            ++lineCursor;
                        }
                        fprintf( outputFile, "}" );
                        ++lineCursor;
                    }
                    else
                    {
                        fprintf( outputFile, "%c", currentChar );
                    }
                }
                break;

                case '_':
                {
                    fprintf( outputFile, "\\emph{" );
                    ++lineCursor;
                    while ( lineBuffer[ lineCursor ] != '_' )
                    {
                        fprintf( outputFile, "%c", lineBuffer[ lineCursor ] );
                        ++lineCursor;
                    }
                    fprintf( outputFile, "}" );
                    ++lineCursor;
                }
                break;

                default:
                {
                    fprintf( outputFile, "%c", currentChar );
                }
                break;
            }

            ++lineCursor;
        }
        previousIndentationLevel = currentIndentationLevel;
    }

    printf( "File %s generated successfully!\n", outputFileName );
    return 0;
}
