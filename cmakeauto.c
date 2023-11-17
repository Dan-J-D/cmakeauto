#include "cmakeauto.h"

bool cma_abspath(char *buf, size_t size, const char *path)
{
#ifdef _WIN32
	return _fullpath(buf, path, size) != NULL;
#else
#error unsupported platform
#endif
}

bool cma_create_process(char *filename,
												char *cmdline,
												char *workdir,
												void **processhandle,
												unsigned int *pid)
{
#ifdef _WIN32
	STARTUPINFOA si = {0};
	si.cb = sizeof(si);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi = {0};
	if (!CreateProcessA(filename, cmdline, NULL, NULL, TRUE, 0, NULL, workdir, &si, &pi))
	{
		printf("failed to create process reason: %d\n", GetLastError());
		return false;
	}

	if (processhandle)
		*processhandle = pi.hProcess;
	else
		CloseHandle(pi.hProcess);

	if (pid)
		*pid = pi.dwProcessId;

	CloseHandle(pi.hThread);
#else
#error unsupported platform
#endif

	return true;
}

bool cma_watch_folder_init(CMakeAutoConfig *config)
{
	/* Unimplemented */
	return false;
}

bool cma_watch_folder_wait_for_next_change(CMakeAutoConfig *config)
{
	/* Unimplemented */
	return false;
}

bool cma_watch_folder_close(CMakeAutoConfig *config)
{
	/* Unimplemented */
	return false;
}

bool cma_init_proj(CMakeAutoConfig *config)
{
	bool is_generator_present = strlen(config->generator) > 0;

	char cmdline[1024];
	sprintf_s(cmdline, sizeof(cmdline), "cmake -S \"%s\" -B \"%s\" %s%s%s-A %s -DCMAKE_BUILD_TYPE=%s %s",
						config->srcdir, config->builddir,
						is_generator_present ? "-G \"" : "", is_generator_present ? config->generator : "", is_generator_present ? "\" " : " ",
						config->arch == CMAKE_AUTO_ARCH_X86		? "Win32"
						: config->arch == CMAKE_AUTO_ARCH_X64 ? "x64"
																									: "Unknown",
						config->mode == CMAKE_AUTO_MODE_DEBUG			? "Debug"
						: config->mode == CMAKE_AUTO_MODE_RELEASE ? "Release"
																											: "Unknown",
						config->extra_init_args ? config->extra_init_args : "");

	void *processhandle = 0;
	if (!cma_create_process(NULL, cmdline, NULL, &processhandle, 0))
	{
		printf("failed to create cma_init_proj process\n");
		return false;
	}

	printf("> %s\n", cmdline);

	WaitForSingleObject(processhandle, INFINITE);

	unsigned long exit_code = 0;
	GetExitCodeProcess(processhandle, &exit_code);

	printf("> exited with code %d\n", exit_code);

	return exit_code == 0;
}

bool cma_build(CMakeAutoConfig *config)
{
	char cmdline[1024];
	sprintf_s(cmdline, sizeof(cmdline), "cmake --build \"%s\" --config \"%s\" %s",
						config->builddir,
						config->mode == CMAKE_AUTO_MODE_DEBUG			? "Debug"
						: config->mode == CMAKE_AUTO_MODE_RELEASE ? "Release"
																											: "Unknown",
						config->extra_build_args ? config->extra_build_args : "");

	void *processhandle = 0;
	if (!cma_create_process(NULL, cmdline, NULL, &processhandle, 0))
	{
		printf("failed to create cma_init_proj process\n");
		return false;
	}

	printf("> %s\n", cmdline);

	WaitForSingleObject(processhandle, INFINITE);

	unsigned long exit_code = 0;
	GetExitCodeProcess(processhandle, &exit_code);

	printf("> exited with code %d\n", exit_code);

	return exit_code == 0;
}

int main(int argc, char **argv)
{
	CMakeAutoConfig config = {0};
	if (!cma_parse_args(argc, argv, &config))
		return -1;

	printf("-------------------\n");
	printf("action: %s\narch: %s\nmode: %s\nbuilddir: %s\nsrcdir: %s\noptions: %d\n",
				 config.action == CMAKE_AUTO_ACTION_BUILD	 ? "build"
				 : config.action == CMAKE_AUTO_ACTION_HELP ? "help"
																									 : "Unknown",
				 config.arch == CMAKE_AUTO_ARCH_X86		? "x86"
				 : config.arch == CMAKE_AUTO_ARCH_X64 ? "x64"
																							: "Unknown",
				 config.mode == CMAKE_AUTO_MODE_DEBUG			? "Debug"
				 : config.mode == CMAKE_AUTO_MODE_RELEASE ? "Release"
																									: "Unknown",
				 config.builddir, config.srcdir, config.options);
	printf("-------------------\n\n");

	switch (config.action)
	{
	case CMAKE_AUTO_ACTION_BUILD:
	{
		if (cma_init_proj(&config))
			cma_build(&config);

		if (config.options & CMAKE_AUTO_OPTION_AUTO_RELOAD)
			printf("auto reload is not implemented yet\n");
		break;
	}
	default:
		printf("Unknown action\n");
	case CMAKE_AUTO_ACTION_HELP:
		cma_print_usage();
		break;
	}

	return 0;
}
