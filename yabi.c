#include "yabi.h"

void die( int code, char *fmt, ... ){
	va_list ap;
	va_start( ap, fmt );

	printf( "[%d] Error: ", code );
	vprintf( fmt, ap );

	va_end( ap );
	exit( code );
}


int bf_interp( bf_code_t *bf ){
	int state;
	switch ( bf->code[ bf->ip ]){
		case '>':
			if ( bf->ptr < bf->memsize )
				bf->ptr++;
			break;
		case '<':
			if ( bf->ptr )
				bf->ptr--;
			break;
		case '+':
			bf->mem[ bf->ptr ]++;
			break;
		case '-':
			bf->mem[ bf->ptr ]--;
			break;
		case '.':
			fputc( bf->mem[ bf->ptr ], bf->out );
			break;
		case ',':
			bf->mem[ bf->ptr ] = fgetc( bf->in );
			break;
		case '[':
			state = 1;
			if ( !bf->mem[ bf->ptr ] ){
				for ( ; state && ++bf->ip < bf->codesize; ){
					switch ( bf->code[ bf->ip ] ){
						case ']': state--; break;
						case '[': state++; break;
					}
				}
			} else {
				bf->lp[++bf->depth] = bf->ip;
			}
			break;

		case ']':
			if ( bf->depth ){
				if ( !bf->mem[ bf->ptr ] )
					bf->depth--;
				else
					bf->ip = bf->lp[ bf->depth ];
			}
			break;
		
		case '#':
			if ( !bf->debugging )
				bf_debugger( bf );
			//fprintf( bf->out, "[debug stub]\n" );
			break;
	}

	return 0;
}

int bf_step( bf_code_t *bf ){
	bf_interp( bf );
	if ( bf->ip < bf->codesize )
		bf->ip++;

	bf->execed++;

	return 0;
}

int bf_run( bf_code_t *bf ){
	while ( bf->ip < bf->codesize )
		bf_step( bf );

	return 0;
}

void bf_help( ){
	printf( "usage: yabi [-f file] [-dh]\n"
		"	-f  specify input code file\n"
		"	-i  file to use as input for program\n"
		"	-o  file to send program output to\n"
		"	-d  debug the program instead of run it\n" 
		"	-h  print this help and exit\n" );
	return;
}

int main( int argc, char *argv[] ){
	struct stat sb;
	bf_code_t *bf;
	int d_memsize = 0x10000;
	int d_loops = 0x1000;
	FILE *fp;

	char *filename = NULL;
	char *p_in = NULL;
	char *p_out = NULL;
	char ch;
	char debug = 0;
	int i = 0;

	while (( ch = getopt( argc, argv, "f:i:o:dh" )) != -1 && i++ < argc ){
		switch( ch ){
			case 'f':
				filename = argv[++i];
				break;
			case 'd':
				debug = 1;
				break;
			case 'i':
				p_in = argv[++i];
				break;
			case 'o':
				p_out = argv[++i];
				break;
			case 'h':
				bf_help( );
				exit( 0 );
				break;
		}
	}

	if ( !filename ){
		die( 1, "Need filename. (see help with -h)\n" );
		exit( 0 );
	}

	if ( stat( filename, &sb ) < -1 )
		die( 2, "Could not stat \"%s\"\n", filename );

	if (( fp = fopen( filename, "r" )) == NULL )
		die( 3, "Could not open program file \"%s\"\n", filename );

	bf = new( bf_code_t );
	bf->codesize = sb.st_size;
	bf->code = new( char[ sb.st_size ]);
	fread( bf->code, bf->codesize, 1, fp );
	fclose( fp );

	bf->in = stdin;
	bf->out = stdout;

	if ( p_in && !( bf->in = fopen( p_in, "r" )))
		die( 4, "Could not open input file \"%s\"\n", p_in );

	if ( p_out && !( bf->out = fopen( p_out, "w" )))
		die( 4, "Could not open output file \"%s\"\n", p_out );

	bf->memsize = d_memsize;
	bf->mem = new( char[ d_memsize + 1 ]);

	bf->maxloops = d_loops;
	bf->lp = new( int[ d_loops ]);

	if ( !debug )
		bf_run( bf );
	else
		bf_debugger( bf );
	
	return 0;
}

