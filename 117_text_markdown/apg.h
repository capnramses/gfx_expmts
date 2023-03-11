/* apg.h  Author's generic C utility functions.
Author:   Anton Gerdelan  antongerdelan.net
Licence:  See bottom of this file.
Language: C89 interface, C99 implementation.

Version History and Copyright
-----------------------------
  1.11  - 11 Jan 2022. Fixed a crash bug when failing to read an entire file.
  1.10  - xx Sep 2022. Cross-platform directory/filesystem functions.
  1.9   - 10 Jun 2022. Large file support in file I/O.
  1.8.1 - 28 Mar 2022. Casting precision fix to gbfs.
  1.8   - 27 Mar 2022. Greedy BFS uses 64-bit integers (suited a project I used it in).
  1.7   - 22 Mar 2022. Greedy BFS speed improvement using bsearch & memmove suffle.
  1.6   - 13 Mar 2022. Greedy Best-First Search first implementation.
  1.5   - 13 Mar 2022. Tidied MSVC build. Added a .bat file for building hash_test.c.
  1.4   - 12 Mar 2022. Hash table functions.
  1.3   - 11 Sep 2020. Fixed apg_file_to_str() portability issue.
  1.2   - 15 May 2020. Updated timers for multi-platform use based on Professional Programming Tools book code. Updated test code.
  1.1   -  4 May 2020. Added custom rand() functions.
  1.0   -  8 May 2015. First version by Anton Gerdelan.

Usage Instructions
-----------------------------
* Just copy-paste the snippets from this file that you want to use.
* Or, to use all of it:
  * In one file #define APG_IMPLEMENTATION above the #include.
  * For backtraces on Windows you need to link against -limagehlp (MinGW/GCC), or /link imagehlp.lib (MSVC/cl.exe).
    You can exclude this by:

  #define APG_NO_BACKTRACES
  #include apg.h

ToDo
-----------------------------
* linearise/unlinearise function. Note(Anton): I can't remember what this even means.
* more string manipulation/trimming utils
* maybe a generic KEY VALUE settings file parser that can be queried for KEY to get VALUE
* swap. everyone has a swap in their utils file
*/

#ifndef _APG_H_
#define _APG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define _FILE_OFFSET_BITS 64 /* Required for ftello on e.g. MinGW to use 8 bytes instead of 4. This can also be defined in a compile string/build file. */
#include <stdbool.h>
#include <stddef.h>   /* size_t */
#include <stdint.h>   /* types */
#include <stdio.h>    /* FILE* */
#include <sys/stat.h> /* File sizes and details. */

/*=================================================================================================
COMPILER HELPERS
=================================================================================================*/
#ifdef _WIN64
#define APG_BUILD_PLAT_STR "Microsoft Windows (64-bit)."
#elif _WIN32
#define APG_BUILD_PLAT_STR "Microsoft Windows (32-bit)."
#elif __CYGWIN__ /* _WIN32 must not be defined */
#define APG_BUILD_PLAT_STR "Cygwin POSIX under Microsoft Windows."
#elif __linux__
#define APG_BUILD_PLAT_STR "Linux."
#elif __APPLE__ /* Can add checks to detect OSX/iPhone/XCode iPhone emulators. */
#define APG_BUILD_PLAT_STR "OS X."
#elif __unix__ /* Also valid for Linux. __APPLE__ is also BSD. */
#define APG_BUILD_PLAT_STR "BSD."
#else
#define APG_BUILD_PLAT_STR "Unknown."
#endif

#define APG_UNUSED( x ) (void)( x ) /* to suppress compiler warnings */

/* to add function deprecation across compilers */
#ifdef __GNUC__
#define APG_DEPRECATED( func ) func __attribute__( ( deprecated ) )
#elif defined( _MSC_VER )
#define APG_DEPRECATED( func ) __declspec( deprecated ) func
#endif

