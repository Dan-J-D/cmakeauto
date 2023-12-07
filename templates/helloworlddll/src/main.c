#ifdef _WIN32
#include <Windows.h>
#else
#endif

#include <stdio.h>

void invoke_main(void)
{
	printf("Hello World!\n");
}

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
#elif __linux__
int main(void)
#else
#error unsupported platform
#endif
{
#ifdef _WIN32
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
#endif

		invoke_main();

#ifdef _WIN32
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
#elif __linux
	return 0;
#else
#error unsupported platform
#endif
}
