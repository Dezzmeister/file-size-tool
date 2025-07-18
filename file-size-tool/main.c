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
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include "files.h"

const WCHAR HELP_TEXT[] =
L"Usage: %1!s! <dir> <threshold>\n\n"
L"\tThis tool reports files and directories larger than a given size.\n\n"
L"\t<dir> is the directory to scan. All subdirectories and files will be scanned.\n"
L"\t<threshold> is a size string like '50K', '0x20M', or '1G'. This string must be\n"
L"\ta positive integer. It can be decimal or hexadecimal, and it can be followed by\n"
L"\t'K' (kilobytes), 'M' (megabytes), or 'G' (gigabytes). If no scale is provided,\n"
L"\tbytes are assumed."
L"\n\n"
L"file-size-tool is licensed under the GNU Public License 3 or any later version at your choice.\n"
L"See https://github.com/Dezzmeister/file-size-tool/blob/master/COPYING for details.\n"
L"You can download the source code at https://github.com/Dezzmeister/file-size-tool.\n";

int wmain(const int argc, WCHAR ** const argv);

// This is the true entrypoint. We are not including the C Runtime, so we have
// to get `argc` and `argv` manually and pass them into `wmain`.
ULONG WINAPI entry() {
	init_globals();

	LPWSTR cmd_line = GetCommandLineW();

	WCHAR ** argv;
	int argc;

	argv = CommandLineToArgvW(cmd_line, &argc);
	check_err(! argv);

	can_use_colors = SetConsoleMode(std_out, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	return wmain(argc, argv);
}

int wmain(const int argc, WCHAR ** const argv) {
	if (argc < 3) {
		print_fmt(HELP_TEXT, argv[0]);

		return 0;
	}

	DWORD64 threshold = size_to_bytes(argv[2]);
	file_map_pair pair = measure_dir(argv[1], threshold, TRUE);

	if (pair.root && pair.root->size < threshold) {
		print_err_fmt(L"No files or directories found with size less than %1!s!\n", argv[2]);

		if (pair.skipped) {
			print_err_fmt(L"\nSome directories were skipped:\n\n");
			print_skipped_file_map(pair.skipped);
		}

		return 1;
	}

	print_file_map(L"", pair.root);

	if (pair.skipped) {
		print_err_fmt(L"\nSome directories were skipped:\n\n");
		print_skipped_file_map(pair.skipped);
	}

	free_file_map(pair.root);
	free_skipped_file_map(pair.skipped);

	return 0;
}