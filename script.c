#include "yabid.h"

int bfid_cmd_no = 0;
bfid_cmd_t *bfid_cmds = NULL;

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

int bfid_execcmd( bf_code_t *dbf, char *str ){
	int 	i, j,
		argc, 
		slen, 
		inword,
		qoutes,
		ret = -1,
		found;
	bfid_cmd_t *move;
	char somebuf[32];
	found = qoutes = inword = argc = 0;
	slen = strlen( str );

	char buf[slen + 1];
	char *args[256];
	char *temp;
	strcpy( buf, str );

	if ( str[0] == '#' ){
		ret = 0;
		goto end;
	}

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
				ret = 0;
				goto end;
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


end:
	sprintf( somebuf, "%u\0", ret );

	set_variable( "?", somebuf );
	return ret;
}

int bfid_script( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 2 )
		printf( "usage: %s [filename]\n", argv[0] );

	FILE *fp;

	if (!( fp = fopen( argv[1], "r" )))
		return 1;

	char buf[256];
	while ( 1 ){
		fgets( buf, 256, fp );
		if ( feof( fp ))
			break;
		bfid_execcmd( dbf, buf );
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