/*=================================================================================================
MATHS
=================================================================================================*/
/* replacement for the deprecated min/max functions from original C spec.
was going to have a series of GL-like functions but it was a lot of fiddly code/alternatives,
so I'm just copying from stb.h here. as much as I dislike pre-processor directives, this makes sense.
I believe the trick is to have all the parentheses. same deal for clamp */
#define APG_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define APG_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define APG_CLAMP( x, lo, hi ) ( APG_MIN( hi, APG_MAX( lo, x ) ) )

/*=================================================================================================
PSEUDO-RANDOM NUMBERS
=================================================================================================*/
/* platform-consistent rand() and srand()
based on http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf pg 312
WARNING - these functions are not thread-safe. */
#define APG_RAND_MAX 32767 // Must be at least 32767 (0x7fff). Windows uses this value.

void apg_srand( unsigned int seed );
int apg_rand( void );

/* same as apg_rand() except returns a value between 0.0 and 1.0 */
float apg_randf( void );

/* useful to re-seed apg_srand later with whatever the pseudo-random sequence is up to now i.e. for saved games. */
unsigned int apg_get_srand_next( void );

/*=================================================================================================
TIME
=================================================================================================*/
/* Set up for using timers.
WARNING - These functions are not thread-safe. */
void apg_time_init( void );

/* Get a monotonic time value in seconds with up to nanoseconds precision.
Value is some arbitrary system time but is invulnerable to clock changes.
Call apg_time_init() once before calling apg_time_s().
WARNING - These functions are not thread-safe. */
double apg_time_s( void );

/* NOTE: for linux -D_POSIX_C_SOURCE=199309L must be defined for glibc to get nanosleep() */
void apg_sleep_ms( int ms );

/*=================================================================================================
STRINGS
=================================================================================================*/
/* Custom strcmp variant to do a partial match avoid commonly-made == 0 bracket soup bugs.
PARAMS
a,b         - Input strings to compare.
a_max,b_max - Maximum lengths of a and b, respectively. Makes function robust to missing nul-terminators.
RETURNS true if both strings are the same, or if the shorter string matches its length up to the longer string at that point. i.e. "ANT" "ANTON" returns true. */
bool apg_strparmatch( const char* a, const char* b, size_t a_max, size_t b_max );

/* because string.h doesn't always have strnlen() */
int apg_strnlen( const char* str, int maxlen );

/* Custom strncat() without the annoying '\0' src truncation issues.
   Resulting string is always '\0' truncated.
   PARAMS
     dest_max - This is the maximum length the destination string is allowed to grow to.
     src_max  - This is the maximum number of bytes to copy from the source string.
*/
void apg_strncat( char* dst, const char* src, const int dest_max, const int src_max );

/*=================================================================================================
FILES
=================================================================================================*/
// These defines allow support of >2GB files on different platforms. Was not required on my Linux with GCC, but was on Windows with GCC on the same hardware.
#ifdef _MSC_VER // This means "if MSVC" because we prefer POSIX stuff on MINGW.
#define apg_fseek _fseeki64
#define apg_ftell _ftelli64
#define apg_stat _stat64
#define apg_stat_t __stat64
#else
#define apg_fseek fseeko
#define apg_ftell ftello
#define apg_stat stat
#define apg_stat_t stat
#endif

/** Represents memory loaded from a file. */
typedef struct apg_file_t {
  void* data_ptr;
  size_t sz; // Size of memory pointed to by data_ptr in bytes.
} apg_file_t;

typedef enum apg_dirent_type_t { APG_DIRENT_NONE, APG_DIRENT_FILE, APG_DIRENT_DIR, APG_DIRENT_OTHER } apg_dirent_type_t;

/** A directory entry. */
typedef struct apg_dirent_t {
  apg_dirent_type_t type;
  char* path;
} apg_dirent_t;

/** Check if a path is a valid file.
 * @return
 * False if path is not a file.
 * False on any error.
 * True if path was a file.
 */
bool apg_is_file( const char* path );

