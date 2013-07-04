#include "yabi.h"

typedef int (*bfid_func_t)( bf_code_t *dbf, int argc, char **argv );
typedef struct bfid_command {
	char *name;
	char *help;
	bfid_func_t func;
} bfid_cmd_t;

int bfid_initialized = 0;
int bfid_cmd_no = 0;
bfid_cmd_t *bfid_cmds;

int bfid_step( bf_code_t *dbf, int argc, char **argv );
int bfid_dump( bf_code_t *dbf, int argc, char **argv );
int bfid_exit( bf_code_t *dbf, int argc, char **argv );
int bfid_cont( bf_code_t *dbf, int argc, char **argv );
int bfid_status( bf_code_t *dbf, int argc, char **argv );
int bfid_set( bf_code_t *dbf, int argc, char **argv ); int bfid_help( bf_code_t *dbf, int argc, char **argv );
int bfid_hook( bf_code_t *dbf, int argc, char **argv );
int bfid_disas( bf_code_t *dbf, int argc, char **argv );
int bfid_trace( bf_code_t *dbf, int argc, char **argv );

int interrupted = 0;
int debug_signal( int s );

int debug_signal( int s ){
	interrupted = 1;

	return 0;
}

void register_bfid_func( char *name, char *help, bfid_func_t function ){
	if ( bfid_cmd_no >= 32 )
		return;

	bfid_cmds[ bfid_cmd_no ].name = name;
	bfid_cmds[ bfid_cmd_no ].help = help;
	bfid_cmds[ bfid_cmd_no ].func = function;
	
	bfid_cmd_no++;
}

void bfid_init( ){
	bfid_cmds = new( bfid_cmd_t[32] );
	
	register_bfid_func( "step", "step through code instructions", bfid_step );
	register_bfid_func( "dump", "dump program memory", bfid_dump );
	register_bfid_func( "cont", "continue program execution in the debugger", bfid_cont );
	register_bfid_func( "exit", "exit the debugger. Resumes execution if "
				"the debugger was started from the code.", bfid_exit );
	register_bfid_func( "status", "information about the program's state", bfid_status );
	register_bfid_func( "set", "set program variables", bfid_set );
	register_bfid_func( "hook", "hook onto program locations", bfid_hook );
	register_bfid_func( "disas", "disassemble program instructions", bfid_disas );
	register_bfid_func( "trace", "trace program structures", bfid_trace );
	register_bfid_func( "help", "Get help about debugger commands", bfid_help );
	register_bfid_func( "man", "Get help about debugger commands", bfid_help );
}

int bfid_execcmd( bf_code_t *dbf, char *str ){
	int i, argc, slen, found;

	found = argc = 0;
	slen = strlen( str );

	char buf[slen + 1];
	char *args[256];
	args[argc++] = buf;
	strcpy( buf, str );

	for ( i = 0; i < slen && argc < 16; i++ ){
		if ( buf[i] == ' ' ){
			args[ argc++ ] = buf + i + 1;
			buf[i] = 0;
		} else if ( buf[i] == '\n' )
			buf[i] = 0;
	}

	for ( i = 0; i < bfid_cmd_no; i++ ){
		if ( strcmp( args[0], bfid_cmds[i].name ) == 0 ){
			found = 1;
			return bfid_cmds[i].func( dbf, argc, args );
		}
	}

	return -1;
}


int bf_debugger( bf_code_t *bf ){
	bf_code_t *dbf = bf;
	//bf_code_t *dbf = new( bf_code_t );
	//memcpy( dbf, bf, sizeof( bf_code_t ));
	char buf[256];
	int lret = 0;

	if ( !bfid_initialized ){
		bfid_init( );
		bfid_initialized = 1;
	}

	dbf->debugging = 1;
	interrupted = 1;
	signal( SIGINT, (__sighandler_t)debug_signal );

	printf( "[>] yabi debugger v0.01\n" );
	while ( dbf->debugging ){
		if ( interrupted ){
			if ( lret )
				printf( "[%d] ", lret );
			else 
				printf( "[ ] " );

			printf( "> " );

			fgets( buf, 256, stdin );
			if ( strlen( buf ) < 2 )
				continue;

			if (( lret = bfid_execcmd( dbf, buf )) < 0 )
				printf( "Unknown command.\n" );
		} else {
			if ( dbf->ip < dbf->codesize )
				bf_step( dbf );
			else {
				printf( "Program terminated.\n" );
				interrupted = 1;
			}
		}
	}

	//free( dbf );
	return 0;
}

