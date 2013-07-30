#include "yabid.h"

int bfid_initialized = 0;
int bfid_cmd_no = 0;
bfid_cmd_t *bfid_cmds = NULL;

bfid_brkp_t *brkp_list = NULL;
int brkp_no = 0;

int interrupted = 0;

bfid_var_t *vars = NULL;

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

int add_breakp( bfid_brkp_t **b, int type, int val ){
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

	return temp->i;
}

void del_breakp( bfid_brkp_t **b, int val ){
	bfid_brkp_t *temp, *move;
	bfid_cmd_hook_t *hook, *next;

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

			if ( temp->hooks ){
				for ( hook = temp->hooks; hook; hook = next ){
					next = hook->next;
					free( hook->cmd );
					free( hook );
				}
			}

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

int set_variable( char *name, char *val ){
	bfid_var_t *var, *move;

	// Try to find variable...
	for ( var = vars; var; var = var->next ){
		if ( strcmp( var->name, name ) == 0 ){
			free( var->val );
			var->val = strdup( val );
			return 0;
		}
	}

	var = new( bfid_var_t );
	var->name 	= strdup( name );
	var->val 	= strdup( val );

	if ( vars ){
		for ( move = vars; move->next; move = move->next );
		move->next = var;
	} else {
		vars = var;
	}

	return 0;
}

char *get_variable( char *name ){
	bfid_var_t *var;

	for ( var = vars; var; var = var->next ){
		//printf( "[debug] comparing %s == %s\n", name, var->name );
		if ( strcmp( var->name, name ) == 0 ){
			//printf( "[debug] yep, got it\n" );
			return var->val;
		}
	}

	return 0;
}

void bfid_init( ){
	// initialize variables...
	set_variable( "?", "0" );
	set_variable( "ip", "0" );
	set_variable( "ptr", "0" );

	register_bfid_func( "alias", "create an alias for a debugger command", 
			"alias [alias] [command]", bfid_alias );

	register_bfid_func( "break", "set program breakpoints", 
			"break [type] [value]", bfid_break );

	register_bfid_func( "clear", "remove program breakpoints", 
			"clear [value]", bfid_clear );

	register_bfid_func( "cont", "continue program execution in the debugger", 
			"cont", bfid_cont );

	register_bfid_func( "disas", "disassemble program instructions", 
			"disas [amount] | disas [start] [end] [(optional)filter string]", bfid_disas );

	register_bfid_func( "dump", "dump program memory", 
			"dump [amount] | dump [start] [end]", bfid_dump );

	register_bfid_func( "echo", "print text to screen", 
			"echo [text]", bfid_echo );

	register_bfid_func( "exec", "execute brainfuck code in program's context",
			"exec [code]", bfid_exec );

	register_bfid_func( "exit", "exit the debugger. Resumes execution if "
			"the debugger was started from the code.", "exit", bfid_exit );

	register_bfid_func( "help", "Get help for debugger commands", 
			"help [command]", bfid_help );

	register_bfid_func( "hook", "hook command onto breakpoint", 
			"hook [id] [code]", bfid_hook );

	register_bfid_func( "peek", "get values from memory", 
			"peek [type] [where]", bfid_peek );

	register_bfid_func( "poke", "place values in memory", 
			"poke [type] [where] [value]", bfid_poke );

	register_bfid_func( "script", "run a debugger script", 
			"script [filename]", bfid_script );

	register_bfid_func( "set", "set program variables", 
			"set [variable] [value]", bfid_set );

	register_bfid_func( "step", "step through code instructions", 
			"step [amount]", bfid_step );

	register_bfid_func( "trace", "trace program structures", 
			"trace [type]", bfid_trace );

	bfid_initialized = 1;
}

int bfid_execcmd( bf_code_t *dbf, char *str ){
	int 	i, j,
		argc, 
		slen, 
		inword,
		qoutes,
		ret = -1,
		found;
	bfid_cmd_t *move;

	found = qoutes = inword = argc = 0;
	slen = strlen( str );

	char buf[slen + 1];
	char *args[256];
	char *temp;
	strcpy( buf, str );

	for ( i = 0; i < slen && argc < 256; i++ ){
		if ( buf[i] == '"' ){
			qoutes = !qoutes;
			buf[i] = 0;

		} else if ( buf[i] != ' ' && buf[i] != '\t' && !inword ){
			args[argc++] = buf + i;
			inword = 1;

		} else if (( buf[i] == ' ' || buf[i] == '\t' ) && !qoutes ){
			buf[i] = 0;
			inword = 0;

		} else if ( buf[i] == '\n' ){
			buf[i] = 0;
			inword = 0;
		}
	}

	if ( qoutes ){
		printf( "Error: unclosed qoutes\n" );
		return 0;
	}

	for ( i = 0; i < argc; i++ ){
		if ( args[i][0] == '$' ){
			temp = get_variable( args[i] + 1 );
			if ( !temp ){
				printf( "Error: variable \"%s\" not found\n", args[i] + 1 );
				return 0;
			}
			args[i] = temp;
		}
	}

	for ( move = bfid_cmds; move; move = move->next ){
		if ( strcmp( args[0], move->name ) == 0 ){
			found = 1;
			//return move->func( dbf, argc, args );
			ret = move->func( dbf, argc, args );
			break;
		}
	}

	char somebuf[32];
	sprintf( somebuf, "%u\0", ret );

	set_variable( "?", somebuf );
	return ret;
}

int bf_debugger( bf_code_t *bf ){
	bf_code_t *dbf = bf;
	//bf_code_t *dbf = new( bf_code_t );
	//memcpy( dbf, bf, sizeof( bf_code_t ));
	char buf[256];
	int lret = 0;
	int broke;

	bfid_brkp_t *temp;
	bfid_cmd_hook_t *hook;

	if ( !bfid_initialized )
		bfid_init( );

	dbf->debugging = 1;
	interrupted = 1;
	signal( SIGINT, (__sighandler_t)debug_signal );

	printf( "[>] yabi debugger v0.99\n" );
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
					broke = 0;
					if ( temp->type == BRK_IP && temp->val == dbf->ip ){
						interrupted = broke = 1;

					} else if ( temp->type == BRK_MEM && temp->val == dbf->ptr ){
						interrupted = broke = 1;

					} else if ( 	temp->type == BRK_INSTR && 
							temp->val == dbf->code[ dbf->ip ]){
						interrupted = broke = 1;
					}

					if ( broke ){
						sprintf( buf, "%u\0", dbf->ip );
						set_variable( "ip", buf );

						sprintf( buf, "%u\0", dbf->ptr );
						set_variable( "ptr", buf );
				
						for ( hook = temp->hooks; hook; hook = hook->next ){
							if ( bfid_execcmd( dbf, hook->cmd ) < 0 )
								printf( "[ ] Unknown hook command: \"%s\"\n", hook->cmd );
						}
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

	char buf[32];
	sprintf( buf, "%u\0", dbf->ip );
	set_variable( "ip", buf );

	sprintf( buf, "%u\0", dbf->ptr );
	set_variable( "ptr", buf );

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

	set_variable( argv[1], argv[2] );

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

int bfid_hook( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 3 ){
		printf( "usage: %s [id] [code]\n", argv[0] );
		printf( "	hook a command to an existing breakpoint.\n" );
		return 1;
	}

	int id = atoi( argv[1] );

	bfid_brkp_t *temp, *move = NULL;
	bfid_cmd_hook_t *hook, *htemp;

	for ( temp = brkp_list; temp; temp = temp->next ){
		if ( temp->i == id ){
			move = temp;
			break;
		}
	}

	if ( !move ){
		printf( "Breakpoint not found.\n" );
		return 2;
	}

	hook = new( bfid_cmd_hook_t );
	hook->cmd = strdup( argv[2] );
	hook->next = NULL;

	if ( !temp->hooks ){
		temp->hooks = hook;
	} else {
		for ( htemp = temp->hooks; htemp->next; htemp = htemp->next );
		htemp->next = hook;
	}

	return 0;
}

int bfid_break( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 3 && !( argc == 2 && strcmp( argv[1], "list" ) == 0)){
		printf( "usage: %s [type] [value]\n", argv[0] );
		printf( "types:	ip 	- numeric ip position\n"
			"	mem	- memory address.\n"
			"	in	- program instruction\n"
			"	list	- list all breakpoints.\n" );

		return 1;
	}

	int type = 0, val = 0, ret = 0;

	if ( strcmp( argv[1], "ip" ) == 0 ){
		val = atoi( argv[2] );

		ret = add_breakp( &brkp_list, BRK_IP, val );

	} else if ( strcmp( argv[1], "mem" ) == 0 ){
		val = atoi( argv[2] );

		ret = add_breakp( &brkp_list, BRK_MEM, val );

	} else if ( strcmp( argv[1], "in" ) == 0 ){
		val = argv[2][0];

		ret = add_breakp( &brkp_list, BRK_INSTR, val );

	} else if ( strcmp( argv[1], "list" ) == 0 ){
		bfid_brkp_t *temp;
		bfid_cmd_hook_t *hook;

		char *tstr[] = { "ip", "mem", "in", "wut" };

		for ( temp = brkp_list; temp; temp = temp->next ){
			printf( "[%d]\t%s:\t%d\n", temp->i, tstr[ temp->type ], temp->val );
			for ( hook = temp->hooks; hook; hook = hook->next )
				printf( "\t`- hook: %s\n", hook->cmd );
		}

	} else {
		printf( "Invalid type.\n" );
	}

	return ret;
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

int bfid_poke( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 4 ){
		printf( "usage: %s [type] [where] [value]\n"
			"[value] must be a decimal number.\n" 
			"types:	ip 	- replace instruction at [where]"
			"	mem	- set value in memory at [where]", argv[0] );

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

int bfid_peek( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 3 ){
		printf( "usage: %s [type] [where]\n"
			"types:	ip 	- replace instruction at [where]"
			"	mem	- set value in memory at [where]", argv[0] );

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

int bfid_exec( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 2 ){
		printf( "usage: %s [code]\n", argv[0] );
		return 1;
	}

	int codesize = dbf->codesize;
	int ip = dbf->ip;
	char *code = dbf->code;

	dbf->ip = 0;
	dbf->codesize = strlen( argv[1] );
	dbf->code = argv[1];

	while ( dbf->ip < dbf->codesize )
		bf_step( dbf );

	dbf->codesize = codesize;
	dbf->ip = ip;
	dbf->code = code;

	return 0;
}

int bfid_echo( bf_code_t *dbf, int argc, char **argv ){
	int i;
	for ( i = 1; i < argc; i++ )
		printf( "%s ", argv[i] );

	putchar( '\n' );
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


