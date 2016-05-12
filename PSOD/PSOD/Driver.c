#include <ntddk.h>
#include <wdf.h>

#define IOCTL_PSOD_CREATE_CALLBACK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_PSOD_BUG_CHECK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)

NTSTATUS	PSOD_IoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID        IrpUnloadHandler(IN PDRIVER_OBJECT DriverObject);
NTSTATUS    IrpCloseHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    IrpCreateHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    IrpNotImplementedHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

EVT_WDF_DRIVER_DEVICE_ADD KmdfPSODEvtDeviceAdd;
KBUGCHECK_REASON_CALLBACK_RECORD callbackRec;

VOID BugCheckDumpIoCallback(KBUGCHECK_CALLBACK_REASON Reason, struct _KBUGCHECK_REASON_CALLBACK_RECORD *Record, PVOID ReasonSpecificData, ULONG ReasonSpecificDataLength)
{
	UNREFERENCED_PARAMETER(Reason);
	UNREFERENCED_PARAMETER(Record);
	UNREFERENCED_PARAMETER(ReasonSpecificData);
	UNREFERENCED_PARAMETER(ReasonSpecificDataLength);
	__asm {
		mov edx, 3C8h; DAC color index port
		mov al, 4; background color
		out dx, al
		mov edx, 0x3C9; DAC color component port
		mov al, 0xFF; RED
		out dx, al
		mov al, 0x69; GREEN
		out dx, al
		mov al, 0xB4; BLUE
		out dx, al
		dec edx
		mov al, 0Fh;Text color
		out dx, al
		mov edx, 0x3C9
		mov al, 0x00; RED
		out dx, al
		mov al, 0x00; GREEN
		out dx, al
		mov al, 0x00; BLUE
		out dx, al
	}
}
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNICODE_STRING DeviceName, Win32Device;
	NTSTATUS status;
	WDF_DRIVER_CONFIG config;
	PDEVICE_OBJECT DeviceObject = NULL;

	RtlInitUnicodeString(&DeviceName, L"\\Device\\PSOD");
	RtlInitUnicodeString(&Win32Device, L"\\DosDevices\\PSOD");

	WDF_DRIVER_CONFIG_INIT(&config, KmdfPSODEvtDeviceAdd);
	status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
	status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
	for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = IrpNotImplementedHandler;
	}
	DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpCreateHandler;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpCloseHandler;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PSOD_IoControl;

	DriverObject->DriverUnload = IrpUnloadHandler;

	DeviceObject->Flags |= DO_DIRECT_IO;
	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	status = IoCreateSymbolicLink(&Win32Device, &DeviceName);
	return status;
}

NTSTATUS KmdfPSODEvtDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
	UNREFERENCED_PARAMETER(Driver);

	NTSTATUS status;
	WDFDEVICE hDevice;

	status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &hDevice);
	return status;
}

NTSTATUS PSOD_HandleIOCTL_CREATE_CALLBACK(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, unsigned int *pdwDataWritten)
{
	UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(pIoStackIrp);
	UNREFERENCED_PARAMETER(pdwDataWritten);

	NTSTATUS status = STATUS_UNSUCCESSFUL;
	KeInitializeCallbackRecord(&callbackRec);
	status = KeRegisterBugCheckReasonCallback(&callbackRec, (PKBUGCHECK_REASON_CALLBACK_ROUTINE)&BugCheckDumpIoCallback, (KBUGCHECK_CALLBACK_REASON)1, (PUCHAR) "BUGCHECK");
	Irp->IoStatus.Information = 0;
	if (status == STATUS_SUCCESS){
		Irp->IoStatus.Status = STATUS_SUCCESS;
	}
	else {
		Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	}
	return status;
}

NTSTATUS PSOD_HandleIOCTL_BUG_CHECK(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, unsigned int *pdwDataWritten)
{
	UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(pIoStackIrp);
	UNREFERENCED_PARAMETER(pdwDataWritten);
	KeBugCheckEx(0x1234, 0, 1, 2, 3);
}

NTSTATUS IrpCreateHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS IrpCloseHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

VOID IrpUnloadHandler(IN PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING DosDeviceName = { 0 };
	RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\PSOD");
	IoDeleteSymbolicLink(&DosDeviceName);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS IrpNotImplementedHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_NOT_SUPPORTED;
}

NTSTATUS PSOD_IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	
	NTSTATUS NtStatus = STATUS_NOT_SUPPORTED;
	unsigned int dwDataWritten = 0;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	if (pIoStackIrp)
	{
		switch (pIoStackIrp->Parameters.DeviceIoControl.IoControlCode){
			case IOCTL_PSOD_CREATE_CALLBACK:
				NtStatus = PSOD_HandleIOCTL_CREATE_CALLBACK(Irp, pIoStackIrp, &dwDataWritten);
				break;

			case IOCTL_PSOD_BUG_CHECK:
				NtStatus = PSOD_HandleIOCTL_BUG_CHECK(Irp, pIoStackIrp, &dwDataWritten);
				break;
			default:
				NtStatus = STATUS_INVALID_DEVICE_REQUEST;
				break;
		}
	}

	Irp->IoStatus.Status = NtStatus;
	Irp->IoStatus.Information = dwDataWritten;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return NtStatus;
}
