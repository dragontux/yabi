#include "yabi.h"

typedef int (*bfid_func_t)( bf_code_t *dbf, int argc, char **argv );
typedef struct bfid_command {
	char *name;
	char *help;
	char *usage;
	bfid_func_t func;
	struct bfid_command *next;
} bfid_cmd_t;

int bfid_initialized = 0;
int bfid_cmd_no = 0;
bfid_cmd_t *bfid_cmds = NULL;

bfid_brkp_t *brkp_list = NULL;
int brkp_no = 0;

int interrupted = 0;

int bfid_step( bf_code_t *dbf, int argc, char **argv );
int bfid_dump( bf_code_t *dbf, int argc, char **argv );
int bfid_exit( bf_code_t *dbf, int argc, char **argv );
int bfid_cont( bf_code_t *dbf, int argc, char **argv );
int bfid_status( bf_code_t *dbf, int argc, char **argv );
int bfid_set( bf_code_t *dbf, int argc, char **argv ); 
int bfid_help( bf_code_t *dbf, int argc, char **argv );
int bfid_hook( bf_code_t *dbf, int argc, char **argv );
int bfid_disas( bf_code_t *dbf, int argc, char **argv );
int bfid_trace( bf_code_t *dbf, int argc, char **argv );
int bfid_break( bf_code_t *dbf, int argc, char **argv );
int bfid_clear( bf_code_t *dbf, int argc, char **argv );
int bfid_script( bf_code_t *dbf, int argc, char **argv );
int bfid_alias( bf_code_t *dbf, int argc, char **argv );

int debug_signal( int s );

int debug_signal( int s ){
	interrupted = 1;

	return 0;
}

void register_bfid_func( char *name, char *help, 
			 char *usage, bfid_func_t function )
{

	bfid_cmd_t 	*move = new( bfid_cmd_t ),
			*temp;

	move->name 	= name;
	move->help 	= help;
	move->usage 	= usage;
	move->func 	= function;
	move->next	= NULL;

	if ( !bfid_cmds )
		bfid_cmds = move;
	else {
		for ( temp = bfid_cmds; temp->next; temp = temp->next );
		temp->next = move;
	}
}

void add_breakp( bfid_brkp_t **b, int type, int val ){
	bfid_brkp_t *temp, *move, *prev;

	temp = new( bfid_brkp_t );
	temp->val = val;
	temp->i = brkp_no;
	temp->type = type;
	brkp_no++;

	if ( *b ){
		for ( prev = 0, move = *b; move->next; prev = move, move = move->next );
		move->next = temp;
		move->prev = prev;
	} else 
		*b = temp;

	//printf( "Added instruction breakpoint %d.\n", temp->i );

	return;
}

void del_breakp( bfid_brkp_t **b, int val ){
	bfid_brkp_t *temp, *move;
	int found = 0;

	if ( !*b ){
		printf( "No breakpoints.\n" );
		return;
	}

	for ( temp = *b; temp; temp = temp->next ){
		if ( temp->i == val ){
			if ( temp->prev )
				temp->prev->next = temp->next;
			if ( temp->next )
				temp->next->prev = temp->prev;

			if ( temp == *b )
				*b = temp->next; 

			free( temp );

			//printf( "Deleted instruction breakpoint %d.\n", val );
			found = 1;
			break;
		}
	}
	
	if ( !found )
		printf( "Invalid breakpoint.\n" );

	return;
}

void bfid_init( ){
	// look at program data
	register_bfid_func( "dump", "dump program memory", 
			"dump [amount] | dump [start] [end]", bfid_dump );
	register_bfid_func( "disas", "disassemble program instructions", 
			"disas [amount] | disas [start] [end] [(optional)filter string]", bfid_disas );
	register_bfid_func( "status", "information about the program's state", 
			"status", bfid_status );

	// manage program flow
	register_bfid_func( "break", "set program breakpoints", 
			"break [type] [value]", bfid_break );
	register_bfid_func( "clear", "remove program breakpoints", 
			"clear [value]", bfid_clear );
	register_bfid_func( "hook", "hook onto program locations (temporary break point)", 
			"hook [type] [value]", bfid_hook );
	register_bfid_func( "trace", "trace program structures", 
			"trace [type]", bfid_trace );
	register_bfid_func( "cont", "continue program execution in the debugger", 
			"cont", bfid_cont );
	register_bfid_func( "step", "step through code instructions", 
			"step [amount]", bfid_step );
	register_bfid_func( "set", "set program variables", 
			"set [variable] [value]", bfid_set );

	register_bfid_func( "script", "run a debugger script", 
			"script [filename]", bfid_script );
	register_bfid_func( "alias", "create an alias for a debugger command", 
			"alias [alias] [command]", bfid_alias );

	register_bfid_func( "exit", "exit the debugger. Resumes execution if "
			"the debugger was started from the code.", "exit", bfid_exit );

	// help function
	register_bfid_func( "help", "Get help for debugger commands", "help [command]", bfid_help );
	register_bfid_func( "man", "Get help for debugger commands", "man [command]", bfid_help );

	bfid_initialized = 1;
}

