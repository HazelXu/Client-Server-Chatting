/* =========================================================================
 * proj1.c :
 * Starts off as a server or a client on a given port
 * Created for CSE 489 Spring 2014 Programming Assignment 1
 * @author Lidan Xu
 * @created 9 March 2015
 * ========================================================================*/
#include <stdlib.h>
#include <string.h>
#include "REPL.h"

int main(int argc, char *argv[])
{

	if (strcmp(argv[1],"s") == 0)
		prepareServer(argv[2]);
	else if (strcmp(argv[1], "c") == 0)
		prepareClient(argv[2]);
	else
		Error("Syntax Error.");
	return EXIT_SUCCESS;
}