/** Check if a path is a valid directory.
 * @return
 * False if path is not a directory.
 * False on any error.
 * True if path was a directory.
 */
bool apg_is_dir( const char* path );

/** Get a file's size. Supports large (multi-GB) files.
 * @return
 * Size in bytes of file given by filename, or -1 on error.
 */
int64_t apg_file_size( const char* filename );

/** Get a list of items in a directory, including file and directories.
 *
 * @param path_ptr
 * A directory path to scan for contents.
 *
 * @param list_ptr
 * The caller must provide an address to a contents pointer. This function
 * will allocate memory for, and populate a list, that this parameter will
 * be pointed to `apg_free_contents_list()`.
 *
 * @param n_list
 * The caller must provide the address on an integer. The number of items
 * populated in the list will be set here. This value must be retained by
 * the called, unmodified, as it is used to free the string memory when
 * passed to
 *
 * @return
 * On success this function returns `true`.
 * Basic errors, such as NULL parameters, or an invalid directory path will
 * return `false`.
 *
 * @warning
 * Symlinks and hard links may not be reported as such, and are most likely
 * still reported as directory, and file types, respectively.
 *
 * @warning
 * This function allocates memory for the the items in `list_ptr`, as well as
 * strings inside each item. Call `apg_free_contents_list()` to free the
 * allocated memory.
 *
 * @note
 * Note that the file names of contents do not include `path`, so you will need
 * to concatenate the full path in order to access the files. The internal
 * function `_fix_dir_slashes()` may be useful here.
 */
bool apg_dir_contents( const char* path_ptr, apg_dirent_t** list_ptr, int* n_list );

bool apg_free_contents_list( apg_dirent_t** list_ptr, int n_list );

/** Reads an entire file into memory, unaltered. Supports large (multi-GB) files.
 *
 * @return
 *   true on success. In this case record->data is allocated memory and must be freed by the caller.
 *   false on any error. Any allocated memory is freed if false is returned.
 *
 * @warning If you are also writing very large files, be aware some platforms (Windows) will stall if fwrite()s are not split into <=2GB chunks.
 */
bool apg_read_entire_file( const char* filename, apg_file_t* record );

/** Loads file_name's contents into a byte array and always ends with a NULL terminator.
 * @param max_len Maximum bytes available to write into str_ptr.
 * @return false on any error, and if the file size + 1 exceeds max_len bytes.
 */
bool apg_file_to_str( const char* file_name, int64_t max_len, char* str_ptr );

/*=================================================================================================
LOG FILES
=================================================================================================*/
/* Make bad log args print compiler warnings. Note: mingw sucks for this. */
#if defined( __clang__ )
#define ATTRIB_PRINTF( fmt, args ) __attribute__( ( __format__( __printf__, fmt, args ) ) )
#elif defined( __MINGW32__ )
#define ATTRIB_PRINTF( fmt, args ) __attribute__( ( format( ms_printf, fmt, args ) ) )
#elif defined( __GNUC__ )
#define ATTRIB_PRINTF( fmt, args ) __attribute__( ( format( printf, fmt, args ) ) )
#else
#define ATTRIB_PRINTF( fmt, args )
#endif

/* open/refresh a new log file and print timestamp */
void apg_start_log();
/* write a log entry */
void apg_log( const char* message, ... ) ATTRIB_PRINTF( 1, 2 );
/* write a log entry and print to stderr */
void apg_log_err( const char* message, ... ) ATTRIB_PRINTF( 1, 2 );

/*=================================================================================================
BACKTRACES AND DUMPS
=================================================================================================*/
/* Obtain a backtrace and print it to an open file stream or eg stdout
note: to convert trace addresses into line numbers you can use gdb:
(gdb) info line *print_trace+0x5e
Line 92 of "src/utils.c" starts at address 0x6c745 <print_trace+74> and ends at 0x6c762 <print_trace+103>. */
void apg_print_trace( FILE* stream );

