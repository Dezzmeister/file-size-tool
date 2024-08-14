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
#include <Windows.h>
#include <Pathcch.h>
#include "files.h"

static void check_path_err(HRESULT result, _In_z_ const LPCWSTR path, _In_z_ const LPCWSTR more) {
	if (result != S_OK) {
		print_err_fmt(L"Failed to join %1!s! and %2!s!\n", path, more);
		ExitProcess(result);
	}
}

static BOOL is_dot_path(_In_reads_z_(3) const LPCWSTR path) {
	return (path[0] == '.' && path[1] == '\0') ||
		(path[0] == '.' && path[1] == '.' && path[2] == '\0');
}

file_map_pair measure_dir(_In_z_ const LPCWSTR dir, const DWORD64 threshold, const BOOL is_top_level) {
	WCHAR path_buf[MAX_PATH];
	WCHAR child_buf[MAX_PATH];
	HRESULT result = PathCchCombine(path_buf, MAX_PATH, dir, L"*");
	check_path_err(result, dir, L"*");

	file_map * out = alloc_or_die(sizeof(file_map));
	out->sibling = NULL;
	CopyMemory(out->filename, dir, MAX_PATH * sizeof(WCHAR));

	WIN32_FIND_DATAW file_data;
	HANDLE h;

	if (is_top_level) {
		h = FindFirstFileW(dir, &file_data);

		if (h == INVALID_HANDLE_VALUE) {
			DWORD err = GetLastError();
			skipped_file_map * skipped = alloc_or_die(sizeof(skipped_file_map));
			skipped->next = NULL;
			skipped->reason = err;

			DWORD len = lstrlenW(dir) + 1;
			CopyMemory(skipped->path, dir, len * sizeof(WCHAR));

			file_map_pair pair;
			pair.root = NULL;
			pair.skipped = skipped;

			return pair;
		}

		out->attributes = file_data.dwFileAttributes;

		BOOL close_result = FindClose(h);
		check_err(! close_result);
	}

	h = FindFirstFileW(path_buf, &file_data);

	if (h == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		skipped_file_map * skipped = alloc_or_die(sizeof(skipped_file_map));
		skipped->next = NULL;
		skipped->reason = err;

		DWORD len = lstrlenW(dir) + 1;
		CopyMemory(skipped->path, dir, len * sizeof(WCHAR));

		file_map_pair pair;
		pair.root = NULL;
		pair.skipped = skipped;

		return pair;
	}

	file_map * root = NULL;
	file_map * curr = NULL;
	file_map * next = NULL;
	DWORD64 total_size = 0;
	file_map_pair sub_result;
	skipped_file_map * skipped_root = NULL;
	skipped_file_map * skipped_curr = NULL;

	do {
		if (is_dot_path(file_data.cFileName)) {
			continue;
		} else if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			result = PathCchCombine(child_buf, MAX_PATH, dir, file_data.cFileName);
			check_path_err(result, dir, file_data.cFileName);

			sub_result = measure_dir(child_buf, threshold, FALSE);
			next = sub_result.root;

			if (sub_result.skipped) {
				if (skipped_curr) {
					skipped_curr->next = sub_result.skipped;
					skipped_curr = skipped_curr->next;
				} else {
					skipped_root = sub_result.skipped;
					skipped_curr = skipped_root;
				}
			}

			if (! next) {
				continue;
			}

			next->attributes = file_data.dwFileAttributes;
		} else {
			next = alloc_or_die(sizeof(file_map));
			next->first_child = NULL;
			next->sibling = NULL;
			next->size = ((DWORD64)file_data.nFileSizeHigh << 32) | file_data.nFileSizeLow;
			CopyMemory(next->filename, file_data.cFileName, MAX_PATH * sizeof(WCHAR));
			next->attributes = file_data.dwFileAttributes;
		}

		if (curr) {
			curr->sibling = next;
		}

		curr = next;

		if (! root) {
			root = curr;
		}

		total_size += curr->size;
	} while (FindNextFileW(h, &file_data));

	BOOL close_result = FindClose(h);
	check_err(! close_result);

	out->first_child = root;
	out->size = total_size;

	file_map * prev = NULL;
	curr = out->first_child;

	// Remove all child nodes with size < threshold
	while (curr) {
		if (curr->size < threshold) {
			if (prev) {
				prev->sibling = curr->sibling;
			} else {
				out->first_child = curr->sibling;
			}

			file_map * next = curr->sibling;
			curr->sibling = NULL;

			free_file_map(curr);
			curr = next;
		} else {
			prev = curr;
			curr = curr->sibling;
		}
	}

	file_map_pair pair;
	pair.root = out;
	pair.skipped = skipped_root;

	return pair;
}

void print_file_map(_In_z_ const LPCWSTR dir, _In_opt_ const file_map * node) {
	if (! node) {
		return;
	}

	static WCHAR size_buf[BYTES_TO_SIZE_MAX_CHARS];
	WCHAR path_buf[MAX_PATH];
	HRESULT result = PathCchCombine(path_buf, MAX_PATH, dir, node->filename);
	check_path_err(result, dir, node->filename);

	LPCWSTR entry_type;

	if (node->attributes & FILE_ATTRIBUTE_DIRECTORY) {
		entry_type = can_use_colors ? L"\x1b[93md\x1b[0m" : L"d";
	} else {
		entry_type = can_use_colors ? L"\x1b[37mf\x1b[0m" : L"f";
	}

	const LPCWSTR fmt_str = can_use_colors ?
		L"\x1b[94m%1!s!\x1b[0m\t\t%2!s!\t%3!s!\n" :
		L"%1!s!\t\t%2!s!\t%3!s!\n";

	bytes_to_size(size_buf, node->size);
	print_fmt(fmt_str, size_buf, entry_type, path_buf);

	print_file_map(path_buf, node->first_child);
	print_file_map(dir, node->sibling);
}

void print_skipped_file_map(_In_opt_ const skipped_file_map * root) {
	if (! root) {
		return;
	}

	LPWSTR err_buf = NULL;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		root->reason,
		0,
		(LPWSTR)&err_buf,
		0,
		NULL
	);

	LPCWSTR default_err = L"Unknown error\n";

	const LPCWSTR fmt_str = can_use_colors ?
		L"%1!s!: \x1b[31m%2!s!\x1b[0m" :
		L"%1!s!: %2!s!";
	print_fmt(fmt_str, root->path, err_buf ? err_buf : default_err);

	if (err_buf) {
		LocalFree(err_buf);
	}

	print_skipped_file_map(root->next);
}

void free_file_map(_In_opt_ const file_map * root) {
	if (! root) {
		return;
	}

	free_file_map(root->sibling);
	free_file_map(root->first_child);

	dealloc_or_die(root);
}

void free_skipped_file_map(_In_opt_ const skipped_file_map * root) {
	if (! root) {
		return;
	}

	free_skipped_file_map(root->next);

	dealloc_or_die(root);
}
