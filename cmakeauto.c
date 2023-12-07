#include "cmakeauto.h"

bool cma_abspath(char *buf, size_t size, const char *path)
{
#ifdef _WIN32
	return _fullpath(buf, path, size) != NULL;
#elif __linux__
	return realpath(path, buf) != NULL;
#else
#error unsupported platform
#endif
}

bool cma_create_dir_include_existing(const char *path)
{
#ifdef _WIN32
	return CreateDirectoryA(path, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
#elif __linux__
	return mkdir(path, 0755) == 0 || errno == EEXIST;
#else
#error unsupported platform
#endif
}

bool cma_file_exists(const char *path)
{
#ifdef _WIN32
	DWORD attr = GetFileAttributesA(path);
	return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#elif __linux__
	struct stat st = {0};
	return stat(path, &st) != -1 && !S_ISDIR(st.st_mode);
#else
#error unsupported platform
#endif
}

bool cma_copy_file(const char *src, const char *dst)
{
#ifdef _WIN32
	return CopyFileA(src, dst, FALSE) != 0;
#elif __linux__
	int srcfd = open(src, O_RDONLY);
	if (srcfd == -1)
		return false;

	int dstfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (dstfd == -1)
	{
		close(srcfd);
		return false;
	}

	char buf[4096];
	ssize_t bytes_read;
	while ((bytes_read = read(srcfd, buf, sizeof(buf))) > 0)
	{
		char *p = buf;
		while (bytes_read > 0)
		{
			ssize_t bytes_written = write(dstfd, p, bytes_read);
			if (bytes_written == -1)
			{
				close(srcfd);
				close(dstfd);
				return false;
			}
			bytes_read -= bytes_written;
			p += bytes_written;
		}
	}

	close(srcfd);
	close(dstfd);
	return true;
#else
#error unsupported platform
#endif
}

bool cma_get_current_process_absfilepath(char *buf, size_t size)
{
#ifdef _WIN32
	return GetModuleFileNameA(NULL, buf, size) != 0;
#elif __linux__
	return readlink("/proc/self/exe", buf, size) != 0;
#else
#error unsupported platform
#endif
}

bool cma_get_workdir(char *buf, size_t size)
{
#ifdef _WIN32
	return GetCurrentDirectoryA(size, buf) != 0;
#elif __linux__
	return getcwd(buf, size) != NULL;
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
	strcat_s(abspath_search, FILE_MAX_PATH, FILE_SEPERATOR "*.*");

	WIN32_FIND_DATA finddata;
	HANDLE findhandle = FindFirstFileA(abspath_search, &finddata);
	if (findhandle == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (strcmp(finddata.cFileName, ".") == 0 || strcmp(finddata.cFileName, "..") == 0)
			continue;

		char fileabspath[FILE_MAX_PATH + 1];
		sprintf_s(fileabspath, FILE_MAX_PATH, "%s" FILE_SEPERATOR "%s", abspath, finddata.cFileName);
		char filerelpath[FILE_MAX_PATH + 1];
		sprintf_s(filerelpath, FILE_MAX_PATH, "%s" FILE_SEPERATOR "%s", relpath, finddata.cFileName);

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
#elif __linux__
	DIR *directory = opendir(abspath);
	if (directory == NULL)
	{
		printf("failed to open directory %s\n", abspath);
		return;
	}

	struct dirent *entry;
	while ((entry = readdir(directory)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		char fileabspath[FILE_MAX_PATH + 1];
		sprintf_s(fileabspath, FILE_MAX_PATH, "%s/%s", abspath, entry->d_name);
		char filerelpath[FILE_MAX_PATH + 1];
		sprintf_s(filerelpath, FILE_MAX_PATH, "%s/%s", relpath, entry->d_name);

		if (entry->d_type == DT_DIR)
		{
			if (should_iter_sub_folder)
				cma_iterate_dir(fileabspath, filerelpath, userdata, should_iter_sub_folder, callback);
			if (!callback(fileabspath, filerelpath, entry->d_name, true, userdata))
				break;
		}
		else
		{
			if (!callback(fileabspath, filerelpath, entry->d_name, false, userdata))
				break;
		}
	}

	closedir(directory);
#else
#error unsupported platform
#endif
}

bool cma_create_process(char *filename,
												char *cmdline,
												char *posix_args[],
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
#elif __linux__
	char filepath[FILE_MAX_PATH + 1];
	cma_abspath(filepath, FILE_MAX_PATH, filename);

	int pid_ = fork();
	if (pid_ == 0) // child process
	{
		if (workdir)
			chdir(workdir);

		if (posix_args)
			execv(filepath, posix_args);
		else
			execl(filepath, (const char *)"", (char *)NULL);

		printf("failed to create process reason: %d\n", errno);
		return false;
	}
	else if (pid_ == -1)
	{
		printf("failed to create process reason: %d\n", errno);
		return false;
	}
	else
	{
		if (pid)
			*pid = (unsigned int)pid_;
	}
	return true;

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
	bool is_generator_present = config->generator && strlen(config->generator) > 0;
	char cmdline[1024];
	memset(cmdline, 0, 1024);
	sprintf_s(cmdline, 1024, "cmake -S \"%s\" -B \"%s\" %s%s%s %s %s %s",
						config->srcdir,
						config->builddir,
						is_generator_present ? "-G \"" : "",
						is_generator_present ? config->generator : "",
						is_generator_present ? "\"" : "",
						config->arch != CMAKE_AUTO_ARCH_UNKONWN ? "-A" : "",
						config->arch
								? config->arch == CMAKE_AUTO_ARCH_X86		? "Win32"
									: config->arch == CMAKE_AUTO_ARCH_X64 ? "x64"
																												: "Unknown"
								: "",
						config->extra_init_args ? config->extra_init_args : "");

#ifdef _WIN32
	void *processhandle = 0;
	if (!cma_create_process(NULL, cmdline, NULL, NULL, &processhandle, 0))
	{
		printf("failed to create cma_init_proj process\n");
		return false;
	}

	printf("> %s\n", cmdline);

	WaitForSingleObject(processhandle, INFINITE);

	unsigned long exit_code = 0;
	GetExitCodeProcess(processhandle, &exit_code);
	printf("> exited with code %ld\n", exit_code);
#elif __linux
	char workdir[FILE_MAX_PATH + 1];
	memset(workdir, 0, FILE_MAX_PATH + 1);
	cma_get_workdir(workdir, FILE_MAX_PATH);

	char *args[] = {
			"sh",
			"-c",
			(char *)cmdline,
			0,
	};

	pid_t pid;
	if (!cma_create_process("/bin/sh", NULL, args, workdir, NULL, (unsigned int *)&pid))
	{
		printf("failed to create cma_init_proj process\n");
		return false;
	}

	printf("> ");
	for (int i = 0; i < (sizeof(args) / sizeof(args[0])) - 1; i++)
	{
		if (args[i] == cmdline)
			printf("\"%s\" ", cmdline);
		else
			printf("%s ", args[i]);
	}
	printf("\n");

	int exit_code = 0;
	if (waitpid(pid, &exit_code, 0) == -1)
	{
		printf("failed to wait for process\n");
		return false;
	}
	printf("> exited with code %d\n", exit_code);
#else
#error unsupported platform
#endif

	return exit_code == 0;
}

bool cma_build(CMakeAutoConfig *config)
{
	char cmdline[1024];
	memset(cmdline, 0, 1024);
	sprintf_s(cmdline, 1024, "cmake --build \"%s\" --config %s %s",
						config->builddir,
						config->mode == CMAKE_AUTO_MODE_DEBUG			? "Debug"
						: config->mode == CMAKE_AUTO_MODE_RELEASE ? "Release"
																											: "Unknown",
						config->extra_build_args ? config->extra_build_args : "");

#ifdef _WIN32
	void *processhandle = 0;
	if (!cma_create_process(NULL, cmdline, NULL, NULL, &processhandle, 0))
	{
		printf("failed to create cma_init_proj process\n");
		return false;
	}

	printf("> %s\n", cmdline);

	WaitForSingleObject(processhandle, INFINITE);

	unsigned long exit_code = 0;
	GetExitCodeProcess(processhandle, &exit_code);
	printf("> exited with code %ld\n", exit_code);
#elif __linux
	char workdir[FILE_MAX_PATH + 1];
	memset(workdir, 0, FILE_MAX_PATH + 1);
	cma_get_workdir(workdir, FILE_MAX_PATH);

	char *args[] = {
			"sh",
			"-c",
			(char *)cmdline,
			0,
	};

	pid_t pid;
	if (!cma_create_process("/bin/sh", NULL, args, workdir, NULL, (unsigned int *)&pid))
	{
		printf("failed to create cma_init_proj process\n");
		return false;
	}

	printf("> ");
	for (int i = 0; i < (sizeof(args) / sizeof(args[0])) - 1; i++)
	{
		if (args[i] == cmdline)
			printf("\"%s\" ", cmdline);
		else
			printf("%s ", args[i]);
	}
	printf("\n");

	int exit_code = 0;
	if (waitpid(pid, &exit_code, 0) == -1)
	{
		printf("failed to wait for process\n");
		return false;
	}
	printf("> exited with code %d\n", exit_code);
#else
#error unsupported platform
#endif

	return exit_code == 0;
}

bool copy_file_callback(const char *abspath, const char *relpath, const char *name, bool isfolder, void *userdata)
{
	if (!isfolder)
	{
		char *folder = (char *)relpath;

		while (folder = strchr(folder + 1, FILE_SEPERATOR_CHAR))
		{
			char relsubfolder[FILE_MAX_PATH + 1];
			memset(relsubfolder, 0, FILE_MAX_PATH + 1);
			memcpy_s(relsubfolder, FILE_MAX_PATH, relpath, folder - relpath);

			char abssubfolder[FILE_MAX_PATH + 1];
			memset(abssubfolder, 0, FILE_MAX_PATH + 1);
			cma_abspath(abssubfolder, FILE_MAX_PATH, relsubfolder);

			if (!cma_create_dir_include_existing(abssubfolder))
			{
				printf("failed to create directory %s\n", abssubfolder);
				return false;
			}
		}

		char absfile[FILE_MAX_PATH + 1];
		memset(absfile, 0, FILE_MAX_PATH + 1);
		cma_abspath(absfile, FILE_MAX_PATH, relpath);

		if (!cma_copy_file(abspath, absfile))
		{
			printf("failed to copy file %s\n", abspath);
			return false;
		}
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
		if (!cma_get_current_process_absfilepath(buf, FILE_MAX_PATH) || !strlen(buf))
			return -2;
		strrchr(buf, FILE_SEPERATOR_CHAR)[1] = 0;

		strcat_s(buf, FILE_MAX_PATH, "templates" FILE_SEPERATOR);
		strcat_s(buf, FILE_MAX_PATH, config.template);

		if (!cma_file_exists)
		{
			printf("template not found\n");
			break;
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