/* writes a backtrace on sigsegv */
void apg_start_crash_handler( void );

#ifdef APG_UNIT_TESTS
void apg_deliberate_sigsegv( void );
void apg_deliberate_divzero( void );
#endif

/*=================================================================================================
COMMAND LINE PARAMETERS
=================================================================================================*/
/* i learned this trick from the DOOM source code */
int apg_check_param( const char* check );

extern int g_apg_argc;
extern char** g_apg_argv;

/*=================================================================================================
MEMORY
=================================================================================================*/

// NB `ULL` postfix is necessary or numbers ~4GB will be interpreted as integer constants and overflow.
#define APG_KILOBYTES( value ) ( (value)*1024ULL )
#define APG_MEGABYTES( value ) ( APG_KILOBYTES( value ) * 1024ULL )
#define APG_GIGABYTES( value ) ( APG_MEGABYTES( value ) * 1024ULL )

/* avoid use of malloc at runtime. use alloca() for up to ~1MB, or scratch_mem() for reusing a larger preallocated heap block */

/* call once at program start, after starting logs
(re)allocates scratch memory for use by scratch_mem
not thread safe
scratch_a_sz - size of main scratch in bytes
scratch_b_sz - size of second scratch in bytes ( can be zero if not needed )
scratch_c_sz - size of second scratch in bytes ( can be zero if not needed )
asserts on out of memory */
void apg_scratch_init( size_t scratch_a_sz, size_t scratch_b_sz, size_t scratch_c_sz );

/* frees both scratch memory allocations
asserts if nothing allocated */
void apg_scratch_free();

/* returns a pointer to a pre-allocated, zeroed, block of memory to use for eg image resizes to avoid alloc
asserts if sz requested in bytes is larger than pre-allocated size or if prealloc_scratch() hasn't been called */
void* apg_scratch_mem_a( size_t sz );

/* use the second scratch */
void* apg_scratch_mem_b( size_t sz );

/* use the third scratch */
void* apg_scratch_mem_c( size_t sz );

/*=================================================================================================
COMPRESSION
=================================================================================================*/
/* Apply run-length encoding to an array of bytes pointed to by bytes_in, over size in bytes given by sz_in.
The result is written to bytes_out, with output size in bytes written to sz_out.
PARAMS
  bytes_in  - If NULL then sz_out is set to 0.
  sz_in     - If 0 then sz_out is set to 0.
  bytes_out - If NULL then sz_out is reported, but no memory is written to. This is useful for determining the size required for output buffer allocation.
  sz_out    - Must not be NULL.
*/
void apg_rle_compress( const uint8_t* bytes_in, size_t sz_in, uint8_t* bytes_out, size_t* sz_out );
void apg_rle_decompress( const uint8_t* bytes_in, size_t sz_in, uint8_t* bytes_out, size_t* sz_out );

/*=================================================================================================
HASH TABLE
Motivation:
 - Avoid performance-disruptive run-time memory allocation, so it's linear probing rather than chained buckets. -> It Still needs to malloc() key strings though.
 - Allow user to check collisions and hash table capacity so user can decide on a good initial table size based on their data.
 - Minimal aux. memory overhead.
 - Fast and simple.
 - Allow user to determine when to rebuild the hash-table. There should never be surprise table reallocations at run-time!
   To explicitly allow (constrained) resizing:
   * After a key is stored with apg_hash_store(), run apg_hash_table_auto_resize( &my_table, max_bytes ).

Potential improvements:
 - If the user program reliably retains strings as well as values, we could avoid string memory allocation during hash_store calls, and just point to external.
 - If I also stored the hash in apg_hash_table_element_t it would avoid many potentially lengthy strcmp() calls during search.
 - String safety isn't checked at all. strndup and strncmp could be used if the user supplies a maximum string length.
 - Could use quadratic probing instead of liner probing.
 ================================================================================================*/

