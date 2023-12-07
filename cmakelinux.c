#include "cmakeauto.h"

#include <stdarg.h>

#ifdef __linux__

bool strcpy_s(
		char *_Destination,
		unsigned long long _SizeInBytes,
		char const *_Source)
{
	if (_SizeInBytes < strlen(_Source) + 1)
		return 1;
	strcpy(_Destination, _Source);
	return 0;
}

bool strcat_s(
		char *_Destination,
		unsigned long long _SizeInBytes,
		char const *_Source)
{
	if (_SizeInBytes < strlen(_Destination) + strlen(_Source) + 1)
		return 1;
	strcat(_Destination, _Source);
	return 0;
}

bool memcpy_s(
		void *const _Destination,
		unsigned long long const _DestinationSize,
		void const *const _Source,
		unsigned long long const _SourceSize)
{
	if (_DestinationSize < _SourceSize)
		return 1;
	memcpy(_Destination, _Source, _SourceSize);
	return 0;
}

int sprintf_s(
		char *const _Buffer,
		unsigned long long const _BufferCount,
		char const *const _Format,
		...)
{
	va_list args;
	va_start(args, _Format);
	int ret = vsprintf(_Buffer, _Format, args);
	va_end(args);
	return ret;
}

#endif
