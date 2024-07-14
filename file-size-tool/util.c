#include <Shlwapi.h>
#include "files.h"

int _fltused = 1;

HANDLE std_out;
HANDLE std_err;
HANDLE heap;
BOOL can_use_colors;

void init_globals() {
	std_out = GetStdHandle(STD_OUTPUT_HANDLE);
	check_err(std_out == INVALID_HANDLE_VALUE);

	std_err = GetStdHandle(STD_ERROR_HANDLE);
	check_err(std_err == INVALID_HANDLE_VALUE);

	heap = GetProcessHeap();
	check_err(! heap);
}

void check_err(BOOL cond) {
	if (! cond) {
		return;
	}

	DWORD err = GetLastError();
	LPWSTR buf;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		err,
		0,
		(LPWSTR)&buf,
		0,
		NULL
	);

	WriteConsole(std_err, buf, lstrlenW(buf), NULL, NULL);
	ExitProcess(err);
}

static BOOL is_mem_error(DWORD except) {
	return except == STATUS_NO_MEMORY || except == STATUS_ACCESS_VIOLATION;
}

void * alloc_or_die(SIZE_T num_bytes) {
	void * out = HeapAlloc(heap, HEAP_GENERATE_EXCEPTIONS, num_bytes);

	if (! out) {
		print_err_fmt(L"Failed to allocate memory: %1!u!\n", num_bytes);
		ExitProcess(1);
	}

	return out;
}

void dealloc_or_die(void * mem) {
	BOOL result = HeapFree(heap, 0, mem);
	check_err(! result);
}

static LPWSTR vfmt(const LPCWSTR fmt_str, va_list args) {
	LPWSTR buf;
	DWORD result = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
		fmt_str,
		0, 0,
		(LPWSTR)&buf,
		0,
		&args
	);
	va_end(args);

	if (! result) {
		static const WCHAR err[] = L"Failed to format string\n";
		WriteConsole(std_err, err, ARR_SIZE(err), NULL, NULL);

		return NULL;
	}

	return buf;
}

LPWSTR fmt(const LPWSTR fmt_str, ...) {
	va_list args;
	va_start(args, fmt_str);

	LPWSTR out = vfmt(fmt_str, args);
	va_end(args);

	return out;

}

void print_fmt(const LPCWSTR fmt_str, ...) {
	va_list args;
	va_start(args, fmt_str);

	LPWSTR msg = vfmt(fmt_str, args);
	va_end(args);

	WriteConsole(std_out, msg, lstrlenW(msg), NULL, NULL);
	LocalFree(msg);
}

void print_err_fmt(const LPCWSTR fmt_str, ...) {
	va_list args;
	va_start(args, fmt_str);

	LPWSTR msg = vfmt(fmt_str, args);
	va_end(args);

	WriteConsole(std_err, msg, lstrlenW(msg), NULL, NULL);
	LocalFree(msg);
}

DWORD64 size_to_bytes(const LPWSTR size_str) {
	int str_len = lstrlenW(size_str);
	WCHAR last_char = size_str[str_len - 1];
	DWORD64 factor = 1;

	if (last_char == L'K' || last_char == L'k') {
		factor = SIZE_SCALE;
		size_str[str_len - 1] = '\0';
	} else if (last_char == L'M' || last_char == L'm') {
		factor = SIZE_SCALE * SIZE_SCALE;
		size_str[str_len - 1] = '\0';
	} else if (last_char == L'G' || last_char == L'g') {
		factor = SIZE_SCALE * SIZE_SCALE * SIZE_SCALE;
		size_str[str_len - 1] = '\0';
	}

	LONGLONG num;
	BOOL result = StrToInt64ExW(size_str, STIF_SUPPORT_HEX, &num);
	size_str[str_len - 1] = last_char;

	if (! result) {
		print_err_fmt(L"Invalid threshold size\n");
		ExitProcess(1);
	}

	if (num < 0) {
		print_err_fmt(L"Threshold size cannot be negative\n");
		ExitProcess(1);
	}

	return num * factor;
}

void bytes_to_size(WCHAR str[BYTES_TO_SIZE_MAX_CHARS], const DWORD64 size) {
	DWORD64 scale_f;
	WCHAR scale;

	if (size < SIZE_SCALE) {
		scale_f = 1L;
		scale = 'B';
	} else if (size < (SIZE_SCALE * SIZE_SCALE)) {
		scale_f = SIZE_SCALE;
		scale = 'K';
	} else if (size < (SIZE_SCALE * SIZE_SCALE * SIZE_SCALE)) {
		scale_f = SIZE_SCALE * SIZE_SCALE;
		scale = 'M';
	} else {
		scale_f = SIZE_SCALE * SIZE_SCALE * SIZE_SCALE;
		scale = 'G';
	}

	double size_f = size / (double)scale_f;
	DWORD64 size_whole = (DWORD64)size_f;
	DWORD64 size_frac = (DWORD64)((size_f - size_whole) * 100);

	LPCWSTR zeroes = L"";

	if (size_frac < 10) {
		zeroes = L"0";
	}

	const LPCWSTR fmt_str = (scale == 'B' || size_frac == 0L) ? L"%1!d!%4!c!" : L"%1!d!.%2!s!%3!d!%4!c!";
	DWORD_PTR args[] = { size_whole, (DWORD_PTR)zeroes, size_frac, scale };
	DWORD num_chars = FormatMessageW(
		FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
		fmt_str,
		0, 0,
		str,
		BYTES_TO_SIZE_MAX_CHARS,
		(va_list *) args
	);

	if (! num_chars) {
		static const WCHAR err[] = L"Failed to format byte string\n";
		WriteConsole(std_err, err, ARR_SIZE(err), NULL, NULL);

		ExitProcess(1);
	}
}