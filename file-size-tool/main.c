#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include "files.h"

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
	if (argc < 2) {
		print_fmt(L"Usage: %1!s! <dir>\n", argv[0]);

		return 0;
	}

	file_map * map = measure_dir(argv[1]);

	print_stats(L"", map);
	free_file_map(map);

	return 0;
}