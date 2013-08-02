#include "yabid.h"

int bfid_peek( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 3 ){
		printf( "usage: %s [type] [where]\n"
			"types:	ip 	- replace instruction at [where]\n"
			"	mem	- set value in memory at [where]\n", argv[0] );

		return 1;
	}

	int where = atoi( argv[2] );

	if ( strcmp( argv[1], "ip" ) == 0 ){
		if ( where >= dbf->codesize ){
			printf( "\"%d\" is out of code range.\n", where );
			return 2;
		}

		return dbf->code[ where ];
	} else if ( strcmp( argv[1], "mem" ) == 0 ){
		if ( where >= dbf->memsize ){
			printf( "\"%d\" is out of memory range.\n", where );
			return 3;
		}

		return dbf->mem[ where ]; 
	} else {
		printf( "Invalid type.\n" );
		return 4;
	}

	return 0;
}

int bfid_poke( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 4 ){
		printf( "usage: %s [type] [where] [value]\n"
			"[value] must be a decimal number.\n" 
			"types:	ip 	- replace instruction at [where]\n"
			"	mem	- set value in memory at [where]\n", argv[0] );

		return 1;
	}

	int where = atoi( argv[2] );
	int val = atoi( argv[3] );

	if ( strcmp( argv[1], "ip" ) == 0 ){
		if ( where >= dbf->codesize ){
			printf( "\"%d\" is out of code range.\n", where );
			return 2;
		}

		dbf->code[ where ] = val;
	} else if ( strcmp( argv[1], "mem" ) == 0 ){
		if ( where >= dbf->memsize ){
			printf( "\"%d\" is out of memory range.\n", where );
			return 3;
		}

		dbf->mem[ where ] = val;
	} else {
		printf( "Invalid type.\n" );
		return 4;
	}

	return 0;
}

int bfid_dump( bf_code_t *dbf, int argc, char **argv ){
	int i, h, l;
	if ( argc == 1 ){
		l = dbf->ptr;
		h = dbf->ptr + 1;
	} else if ( argc == 2 ){
		l = dbf->ptr;
		h = l + atoi( argv[1] );
	} else {
		l = atoi( argv[1] );
		h = atoi( argv[2] );
	}
	for ( i = l; i < h && i < dbf->memsize; i++ ){
		if (!(( i - l ) % 16 )){
			if ( i - l ) printf( "\n" );
			printf( "[%d]\t", i );
		}

		if (!(( i - l ) % 8 ))
			printf( "  ", i );

		if ( dbf->mem[i] )
			printf( "%02x ", dbf->mem[i] & 0xff );
		else
			printf( "   " );
	}
	printf( "\n" );

	return 0;
}

int bfid_disas( bf_code_t *dbf, int argc, char **argv ){
	int i, j, h, l;
	char *bfcmds = "><+-.,[]#";
	char c;

	if ( argc == 1 ){
		l = dbf->ip;
		h = dbf->ip + 1;
	} else if ( argc == 2 ){
		l = dbf->ip;
		h = l + atoi( argv[1] );
	} else if ( argc == 3 ){
		l = atoi( argv[1] );
		h = atoi( argv[2] );
	} else {
		l = atoi( argv[1] );
		h = atoi( argv[2] );
		bfcmds = argv[3];
	}

	for ( i = l; i < h && i < dbf->codesize; i++ ){
		if (!(( i - l ) % 32 )){
			if ( i - l ) printf( "\n" );
			printf( "[%d]\t", i );
		}

		if (!(( i - l ) % 8 ))
			printf( "  ", i );

		c = '_';
		for ( j = 0; bfcmds[j]; j++ ){
			if ( dbf->code[i] == bfcmds[j] ){
				c = dbf->code[i];
				break;
			}
		}

		printf( "%c", c );
	}
	printf( "\n" );

	return 0;
}

int bfid_trace( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 2 ){
		printf( "usage: %s [type]\n", argv[0] );
		printf( "types:	loop 	- trace where loops enter/leave.\n" );
		return 1;
	}

	if ( strcmp( argv[1], "loop" ) == 0 ) {
		int i = dbf->depth, j, ip;
		if ( !i ){
			printf( "Not inside a loop.\n" );
			return 0;
		}

		//printf( "trace from loop %d:\n", i );
		while( i ){
			j = 1;
			ip = dbf->lp[i];

			while( j && ++ip < dbf->codesize ){
				switch( dbf->code[ ip ] ){
					case ']': 
						j--; 
						//printf( "j: %d, ", j );
						break;
					case '[': 
						j++; 
						//printf( "j: %d, ", j );
						break;
				}
			}

			printf( "\tloop %d:\tip=%d", i, dbf->lp[i] );
			if ( !j )
				printf( "\tclose=%d ", ip );
			else
				printf( "\t[open] " );

			printf( "\n" );
			i--;
		}

	} else {
		printf( "Invalid type.\n" );
		return 2;
	}

	return 0;
}
