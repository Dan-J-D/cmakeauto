#ifndef CMAKEAUTO_H_
#define CMAKEAUTO_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#define FILE_MAX_PATH _MAX_PATH
#else
#error unsupported platform
#endif

#define GERNERATOR_MAX_LEN 64

typedef enum
{
	CMAKE_AUTO_ACTION_UNKNOWN,

	CMAKE_AUTO_ACTION_BUILD,
	CMAKE_AUTO_ACTION_HELP,
	CMAKE_AUTO_ACTION_CONFIGURE,
	CMAKE_AUTO_ACTION_TEMPLATE,
} CMakeAutoAction;

typedef enum
{
	CMAKE_AUTO_ARCH_UNKONWN,

	CMAKE_AUTO_ARCH_X86,
	CMAKE_AUTO_ARCH_X64,
} CMakeAutoArchitecture;

typedef enum
{
	CMAKE_AUTO_MODE_UNKNOWN,

	CMAKE_AUTO_MODE_DEBUG,
	CMAKE_AUTO_MODE_RELEASE,
} CMakeAutoMode;

typedef enum
{
	CMAKE_AUTO_OPTION_NONE,
	CMAKE_AUTO_OPTION_AUTO_RELOAD = (1 << 0),
} CMakeAutoOptions;

typedef struct
{
	CMakeAutoAction action;
	CMakeAutoArchitecture arch;
	CMakeAutoMode mode;
	int options;

	char builddir[FILE_MAX_PATH];
	char srcdir[FILE_MAX_PATH];
	char generator[GERNERATOR_MAX_LEN];
	char *template;
	char *extra_init_args;
	char *extra_build_args;

	void *watchfolderhandle;
} CMakeAutoConfig;

/**
 * @brief Prints this program's usage
 */
void cma_print_usage();

/**
 * @brief Gets the absolute path of a path
 *
 * @param buf
 * @param size
 * @param path
 * @return true if succeeded
 * @return false if failed
 */
bool cma_abspath(char *buf, size_t size, const char *path);

void cma_iterate_dir(const char *abspath,
										 const char *relpath, // default should be "./"
										 void *userdata,
										 bool should_iter_sub_folder,
										 bool (*callback)(const char *abspath, // return false to stop iterating
																			const char *relpath,
																			const char *name,
																			bool isfolder /* 0 = file, 1 = folder */,
																			void *userdata));

/**
 * @brief Creates a process
 *
 * @param filename
 * @param cmdline
 * @param workdir
 * @param processhandle (optional) (out)
 * @param pid (optional) (out)
 * @return true if created successfully
 * @return false if failed
 */
bool cma_create_process(char *filename,
												char *cmdline,
												char *workdir,
												void **processhandle,
												unsigned int *pid);

bool cma_watch_folder_init(CMakeAutoConfig *config);
bool cma_watch_folder_wait_for_next_change(CMakeAutoConfig *config);
bool cma_watch_folder_close(CMakeAutoConfig *config);

/**
 * @brief Parses command line arguments
 *
 * @param argc
 * @param argv
 * @param config (out)
 * @return true if parsed successfully
 * @return false if failed
 */
bool cma_parse_args(int argc, char **argv, CMakeAutoConfig *config);

/**
 * @brief Initalized Project Files
 *
 * @param config
 * @return true if process created & exited correctly
 * @return false if failed
 */
bool cma_init_proj(CMakeAutoConfig *config);
/**
 * @brief Builds Project Files
 *
 * @param config
 * @return true if process created & exited correctly
 * @return false if failed
 */
bool cma_build(CMakeAutoConfig *config);

#endif // CMAKEAUTO_H_
