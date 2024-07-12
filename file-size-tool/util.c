#include "files.h"

HANDLE std_out;
HANDLE std_err;
HANDLE heap;

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

LPWSTR vfmt(WCHAR * const fmt_str, va_list args) {
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

void print_fmt(const LPWSTR fmt_str, ...) {
	va_list args;
	va_start(args, fmt_str);

	LPWSTR msg = vfmt(fmt_str, args);
	va_end(args);

	WriteConsole(std_out, msg, lstrlenW(msg), NULL, NULL);
	LocalFree(msg);
}

void print_err_fmt(const LPWSTR fmt_str, ...) {
	va_list args;
	va_start(args, fmt_str);

	LPWSTR msg = vfmt(fmt_str, args);
	va_end(args);

	WriteConsole(std_err, msg, lstrlenW(msg), NULL, NULL);
	LocalFree(msg);
}