int bfid_step( bf_code_t *dbf, int argc, char **argv ){
	char *bfcmds = "><+-.,[]#";
	char c;
	int i, j;
	if ( argc > 1 )
		i = atoi( argv[1] );
	else
		i = 1;

	//i %= dbf->codesize - dbf->ip + 1;
	c = '_';
	for ( j = 0; bfcmds[j]; j++ ){
		if ( dbf->code[ dbf->ip ] == bfcmds[j] ){
			c = dbf->code[ dbf->ip ];
			break;
		}
	}

	//printf( "stepping for %d\n", i );
	while( i-- ){
		printf( "[%c] stepping...\n", c );
		bf_step( dbf );
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

		printf( "%02x ", dbf->mem[i] & 0xff );
	}
	printf( "\n" );

	return 0;
}

int bfid_exit( bf_code_t *dbf, int argc, char **argv ){
	dbf->debugging = 0;
	printf( "resuming...\n" );
	//signal( SIGINT, SIG_DFL );

	return 0;
}

int bfid_cont( bf_code_t *dbf, int argc, char **argv ){
	interrupted = 0;

	return 0;
}

int bfid_status( bf_code_t *dbf, int argc, char **argv ){
	printf( "\tip: %d, ptr: %d, depth: %d\n", dbf->ip, dbf->ptr, dbf->depth );

	return 0;
}

int bfid_set( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 3 ){ 
		printf( "Usage: %s [variable] [value]\n", argv[0] );
		printf( "Can set: ip, ptr\n" );

		return 1;
	}

	if ( strcmp( argv[1], "ip" ) == 0 )
		dbf->ip = atoi( argv[2] );
	else if ( strcmp( argv[1], "ptr" ) == 0 )
		dbf->ptr = atoi( argv[2] );
	else
		printf( "Unknown variable name.\n" );

	return 0;
}

int bfid_hook( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 3 ){
		printf( "usage: %s [type] [value]\n", argv[0] );
		printf( "Type can be \"ip\", a numeric ip position other than 0, \"in\", a \n"
			"single-character instruction, or \"mem\", a memory address.\n" );
		return 1;
	}

	int type = 0, val = 0;

	if ( strcmp( argv[1], "ip" ) == 0 ) type = 1;
	else if ( strcmp( argv[1], "in" ) == 0 ) type = 2;
	else if ( strcmp( argv[1], "mem" ) == 0 ) type = 3;

	if ( !type ){
		printf( "Invalid type.\n" );
		return 2;
	}

	switch ( type ){
		case 1:
			val = atoi( argv[2] );
			while ( dbf->ip != val && dbf->ip < dbf->codesize )
				bf_step( dbf );
			break;
		case 2:
			val = argv[2][0];
			while ( dbf->code[ dbf->ip ] != val && dbf->ip < dbf->codesize )
				bf_step( dbf );
			break;
		case 3:
			val = atoi( argv[2] );
			while( dbf->ptr != val && dbf->ip != dbf->codesize )
				bf_step( dbf );
			break;
	}

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
		printf( "Type can be \"loop\", to trace where loops enter/leave.\n" );
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
	

int bfid_help( bf_code_t *dbf, int argc, char **argv ){
	int i;
	if ( argc < 2 ){
		printf( "usage: %s [command]\n", argv[0] );
		printf( "commands: \n" );
		for ( i = 0; i < bfid_cmd_no; i++ )
			printf( "\t%s\t%s\n", bfid_cmds[i].name, bfid_cmds[i].help );

		return 1;
	}

	for ( i = 0; i < bfid_cmd_no; i++ ){
		if ( strcmp( argv[1], bfid_cmds[i].name ) == 0 ){
			printf( "%s\n", bfid_cmds[i].help );
			break;
		}
	}
	
	return 0;
}

