#include "yabid.h"

bfid_var_t *vars = NULL;

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

	// Nope, make new variable
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
		if ( strcmp( var->name, name ) == 0 ){
			return var->val;
		}
	}

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

