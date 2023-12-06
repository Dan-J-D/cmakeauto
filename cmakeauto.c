#include "cmakeauto.h"

bool cma_abspath(char *buf, size_t size, const char *path)
{
#ifdef _WIN32
	return _fullpath(buf, path, size) != NULL;
#else
#error unsupported platform
#endif
}

void cma_iterate_dir(const char *abspath,
										 const char *relpath,
										 void *userdata,
										 bool should_iter_sub_folder,
										 bool (*callback)(const char *abspath,
																			const char *relpath,
																			const char *name,
																			bool isfolder /* 0 = file, 1 = folder */,
																			void *userdata))
{
#ifdef _WIN32
	char abspath_search[FILE_MAX_PATH + 1];
	strcpy_s(abspath_search, FILE_MAX_PATH, abspath);
	strcat_s(abspath_search, FILE_MAX_PATH, "\\*.*");

	WIN32_FIND_DATA finddata;
	HANDLE findhandle = FindFirstFileA(abspath_search, &finddata);
	if (findhandle == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (strcmp(finddata.cFileName, ".") == 0 || strcmp(finddata.cFileName, "..") == 0)
			continue;

		char fileabspath[FILE_MAX_PATH + 1];
		sprintf_s(fileabspath, FILE_MAX_PATH, "%s\\%s", abspath, finddata.cFileName);
		char filerelpath[FILE_MAX_PATH + 1];
		sprintf_s(filerelpath, FILE_MAX_PATH, "%s\\%s", relpath, finddata.cFileName);

		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (should_iter_sub_folder)
				cma_iterate_dir(fileabspath, filerelpath, userdata, should_iter_sub_folder, callback);
			if (!callback(fileabspath, filerelpath, finddata.cFileName, true, userdata))
				break;
		}
		else
		{
			if (!callback(fileabspath, filerelpath, finddata.cFileName, false, userdata))
				break;
		}
	} while (FindNextFileA(findhandle, &finddata));

	FindClose(findhandle);
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

bool copy_file_callback(const char *abspath, const char *relpath, const char *name, bool isfolder, void *userdata)
{
	if (!isfolder)
	{
		char *folder = (char *)relpath;

		while (folder = strchr(folder + 1, '\\'))
		{
			char relsubfolder[FILE_MAX_PATH + 1];
			memset(relsubfolder, 0, FILE_MAX_PATH + 1);
			strncpy_s(relsubfolder, FILE_MAX_PATH, relpath, folder - relpath);

			char abssubfolder[FILE_MAX_PATH + 1];
			memset(abssubfolder, 0, FILE_MAX_PATH + 1);
			cma_abspath(abssubfolder, FILE_MAX_PATH, relsubfolder);

#ifdef _WIN32
			if (CreateDirectoryA(abssubfolder, NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS)
			{
				printf("failed to create directory %s\n", abssubfolder);
				return false;
			}
#else
#error unsupported platform
#endif
		}

		char absfile[FILE_MAX_PATH + 1];
		memset(absfile, 0, FILE_MAX_PATH + 1);
		cma_abspath(absfile, FILE_MAX_PATH, relpath);
		CopyFileA(abspath, absfile, FALSE);
	}

	return true;
}

int main(int argc, char **argv)
{
	CMakeAutoConfig config = {0};
	if (!cma_parse_args(argc, argv, &config))
		return -1;

	printf("-------------------\n");
	printf("action: %s\narch: %s\nmode: %s\nbuilddir: %s\nsrcdir: %s\noptions: %d\n",
				 config.action == CMAKE_AUTO_ACTION_BUILD				? "build"
				 : config.action == CMAKE_AUTO_ACTION_HELP			? "help"
				 : config.action == CMAKE_AUTO_ACTION_CONFIGURE ? "configure"
				 : config.action == CMAKE_AUTO_ACTION_TEMPLATE	? "template"
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
	case CMAKE_AUTO_ACTION_CONFIGURE:
		cma_init_proj(&config);
		break;
	case CMAKE_AUTO_ACTION_TEMPLATE:
	{
		char buf[FILE_MAX_PATH + 1];
		memset(buf, 0, FILE_MAX_PATH + 1);
#ifdef _WIN32
		GetModuleFileNameA(NULL, buf, FILE_MAX_PATH);
		if (!strlen(buf))
			return;
		strrchr(buf, '\\')[1] = 0;
#else
#error unsupported platform
#endif
		strcat_s(buf, FILE_MAX_PATH, "templates\\");
		strcat_s(buf, FILE_MAX_PATH, config.template);

		DWORD attr = GetFileAttributesA(buf);
		if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
		{
			printf("template not found\n");
			return -1;
		}

		cma_iterate_dir(buf, ".", 0, true, copy_file_callback);

		printf("template copied\n");
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
