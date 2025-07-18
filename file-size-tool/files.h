/*
 * This file is part of file-size-tool, a directory scanner.
 * Copyright (C) 2024  Joe Desmond
 *
 * file-size-tool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * file-size-tool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with file-size-tool.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <Windows.h>

#define ARR_SIZE(T)						(sizeof (T) / sizeof ((T)[0]))
#define BYTES_TO_SIZE_MAX_CHARS			16
#define SIZE_SCALE						1000L
#define LOCAL_MAX_PATH					4096

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
	WCHAR filename[LOCAL_MAX_PATH];
	// File attributes. These come from the `WIN32_FIND_DATAW` structure.
	DWORD attributes;
};
typedef struct file_map file_map;

// This is a list of files or directories that were skipped. The `reason` is an error
// code returned from `GetLastError`.
struct skipped_file_map {
	// The next skipped entry, or NULL if this is the last one
	struct skipped_file_map * next;
	// The full path to this entry
	WCHAR path[LOCAL_MAX_PATH];
	// An error code from `GetLastError` that gives the reason why this entry was skipped
	DWORD reason;
};
typedef struct skipped_file_map skipped_file_map;

typedef struct file_map_pair {
	file_map * root;
	skipped_file_map * skipped;
} file_map_pair;

void free_file_map(_In_opt_ const file_map * root);

void free_skipped_file_map(_In_opt_ const skipped_file_map * root);

// This is called before `wmain` to initialize some of the global constants that are used
// by other functions declared here.
void init_globals();

// Checks the error status of the last Win32 API call with `GetLastError`. If there was an
// error, it will be printed to stderr and the program will exit with the error's code.
void check_err(BOOL cond);

// Allocates `num_bytes` bytes on the default heap, and exits if the allocation
// fails. The result must be freed with `dealloc_or_die`. The bytes are 
// not zero-initialized.
_Ret_notnull_ void * alloc_or_die(SIZE_T num_bytes);

// Deallocates/frees some memory obtained with `alloc_or_die`. If the deallocation fails,
// the program exits.
void dealloc_or_die(_In_ const void * mem);

// Returns a formatted string. The caller is responsible for freeing
// the result with `LocalFree`. Note that the result will be NULL if 
// we failed to format the string for some reason.
LPWSTR fmt(_In_z_ const LPWSTR fmt_str, ...);

// Prints a formatted string to stdout. This is very similar to printf, but it's defined
// in terms of Win32 API functions instead of those in the stdlib.
void print_fmt(_In_z_ const LPCWSTR fmt_str, ...);

// Prints a formatted string to stderr.
void print_err_fmt(_In_z_ const LPCWSTR fmt_str, ...);

// Measures the size of a directory and all child entries. Entries with a size lower than
// the given threshold are discarded, but their sizes are still accounted for. An entry for
// the root directory is returned, along with any directories that could not be entered for
// whatever reason.
file_map_pair measure_dir(_In_z_ const LPCWSTR root_dir, const DWORD64 threshold, const BOOL is_top_level);

void print_file_map(_In_z_ const LPCWSTR dir, _In_opt_ const file_map * root);

void print_skipped_file_map(_In_opt_ const skipped_file_map * root);

// Converts the given size string to bytes. The size string may have a single letter suffix
// indicating the unit (either 'B' (bytes), 'K' (kilobytes), 'M' (megabytes), or 'G' (gigabytes)).
// If there is no suffix, the unit is assumed to be bytes. The numeric part of the string must
// be a non-negative integer. It can be given in decimal or hexadecimal (with an '0x' prefix).
DWORD64 size_to_bytes(_In_z_ const LPWSTR size_str);

// Converts the given `size` (in bytes) to a string and writes the result to `str`. The size
// may be converted to kilobytes, megabytes, or gigabytes. In any case, a single-character suffix
// will indicate the unit (either 'B', 'K', 'M', or 'G'). If the size is converted to another
// unit, the fractional part will be written with 2 digits of precision. If the fractional part
// is zero, no fractional part will be indicated (e.g., '5K' will be written instead of '5.00K').
void bytes_to_size(_Out_writes_(BYTES_TO_SIZE_MAX_CHARS) WCHAR str[BYTES_TO_SIZE_MAX_CHARS], const DWORD64 size);
