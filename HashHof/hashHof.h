
#include <stdio.h>
#include <CscNetLib/hashStr.h>


// HOF: Hash Only Fixed:  Fixed size hash table without keys or data:-
// 
// *	Only the hash of the key is kept, in order to minimize space.  A
// 		collision would cause a false positive of a key being present in
// 		the table.  So this hash table will  allow us to detect the presence
// 		or not of a key in the table with some risk of a false positive.
// 
// *	Table has fixed size.  The number of elements in the table must always
// 		be less than the size, and preferably less than or equal to 1/2
// 		the size.  Hence this is NOT a good hash table if you don't know
// 		what the maximum number of keys would be.  If you do know the
// 		maximum number of keys, then choose the table size to be twice that.
// 
//	*	The R program "collisionRisk.r", indicates that if the number of
// 		elements is the square root of the space (by space I refer to
// 		the number distinct potential elements), then the probability of there
// 		being one or more collisions is approximately 0.4, or a 60%
// 		certainty of there being no collisions, assuming a good hash function.
// 		That means we could use 6.5e4, 4e9, 2e19 hashes using 32, 64 and
// 		128 bit hashes respectively, and have a 40% chance of having
// 		one or more collisions.
// 
// 		My gut feeling is that a 40% chance of having one or more
// 		collisions is acceptable for the altsearch project.  Using a
// 		hash size that is larger than required will waste in-memory and
// 		on-disk space.
// 
// 		Alternatively, if the number of hashes is one hundredth of the
// 		square root of the space  then we have a half of a percent
// 		chance of having one or more collisions.  That means we could
// 		use 650, 4e7, 2e17 hashes using 32, 64 and 128 bit hashes
// 		respectively, and have a 0.5% chance of having one or more
// 		collisions.
// 
// 	*	0 (the null hash) is used to indicate an empty slot, therefore
// 		if a key hashes to zero, there is a problem.  To solve this, our
// 		good hash function is modified so that if value of a key is
// 		zero, then we use the value 1 as the hash value.  So the hash
// 		value 1 is twice as likely to be collided as any other hash
// 		value.
// 
// 	*	The hash value is chosen at compile time, and can be 32 bit, 64
// 		bit or 128 bit value.  In order to keep your code free of
// 		dependency on the size of the hash, you need to use the
// 		following macros:-
// 		*	hashHof_hVal_t  // The type of the hash.
// 		*	hashHof_hVal_word_t  // A 64bit representation of the hash.
// 		*	hashHof_hVal_isZero(h) // If null hash, then no hash present.
// 		*	hashHof_hVal_isEq(h,v) // Are two hashes equal.
// 		*	hashHof_hVal_setToZero(h) // Set to the null hash.
// 		*	hashHof_hVal_toWord(h) (h)  // Get a 64 digest of the hash. 
// 		*	hashHof_hVal_str(s)   // Make a hash from a string.
// 		*	hashHof_hVal_setVal(h,v) // Set the hash to a 64 bit value.
// 
// HOF: Has two forms:-
// 
//	*	An in memory based form that:-
// 		*	Has no persistance.
// 		*	Is very fast.
// 		*	Is limited by available memory.
// 
//	*	A file based structure that:-
// 		*	Has persistance.
// 		*	Is much slower
// 		*	Is not limited by available memory.


typedef struct hashHof_t hashHof_t;


// ---------- Code dependent on size of hash value ----------
 
// #define hashHof_hBits 32
// #define hashHof_hBits 64
#define hashHof_hBits 128
 
#if hashHof_hBits == 32    // 32 bit
 
typedef uint32_t hashHof_hVal_t;
typedef uint32_t hashHof_hVal_word_t;
#define hashHof_hVal_isZero(h) ((h) == 0)
#define hashHof_hVal_isEq(h,v) ((h) == (v))
#define hashHof_hVal_setToZero(h) ((h) = 0)
#define hashHof_hVal_toWord(h) (h)
#define hashHof_hVal_str(s) ((uint32_t)csc_hash_str((void*)s))
#define hashHof_hVal_setVal(h,v) ((h) = v)
 
#elif hashHof_hBits == 64    // 64 bit
 
typedef uint64_t hashHof_hVal_t;
typedef uint64_t hashHof_hVal_word_t;
#define hashHof_hVal_isZero(h) ((h) == 0)
#define hashHof_hVal_isEq(h,v) ((h) == (v))
#define hashHof_hVal_setToZero(h) ((h) = 0)
#define hashHof_hVal_toWord(h) (h)
#define hashHof_hVal_str(s) ((uint64_t)csc_hash_str((void*)s))
#define hashHof_hVal_setVal(h,v) ((h) = v)
 
#elif hashHof_hBits == 128    // 128 bit
 
typedef csc_hash_hval128_t hashHof_hVal_t;
typedef uint64_t hashHof_hVal_word_t;
#define hashHof_hVal_isZero(h) ((h).h0==0 && (h).h1==0)
#define hashHof_hVal_isEq(h,v) ((h).h0==(v).h0 && (h).h1==(v).h1)
#define hashHof_hVal_setToZero(h) ((h).h0=0, (h).h1=0)
#define hashHof_hVal_toWord(h) ((h).h0 ^ (h).h1)
#define hashHof_hVal_str(s) csc_hash_str128(s)
#define hashHof_hVal_setVal(h,v) (h.h0=0, h.h1=v)
 
#endif

#define hashHof_hBytes (hashHof_hBits/8)


// ------------------- Constructors ---------------

	// For a file based path, 'filePath' will be the name of the file that
	// is, or will become, the table.
hashHof_t *hashHof_new_fileNew(uint64_t tblSiz, char *filePath);
hashHof_t *hashHof_new_fileExists(char *filePath);

    // In-memory constructor.
hashHof_t *hashHof_new_inMem(uint64_t tblSiz);

    // Drag a hash file into memory.
hashHof_t *hashHof_new_fileMem(uint64_t newTblSiz, char *filePath);


// ------------------- Destructor ------------------

void hashHof_free(hashHof_t *hof);


// ----------------- Methods --------------------------------

// Add a hash 'hval' to the table.
// Returns 0 on success.
// Returns -1 if cant add because hash 'hval' exists in table.
// Returns -2 on file error.
int hashHof_add(hashHof_t *hof, hashHof_hVal_t hval);

// Determine whether key with hash 'hval' exists in table.
// Returns 0 if the hash 'hval' does not exist in the table.
// Returns -1 if the hash 'hval' exists in the table.
// Returns -2 on file IO error.
int hashHof_has(hashHof_t *hof, hashHof_hVal_t hval);


// ----------------- Misc --------------------------------

// The size of the hash table.
uint64_t hashHof_tblSiz(hashHof_t *hof);

// The number of elements in the table.
// Ideally should be not more than half the size of the table.
uint64_t hashHof_nElem(hashHof_t *hof);

// The hash function being used.
hashHof_hVal_t hashFunc(const char *str);



