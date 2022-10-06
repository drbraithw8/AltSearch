
#include <assert.h>
#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/hash.h>

#include "hashHof.h"

// HOF: Hash Only Fixed:  Fixed size hash table without keys or data.


typedef struct hashHof_t
{	hashHof_hVal_t *tbl;
	FILE *ftbl;
	int64_t tblSiz;
	int64_t nElem;
	int isDirty;
} hashHof_t;



// ------------------- Constructors ---------------

hashHof_t *hashHof_new_inMem(uint64_t tblSiz)
{	hashHof_t *hof = csc_allocOne(hashHof_t);
	hof->tbl = csc_ck_calloc(sizeof(hashHof_hVal_t) * (int)tblSiz);
	hof->ftbl = NULL;
	hof->tblSiz = tblSiz;
	hof->nElem = 0;
	hof->isDirty = 0;
	return hof;
}


static csc_bool_t writeEmptyFile(uint64_t tblSiz, char *filePath)
{	FILE *fout = NULL;
 
	uint64_t elSize = hashHof_hBits ; // The inital number of elements.
	uint64_t nelem = 0; // The inital number of elements.
 
// Handy value for writing a lot.
	hashHof_hVal_t hZero;
	hashHof_hVal_setToZero(hZero);
 
// Open the file.
	fout = fopen(filePath, "wb");
	if (fout == NULL)
		return csc_FALSE;
 
// Write out the file header.
	fwrite(&elSize, sizeof(uint64_t), 1, fout);  // Size of each element.
	fwrite(&tblSiz, sizeof(uint64_t), 1, fout);  // Table size.
	fwrite(&nelem, sizeof(uint64_t), 1, fout);   // Current element count.
 
// Write the elements.
	for (uint64_t i=0; i<tblSiz; i++)
	{	fwrite(&hZero, sizeof(hZero), 1, fout);
	}
 
// Free Resources.
	fclose(fout);
 
// Return the structure.
	return csc_TRUE;
}


hashHof_t *hashHof_new_fileExists(char *filePath)
{	int retVal;
	hashHof_t *hof;
 
// Table parameters to be read in.
	uint64_t elSize;
	uint64_t tblSiz;
	uint64_t nElem;

// Resources.
	FILE *ftbl = NULL;
 
// Open the file.
	ftbl = fopen(filePath, "r+b");
	if (ftbl == NULL)
		goto cleanup;

// Read in the element size.
	retVal = fread(&elSize, sizeof(uint64_t), 1, ftbl);
	if (retVal!=1 || elSize!=hashHof_hBits)
		goto cleanup;
 
// Read in the table size.
	retVal = fread(&tblSiz, sizeof(uint64_t), 1, ftbl);
	if (retVal != 1)
		goto cleanup;
 
// Read in the number of elements.
	retVal = fread(&nElem, sizeof(uint64_t), 1, ftbl);
	if (retVal != 1)
		goto cleanup;
	if (nElem >= tblSiz)
		goto cleanup;
 
// Create the structure.
	hof = csc_allocOne(hashHof_t);
	hof->nElem = nElem;
	hof->tblSiz = tblSiz;
	hof->tbl= NULL;
	hof->ftbl= ftbl;
	hof->isDirty = 0;
	return hof;
 
cleanup:
	assert(csc_FALSE);
	if (ftbl)
		fclose(ftbl);
	return NULL;
}


hashHof_t *hashHof_new_fileNew(uint64_t tblSiz, char *filePath)
{	
// Create the file.
	if (!writeEmptyFile(tblSiz, filePath))
		return NULL;
 
// Create the structure.
	return hashHof_new_fileExists(filePath);
}


// ------------------- Destructor ------------------

void hashHof_flush(hashHof_t *hof)
{	if (hof->ftbl)
	{	if (hof->isDirty)
		{	int retVal;

		// Seek the file position.
			retVal = fseek(hof->ftbl, (2)*sizeof(uint64_t), SEEK_SET);
			assert(retVal == 0);
	 
		// Write the current number of elements.
			retVal = fwrite(&hof->nElem, sizeof(hof->nElem), 1, hof->ftbl);
			assert(retVal == 1);
		}
		fflush(hof->ftbl);
		hof->isDirty = 0;
	}
}