typedef struct apg_hash_table_element_t {
  char* keystr;    // This is either an allocated ASCII string or an integer value.
  void* value_ptr; // Address of value in user code. Value data is not allocated or stored directly in the table. If NULL then element is empty.
} apg_hash_table_element_t;

typedef struct apg_hash_table_t {
  apg_hash_table_element_t* list_ptr;
  uint32_t n;
  uint32_t count_stored;
} apg_hash_table_t;

/** Allocates memory for a hash table of size `table_n`.
 * @param table_n For a well performing table use a number somewhat larger than required space.
 * @return A generated, empty, hash table, or an empty table ( list_ptr == NULL ) on out of memory error.
 */
apg_hash_table_t apg_hash_table_create( uint32_t table_n );

/** Free any memory allocated to the table, including allocated key string memory. */
void apg_hash_table_free( apg_hash_table_t* table_ptr );

/** Returns a hash for a key->table mapping.
 * Be sure to compute hash_index = hash % table_N after calling this function.
 */
uint32_t apg_hash( const char* keystr );

/** A second hash function, using djb2 (based on http://www.cse.yorku.ca/~oz/hash.html),
 * This is used by store and search functions on first collision for a double-hashing approach.
 */
uint32_t apg_hash_rehash( const char* keystr );

/** Store a key-value pair in a given hash table.
 * @param keystr        A null-terminated C string. Must not be NULL.
 * @param value_ptr     Address of external memory to point to. Must not be NULL.
 * @param table_ptr     Address of a hash table previously allocated with a call to apg_hash_table_create().
 * @param collision_ptr Optional argument. If non-NULL, then the integer pointed to is set to the number of collisions incurred by this function call.
 *                      In cases where the function returns false then the collision counter is not incremented.
 * @return              This function returns true on success. It returns false in cases where the table is full,
 *                      the key was already stored in the table, or the parameters are invalid.
 */
bool apg_hash_store( const char* keystr, void* value_ptr, apg_hash_table_t* table_ptr, uint32_t* collision_ptr );

/**
 * @return This function returns true if the key is found in the table. In this case the integer pointed to by `idx_ptr` is set to the corresponding table
 * index. This function returns false if the table is empty, the parameters are invalid, or the key is not stored in the table.
 */
bool apg_hash_search( const char* keystr, apg_hash_table_t* table_ptr, uint32_t* idx_ptr, uint32_t* collision_ptr );

/** Expand when hash table when >= 50% full, and double its size if so, but don't allocate a table of more than `max_bytes`.
 *  This function could be improved in performance (at expense of brevity) by manually writing out apg_hash_store() and excluding string allocations.
 *  This function could be upgraded into _auto_resize() which also scales down on e.g. < 25% load.
 */
bool apg_hash_auto_expand( apg_hash_table_t* table_ptr, size_t max_bytes );

/*=================================================================================================
GREEDY BEST-FIRST SEARCH
=================================================================================================*/

/// If a node can have more than 6 neighbours change this value to set the size of the array of neighbour keys.
#define APG_GBFS_NEIGHBOURS_MAX 6

/// Aux. memory retained to represent a 'vertex' in the search graph.
typedef struct apg_gbfs_node_t {
  int64_t parent_idx; // Index of parent in the evaluated_nodes list.
  int64_t our_key;    // Identifying key of the original node (e.g. a tile or pixel index in an array).
  int64_t h;          // Distance to goal.
} apg_gbfs_node_t;

