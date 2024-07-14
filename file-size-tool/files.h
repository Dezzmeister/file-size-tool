#pragma once
#include <Windows.h>

#define ARR_SIZE(T)						(sizeof (T) / sizeof ((T)[0]))
#define BYTES_TO_SIZE_MAX_CHARS			16
#define SIZE_SCALE						1000L

extern int _fltused;

// These will be initialized before `wmain`.
extern HANDLE std_out;
extern HANDLE std_err;
extern HANDLE heap;
extern BOOL can_use_colors;

// This is a trie-like structure where the keys are paths, and the key
// "characters" are path segments.
struct file_map {
	// The first child of this entry. (TODO: Sort entries and consolidate them
	// if possible)
	// The fully qualified path of the first child will be the current filename +
	// the first_child's filename.
	struct file_map * first_child;
	// The next sibling of this entry. The fully qualified path of the next sibling
	// will be the parent filename (path) + the sibling's filename. The sibling is another
	// item in the same directory as this one.
	struct file_map * sibling;
	// The size of this entry in bytes. If this entry is a file, this is the size of the file.
	// If this entry is a directory, this is the size of all the directory's children. The size
	// of the directory entries themselves are not included.
	DWORD64 size;
	// The path segment of this entry. Taken with the parent's path, this forms a unique key
	// into the structure. If this is the root entry, then this will be the fully qualified
	// path of the root.
	WCHAR filename[MAX_PATH];
	// File attributes. These come from the `WIN32_FIND_DATAW` structure.
	DWORD attributes;
};
typedef struct file_map file_map;

void free_file_map(file_map * root);

// This is called before `wmain` to initialize some of the global constants that are used
// by other functions declared here.
void init_globals();

void check_err(BOOL cond);

// Allocates `num_bytes` bytes on the default heap, and exits if the allocation
// fails. The result must be freed with `dealloc_or_die`. The bytes are 
// not zero-initialized.
void * alloc_or_die(SIZE_T num_bytes);

// Deallocates/frees some memory obtained with `alloc_or_die`. If the deallocation fails,
// the program exits.
void dealloc_or_die(void * mem);

// Returns a formatted string. The caller is responsible for freeing
// the result with LocalFree. Note that the result will be NULL if 
// we failed to format the string for some reason.
LPWSTR fmt(const LPWSTR fmt_str, ...);

// Prints a formatted string to stdout. This is very similar to printf, but it's defined
// in terms of Win32 API functions instead of those in the stdlib.
void print_fmt(const LPCWSTR fmt_str, ...);

// Prints a formatted string to stderr.
void print_err_fmt(const LPCWSTR fmt_str, ...);

// Constructs a `file_map` from the given directory.
file_map * measure_dir(const LPCWSTR root_dir, const DWORD64 threshold, const BOOL is_top_level);

void print_stats(const LPCWSTR dir, file_map * root);

DWORD64 size_to_bytes(const LPWSTR size_str);

void bytes_to_size(WCHAR str[BYTES_TO_SIZE_MAX_CHARS], const DWORD64 size);
