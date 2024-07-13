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
L"\tbytes are assumed.";

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

	return wmain(argc, argv);
}

int wmain(const int argc, WCHAR ** const argv) {
	if (argc < 3) {
		print_fmt(HELP_TEXT, argv[0]);

		return 0;
	}

	DWORD64 threshold = size_to_bytes(argv[2]);
	file_map * map = measure_dir(argv[1], threshold);

	if (map->size < threshold) {
		print_err_fmt(L"No files or directories found less than %1!s!\n", argv[2]);
		return 1;
	}

	print_stats(L"", map);
	free_file_map(map);

	return 0;
}