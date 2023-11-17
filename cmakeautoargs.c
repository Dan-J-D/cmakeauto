#include "cmakeauto.h"

void cma_print_usage()
{
	printf("Usage: cmakeauto (help|build) -b <build path> -s <source path> [-g <Generator>] [-m <Mode>] [-a <Architecture>] [-ei <Extra Init Params>] [-eb <Extra Build Params>] [-ar]\n");
	printf("\thelp: print help message\n");
	printf("\tbuild: build project\n");
	printf("\nParameters:\n\t-b <build path>: (Default: build) This specifies where the project would be built.\n");
	printf("\t-s <source path>: (Default: src) This specifies where the project source is.\n");
	printf("\t-g <Generator Name>: (Optional) This specifies the generator to be used.\n");
	printf("\t-m: (Default: release) This specifies the build mode to be Debug.\n");
	printf("\t\tdebug: Debug mode\n");
	printf("\t\trelease: Release mode\n");
	printf("\t-a: (Default: x64) This specifies the architecture to be built.\n");
	printf("\t\tx86: 32-bit\n");
	printf("\t\tx64: 64-bit\n");
	printf("\t-ei: (Optional) This specifies extra init params to be passed to cmake.\n");
	printf("\t-eb: (Optional) This specifies extra build params to be passed to cmake.\n");
	printf("\t-ar: (Optional) This specifies whether to auto reload the project when source files changed.\n");
}

bool cma_parse_args(int argc, char **argv, CMakeAutoConfig *config)
{
	if (argc < 2)
	{
		config->action = CMAKE_AUTO_ACTION_UNKNOWN;
		return config;
	}

	if (stricmp(argv[1], "build") == 0)
	{
		config->action = CMAKE_AUTO_ACTION_BUILD;
		config->mode = CMAKE_AUTO_MODE_RELEASE;
		config->arch = CMAKE_AUTO_ARCH_X64;
		if (!cma_abspath(config->builddir, FILE_MAX_PATH, "build"))
			memset(config->builddir, 0, FILE_MAX_PATH);
		if (!cma_abspath(config->srcdir, FILE_MAX_PATH, "src"))
			memset(config->srcdir, 0, FILE_MAX_PATH);

		for (int i = 2; i < argc; i++)
		{
			if (stricmp(argv[i], "-s") == 0)
			{
				if (i + 1 < argc)
				{
					if (!cma_abspath(config->srcdir, FILE_MAX_PATH, argv[i + 1]))
					{
						printf("invalid or error happened when parsing source dir\n");
						return false;
					}
					i++;
				}
				else
				{
					printf("no source dir specified\n");
					return false;
				}
			}
			else if (stricmp(argv[i], "-b") == 0)
			{
				if (i + 1 < argc)
				{
					if (!cma_abspath(config->builddir, FILE_MAX_PATH, argv[i + 1]))
					{
						printf("invalid or error happened when parsing build dir\n");
						return false;
					}
					i++;
				}
				else
				{
					printf("no build dir specified\n");
					return false;
				}
			}
			else if (stricmp(argv[i], "-g") == 0)
			{
				if (i + 1 < argc)
				{
					if (strcpy_s(config->generator, GERNERATOR_MAX_LEN, argv[i + 1]))
					{
						printf("invalid or error happened when parsing generator\n");
						return false;
					}
					i++;
				}
			}
			else if (stricmp(argv[i], "-m") == 0)
			{
				if (i + 1 < argc)
				{
					if (stricmp(argv[i + 1], "debug") == 0)
						config->mode = CMAKE_AUTO_MODE_DEBUG;
					else if (stricmp(argv[i + 1], "release") == 0)
						config->mode = CMAKE_AUTO_MODE_RELEASE;
					else
					{
						printf("no build mode specified\n");
						return false;
					}
					i++;
				}
				else
				{
					printf("no build mode specified\n");
					return false;
				}
			}
			else if (stricmp(argv[i], "-a") == 0)
			{
				if (i + 1 < argc)
				{
					if (stricmp(argv[i + 1], "x86") == 0)
						config->arch = CMAKE_AUTO_ARCH_X86;
					else if (stricmp(argv[i + 1], "x64") == 0)
						config->arch = CMAKE_AUTO_ARCH_X64;
					else
					{
						printf("no build architecture specified\n");
						return false;
					}
					i++;
				}
				else
				{
					printf("no build architecture specified\n");
					return false;
				}
			}
			else if (stricmp(argv[i], "-ei") == 0)
			{
				if (i + 1 < argc)
				{
					config->extra_init_args = argv[i + 1];
					i++;
				}
			}
			else if (stricmp(argv[i], "-eb") == 0)
			{
				if (i + 1 < argc)
				{
					config->extra_build_args = argv[i + 1];
					i++;
				}
			}
			else if (stricmp(argv[i], "-ar") == 0)
			{
				config->options |= CMAKE_AUTO_OPTION_AUTO_RELOAD;
			}
			else if (stricmp(argv[i], "-h") == 0)
			{
				config->action = CMAKE_AUTO_ACTION_HELP;
				return true;
			}
			else
			{
				printf("unknown argument: %s\n", argv[i]);
				return false;
			}
		}

		if (!config->builddir ||
				!config->srcdir ||
				config->mode == CMAKE_AUTO_MODE_UNKNOWN ||
				config->action == CMAKE_AUTO_ACTION_UNKNOWN ||
				config->arch == CMAKE_AUTO_ARCH_UNKONWN)
		{
			printf("invalid build arguments\n");
			return false;
		}
	}
	else if (stricmp(argv[1], "help") == 0 || stricmp(argv[1], "-h") == 0)
	{
		config->action = CMAKE_AUTO_ACTION_HELP;
	}

	return true;
}