int bfid_execcmd( bf_code_t *dbf, char *str ){
	int i, argc, slen, found;
	bfid_cmd_t *move;

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

	for ( move = bfid_cmds; move; move = move->next ){
		if ( strcmp( args[0], move->name ) == 0 ){
			found = 1;
			return move->func( dbf, argc, args );
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
	bfid_brkp_t *temp;

	if ( !bfid_initialized )
		bfid_init( );

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
			if ( dbf->ip < dbf->codesize ){
				bf_step( dbf );

				for ( temp = brkp_list; temp; temp = temp->next ){
					if ( temp->type == BRK_IP && temp->val == dbf->ip ){
						printf( "Caught breakpoint %d: ip=%d\n", temp->i, dbf->ip );
						interrupted = 1;
						break;
					} else if ( temp->type == BRK_MEM && temp->val == dbf->ptr ){
						printf( "Caught breakpoint %d: ptr=%d\n", temp->i, dbf->ptr );
						interrupted = 1;
						break;
					} else if ( temp->type == BRK_INSTR && 
							temp->val == dbf->code[ dbf->ip ]){
						printf( "Caught breakpoint %d: instruction='%c'\n", temp->i, temp->val );
						interrupted = 1;
						break;
					}
				}

			} else {
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
		//printf( "[%c] stepping...\n", c );
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

		if ( dbf->mem[i] )
			printf( "%02x ", dbf->mem[i] & 0xff );
		else
			printf( "   " );
	}
	printf( "\n" );

	return 0;
}

int bfid_exit( bf_code_t *dbf, int argc, char **argv ){
	dbf->debugging = 0;
	signal( SIGINT, SIG_DFL );

	return 0;
}

int bfid_cont( bf_code_t *dbf, int argc, char **argv ){
	interrupted = 0;

	return 0;
}

int bfid_status( bf_code_t *dbf, int argc, char **argv ){
	printf( "\tip: %d, ptr: %d, depth: %d, total insructions executed: %d\n", 
		dbf->ip, dbf->ptr, dbf->depth, dbf->execed );

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

	interrupted = 0;
	switch ( type ){
		case 1:
			val = atoi( argv[2] );
			while ( dbf->ip != val && dbf->ip < dbf->codesize && !interrupted )
				bf_step( dbf );
			break;
		case 2:
			val = argv[2][0];
			while ( dbf->code[ dbf->ip ] != val && dbf->ip < dbf->codesize && !interrupted )
				bf_step( dbf );
			break;
		case 3:
			val = atoi( argv[2] );
			while( dbf->ptr != val && dbf->ip != dbf->codesize && !interrupted )
				bf_step( dbf );
			break;
	}
	interrupted = 1;

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

int bfid_break( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 3 && !( argc == 2 && strcmp( argv[1], "list" ) == 0)){
		printf( "usage: %s [type] [value]\n", argv[0] );
		printf( "Type can be \"ip\", a numeric ip position, or \"mem\", a memory address.\n"
			"\"list\" can be used to list all breakpoints.\n" );

		return 1;
	}

	int type = 0, val = 0;

	if ( strcmp( argv[1], "ip" ) == 0 ){
		val = atoi( argv[2] );

		add_breakp( &brkp_list, BRK_IP, val );
	} else if ( strcmp( argv[1], "mem" ) == 0 ){
		val = atoi( argv[2] );

		add_breakp( &brkp_list, BRK_MEM, val );
	} else if ( strcmp( argv[1], "in" ) == 0 ){
		val = argv[2][0];

		add_breakp( &brkp_list, BRK_INSTR, val );
	} else if ( strcmp( argv[1], "list" ) == 0 ){
		bfid_brkp_t *temp;
		char *tstr[] = { "ip", "mem", "in", "wut" };
		for ( temp = brkp_list; temp; temp = temp->next )
			printf( "[%d]\t%s:\t%d\n", temp->i, tstr[ temp->type ], temp->val );

	} else {
		printf( "Invalid type.\n" );
		return 2;
	}

	return 0;
}

int bfid_clear( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 2 ){
		printf( "usage: %s [value]\n", argv[0] );
		return 1;
	}

	int val = 0;

	val = atoi( argv[1] );
	del_breakp( &brkp_list, val );

	return 0;
}

int bfid_script( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 2 )
		printf( "usage: %s [filename]\n", argv[0] );

	FILE *fp;

	if (!( fp = fopen( argv[1], "r" )))
		return 1;

	char buf[256];
	while ( !feof( fp )){
		fgets( buf, 256, fp );
		bfid_execcmd( dbf, buf );
		printf( "[debug statement]\n" );
	}

	return 0;
}

int bfid_alias( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 3 ){
		printf( "usage: %s [alias] [command]\n", argv[0] );
		printf( "Creates an alias for a debugger command.\n" );
		return 1;
	}

	int i;
	bfid_cmd_t *temp = NULL, *move;

	for ( move = bfid_cmds; move; move = move->next ){
		if ( strcmp( move->name, argv[1] ) == 0 ){
			printf( "Command \"%s\" already exists\n", argv[1] );
			return 2;
		} else if ( strcmp( move->name, argv[2] ) == 0 ){
			temp = move;
		}
	}

	if ( !temp ){
		printf( "Invalid command name.\n" );
		return 1;
	}

	printf( "Registering \"%s\"...\n", argv[1] );
	char *buf = strdup( argv[1] );

	register_bfid_func( buf, temp->help, temp->usage, temp->func );
	return 0;
}


int bfid_help( bf_code_t *dbf, int argc, char **argv ){
	bfid_cmd_t *move;
	int i;
	if ( argc < 2 ){
		printf( "usage: %s [command]\n", argv[0] );
		printf( "commands: \n" );
		for ( move = bfid_cmds; move; move = move->next )
			printf( "\t%s\t%s\n", move->name, move->help );

		return 1;
	}

	for ( move = bfid_cmds; move; move = move->next ){
		if ( strcmp( argv[1], move->name ) == 0 ){
			printf( "%s\n", move->help );
			printf( "Usage: %s\n", move->usage );
			break;
		}
	}
	
	return 0;
}

