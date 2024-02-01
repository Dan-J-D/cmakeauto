#include <ntdef.h>
#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>
#include <stdbool.h>

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING registryPath)
{
	DbgPrintEx(0, 0, "Hello World!");
	return STATUS_SUCCESS;
}
