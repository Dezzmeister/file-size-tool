#include <Windows.h>
#include <Pathcch.h>
#include "files.h"

static void check_path_err(HRESULT result, LPWSTR path, LPWSTR more) {
	if (result != S_OK) {
		print_err_fmt(L"Failed to join %1!s! and %2!s!\n", path, more);
		ExitProcess(result);
	}
}

static BOOL is_dot_path(const WCHAR path[MAX_PATH]) {
	return (path[0] == '.' && path[1] == '\0') ||
		(path[0] == '.' && path[1] == '.' && path[2] == '\0');
}

file_map * measure_dir(LPWSTR dir) {
	WCHAR path_buf[MAX_PATH];
	WCHAR child_buf[MAX_PATH];
	HRESULT result = PathCchCombine(path_buf, MAX_PATH, dir, L"*");
	check_path_err(result, dir, L"*");

	WIN32_FIND_DATA file_data;
	HANDLE h = FindFirstFileW(path_buf, &file_data);
	check_err(h == INVALID_HANDLE_VALUE);

	file_map * root = NULL;
	file_map * curr = NULL;
	file_map * next = NULL;
	DWORD64 total_size = 0;

	do {
		if (is_dot_path(file_data.cFileName)) {
			continue;
		} else if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			result = PathCchCombine(child_buf, MAX_PATH, dir, file_data.cFileName);
			check_path_err(result, dir, file_data.cFileName);

			next = measure_dir(child_buf);
		} else {
			next = alloc_or_die(sizeof(file_map));
			next->first_child = NULL;
			next->sibling = NULL;
			next->size = ((DWORD64)file_data.nFileSizeHigh << 32) | file_data.nFileSizeLow;
			CopyMemory(next->filename, file_data.cFileName, MAX_PATH);
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

	file_map * out = alloc_or_die(sizeof(file_map));
	out->first_child = root;
	out->sibling = NULL;
	out->size = total_size;
	CopyMemory(out->filename, dir, MAX_PATH);

	return out;
}

void print_stats(LPWSTR dir, file_map * node) {
	WCHAR path_buf[MAX_PATH];
	HRESULT result = PathCchCombine(path_buf, MAX_PATH, dir, node->filename);
	check_path_err(result, dir, node->filename);

	print_fmt(L"%1!d!\t%2!s!\n", node->size, path_buf);

	if (node->first_child) {
		print_stats(path_buf, node->first_child);
	}

	if (node->sibling) {
		print_stats(dir, node->sibling);
	}
}

void free_file_map(file_map * root) {
	if (root->sibling) {
		free_file_map(root->sibling);
	}

	if (root->first_child) {
		free_file_map(root->first_child);
	}

	dealloc_or_die(root);
}