void hashHof_free(hashHof_t *hof)
{	if (hof->tbl)
		free(hof->tbl);
	if (hof->ftbl)
	{	hashHof_flush(hof);
		fclose(hof->ftbl);
	}
	free(hof);
}


// ----------------- Methods --------------------------------


int hashHof_add(hashHof_t *hof, hashHof_hVal_t hval)
{	
	uint64_t tblSiz = hof->tblSiz;
	uint64_t ndx = hashHof_hVal_toWord(hval) % tblSiz;
	uint64_t bytePos;
	int retVal;
 
	hashHof_hVal_t val;
	hashHof_hVal_t *tbl = hof->tbl;
 
	for (;;)
	{
	// Get the value from the index.
		if (tbl)
		{	val = tbl[ndx];
		}
		else
		{ 
		// Seek the file position.
			bytePos = 3*sizeof(uint64_t) + ndx*hashHof_hBytes;
			retVal = fseek(hof->ftbl, bytePos, SEEK_SET);
			if (retVal)
				return -2;
 
		// Read the hash value.
			retVal = fread(&val, hashHof_hBytes, 1, hof->ftbl);
			if (retVal != 1)
				return -2;
		}
 
	// Check if our probing is finished.
		if (hashHof_hVal_isZero(val) || hashHof_hVal_isEq(val,hval))
			break;
 
	// Next index to probe.
		ndx++;
		if (ndx == tblSiz)
			ndx = 0;
	}
 
// Is the hash value already there?
	if (hashHof_hVal_isEq(val,hval))
	{	return -1;
	}
 
// Add the new element here.
	if (tbl)
	{	tbl[ndx] = hval;
		hof->nElem++;
		assert(hof->nElem < tblSiz);
	}
	else
	{ 
	// Seek the file position.
		bytePos = 3*sizeof(uint64_t) + ndx*hashHof_hBytes;
		retVal = fseek(hof->ftbl, bytePos, SEEK_SET);
		if (retVal)
			return -2;
 
	// Write the hash value.
		retVal = fwrite(&hval, sizeof(hval), 1, hof->ftbl);
		if (retVal != 1)
			return -2;
 
	// Note that a record has been added.
		hof->nElem++;
		assert(hof->nElem < tblSiz);
		hof->isDirty = 1;
	}
 
// Return success.
	return 0;
}


int hashHof_has(hashHof_t *hof, hashHof_hVal_t hval)
{
	uint64_t tblSiz = hof->tblSiz;
	uint64_t ndx = hashHof_hVal_toWord(hval) % tblSiz;
	uint64_t bytePos;
	int retVal;
 
	hashHof_hVal_t *tbl = hof->tbl;
	hashHof_hVal_t val;
 
	for (;;)
	{
	// Get the value from the index.
		if (tbl)
		{	val = tbl[ndx];
		}
		else
		{ 
		// Seek the file position.
			bytePos = 3*sizeof(uint64_t) + ndx*hashHof_hBytes;
			retVal = fseek(hof->ftbl, bytePos, SEEK_SET);
			if (retVal)
				return -2;
 
		// Read the word.
			retVal = fread(&val, sizeof(val), 1, hof->ftbl);
			if (retVal != 1)
				return -2;
		}
 
	// Check if our probing is finished.
		if (hashHof_hVal_isZero(val) || hashHof_hVal_isEq(hval,val))
			break;
 
	// Next index to probe.
		ndx++;
		if (ndx == tblSiz)
			ndx = 0;
	}
 
// Bye.
	if (hashHof_hVal_isEq(hval,val))
		return -1;
	else
		return 0;
}



uint64_t hashHof_nElem(hashHof_t *hof)
{	return hof->nElem;
}


uint64_t hashHof_tblSiz(hashHof_t *hof)
{	return hof->tblSiz;
}


hashHof_hVal_t hashFunc(const char *str)
{	hashHof_hVal_t hval = hashHof_hVal_str(str);
	if (hashHof_hVal_isZero(hval))
		hashHof_hVal_setVal(hval,1);
	return hval;
}


#include <CscNetLib/fileProperties.h>
#include <CscNetLib/isvalid.h>

