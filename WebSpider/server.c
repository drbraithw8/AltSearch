
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/hash.h>
#include <CscNetLib/iniFile.h>
#include <CscNetLib/logger.h>
#include "inch.h"
#include "bodyChar.h"


char *progname;
void usage(const char *errStr)
{	fprintf(stderr, "Error: %s!\n", errStr);
	fprintf(stderr, "Usage: %s iniFilePath\n", progname);
	exit(1);
}


void main(int argc, char **argv)
{
	progname = argv[0];

// Resources.
	csc_ini_t *ini = NULL;
	csc_log_t *log = NULL;
	csc_str_t *errMsg = NULL;

// Are we invoked with proper args?
	if (argc != 2)
		usage("Path to configuration file required");

// Open the ini file.
	ini = csc_ini_new(); csc_assert(ini);
	int ret = csc_ini_read(ini, argv[1]);
	if (ret == -1)
	{	usage("Could not open configuration file for read");
	}

// Open the logging.
	log = csc_log_new_ini(&errMsg, ini, "server");
	if (log == NULL)
	{	usage(csc_str_charr(errMsg));
	}

// Free resources.
	if (ini)
		csc_ini_free(ini);
	if (log)
		csc_log_free(log);
	if (errMsg)
		csc_str_free(errMsg);

// Bye.
	exit(0);
}
	