/** Greedy best-first search.
 * This function was designed so that no heap memory is allocated. It has some stack memory limits but that's usually fine for real-time applications.
 * It will return false if these limits are reached for big mazes. It could be modified to use or realloc() heap memory to solve for these cases.
 * I usually use an index or a handles as unique O(1) look-up for graph nodes/voxels/etc. But these could also have been pointers/addresses.
 *
 * @param start_key,target_key  The user provides initial 2 node/vertex keys, expressed as integers
 * @param h_cb_ptr()            User-defined function to return a distance heuristic, h, for a key.
 * @param neighs_cb_ptr()       User-defined function to pass an array of up to 6 (for now) neighbours' keys.
 *                              It should return the count of keys in the array.
 * @param reverse_path_ptr      Pointer to a user-created array of size `max_path_steps`.
 *                              On success the function will write the reversed path of keys into this array.
 * @param path_n                The number of steps in reverse_path_ptr is written to the integer at address `path_n`.
 * @param evaluated_nodes_ptr   User-allocated array of working memory used. Size in bytes is sizeof(apg_gbfs_node_t) * evaluated_nodes_max.
 * @param evaluated_nodes_max   Count of `apg_gbfs_node_t`s allocated to evaluated_nodes_ptr. Worst case - bounds of search domain.
 * @param visited_set_ptr       User-allocated array of working memory used. Size in bytes is sizeof(int) * visited_set_max.
 * @param visited_set_max       Count of `int`s allocated to evaluated_nodes_ptr. Worst case - bounds of search domain.
 * @param queue_ptr             User-allocated array of working memory used. Size in bytes is sizeof(apg_gbfs_node_t) * queue_max.
 * @param queue_max             Count of `apg_gbfs_node_t`s allocated to evaluated_nodes_ptr. Worst case - bounds of search domain.
 * @return                      If a path is found the function returns `true`.
 *                              If no path is found, or there was an error, such as array overflow, then the function returns `false`.
 *
 * @note I let the user supply the working sets (queue, evualated, and visited set) memory. This allows bigger searches than using small stack arrays,
 * and can avoid syscalls. Repeated searches can reuse any allocated memory.
 */
bool apg_gbfs( int64_t start_key, int64_t target_key, int64_t ( *h_cb_ptr )( int64_t key, int64_t target_key ),
  int64_t ( *neighs_cb_ptr )( int64_t key, int64_t target_key, int64_t* neighs ), int64_t* reverse_path_ptr, int64_t* path_n, int64_t max_path_steps,
  apg_gbfs_node_t* evaluated_nodes_ptr, int64_t evaluated_nodes_max, int64_t* visited_set_ptr, int64_t visited_set_max, apg_gbfs_node_t* queue_ptr, int64_t queue_max );

#ifdef __cplusplus
}
#endif

#endif /* _APG_H_ */

/*
-------------------------------------------------------------------------------------
This software is available under two licences - you may use it under either licence.
-------------------------------------------------------------------------------------
FIRST LICENCE OPTION

>                                  Apache License
>                            Version 2.0, January 2004
>                         http://www.apache.org/licenses/
>    Copyright 2019 Anton Gerdelan.
>    Licensed under the Apache License, Version 2.0 (the "License");
>    you may not use this file except in compliance with the License.
>    You may obtain a copy of the License at
>        http://www.apache.org/licenses/LICENSE-2.0
>    Unless required by applicable law or agreed to in writing, software
>    distributed under the License is distributed on an "AS IS" BASIS,
>    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
>    See the License for the specific language governing permissions and
>    limitations under the License.
-------------------------------------------------------------------------------------
SECOND LICENCE OPTION

> This is free and unencumbered software released into the public domain.
>
> Anyone is free to copy, modify, publish, use, compile, sell, or
> distribute this software, either in source code form or as a compiled
> binary, for any purpose, commercial or non-commercial, and by any
> means.
>
> In jurisdictions that recognize copyright laws, the author or authors
> of this software dedicate any and all copyright interest in the
> software to the public domain. We make this dedication for the benefit
> of the public at large and to the detriment of our heirs and
> successors. We intend this dedication to be an overt act of
> relinquishment in perpetuity of all present and future rights to this
> software under copyright law.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
> EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
> MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
> IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
> OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
> ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
> OTHER DEALINGS IN THE SOFTWARE.
>
> For more information, please refer to <http://unlicense.org>
-------------------------------------------------------------------------------------
*/