void usage(char *progName)
{	fprintf(stderr, "Usage: %s [path] [nrecords]\n%s\n", progName,
"If you specify only the path, then this program will attempt to open\n"
"that path as an existing hash table file.  If you specify only the\n"
"number of records, nrecords, then this program will open an internal\n"
"hash table.  If you specify both path and nrecords, then a new hash\n"
"table file will be created with that path and the number of records\n"
"will be nrecords.  Other combinations are invalid.\n"
            );
	exit(1);
}


void main(int argc, char **argv)
{	char buf[256];
	int nRecords = -1;
	char *path = NULL;
	int retVal;
	csc_bool_t isPopulate = csc_FALSE;

	hashHof_t *hof = NULL;
	hashHof_hVal_t hval;

// Process arguments.
	if (argc == 0)
	{	fprintf(stderr, "%s", "Error: missing argument!\n");
		usage(argv[0]);
		exit(1);
	}
	for (int i=1; i<argc; i++)
	{	if (csc_isValid_int(argv[i]))
		{	if (nRecords != -1)
			{	fprintf(stderr, "%s", "Error: nrecords specified twice!\n");
				usage(argv[0]);
				exit(1);
			}
			else
			{	nRecords = atoi(argv[i]);
			}
		}
		else if (csc_isValid_decentPath(argv[i]))
		{	if (path != NULL)
			{	fprintf(stderr, "%s", "Error: path specified twice!\n");
				usage(argv[0]);
				exit(1);
			}
			else
			{	path = argv[i];
			}
		}
		else
		{	fprintf(stderr, "Error: bad argument %s!\n", argv[i]);
			usage(argv[0]);
			exit(1);
		}
	}

// Call appropriate constructor.
	if (path && nRecords!=-1)
	{	hof = hashHof_new_fileNew(nRecords, path);
		if (hof == NULL)
		{	fprintf(stderr, "Error: failed to open %s!\n", path);
			usage(argv[0]);
			exit(1);
		}
		isPopulate = csc_TRUE;
	}
	else if (nRecords != -1)
	{	hof = hashHof_new_inMem(nRecords);
		if (hof == NULL)
		{	fprintf(stderr, "Error: failed to open inMemory!\n");
			usage(argv[0]);
			exit(1);
		}
		isPopulate = csc_TRUE;
	}
	else if (path)
	{	hof = hashHof_new_fileExists(path);
		if (hof == NULL)
		{	fprintf(stderr, "Error: failed to open %s!\n", path);
			usage(argv[0]);
			exit(1);
		}
		isPopulate = csc_FALSE;
		nRecords = hashHof_tblSiz(hof);
		fprintf(stderr, "nRecords=%d\n", nRecords);
	}

// Populate hash table.
	if (isPopulate)
	{	for (int i=0; i<(nRecords/2); i++)
		{	sprintf(buf, "%d", i);
			hval = hashFunc(buf);
			retVal = hashHof_add(hof,hval);
			if (retVal != 0)
			{	fprintf(stderr, "hashHof_add(%s) returned %d!\n", buf, retVal);
				assert(retVal == -1);
			}
		}
	}
 
// Test out populated table.
	for (int i=0; i<(nRecords/2); i++)
	{	sprintf(buf, "%d", i);
		hval = hashFunc(buf);
 
	// Check added value is present.
		retVal = hashHof_has(hof,hval);
 		if (retVal != -1)
		{	fprintf(stderr, "hashHof_has(%s) returned %d!\n", buf, retVal);
			assert(retVal == 0);
		}
 
	// Check not added value is not present.
		hashHof_hVal_word_t word = hashHof_hVal_toWord(hval);
		word++;
		hashHof_hVal_setVal(hval, word);
		retVal = hashHof_has(hof,hval);
 		if (retVal != 0)
		{	fprintf( stderr, "hashHof_has(%lu) returned %d!\n"
				   , (unsigned long)word, retVal
				   );
			assert(retVal == -1);
		}
	}

// Bye. Check for memory leaks.
	hashHof_free(hof);
  	csc_mck_print(stderr);
	exit(0);
}
 

// make clean
// make
// ./testHof 20
// ./testHof 200000
// ./testHof temp1 20
// ./testHof temp1
// rm temp1
// ./testHof temp1 200000
// ./testHof temp1


