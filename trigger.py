import ctypes
import ctypes.wintypes as wintypes
from ctypes import windll

# https://gist.github.com/santa4nt/11068180

LPDWORD = ctypes.POINTER(wintypes.DWORD)
LPOVERLAPPED = wintypes.LPVOID
LPSECURITY_ATTRIBUTES = wintypes.LPVOID

GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
GENERIC_EXECUTE = 0x20000000
GENERIC_ALL = 0x10000000

CREATE_NEW = 1
CREATE_ALWAYS = 2
OPEN_EXISTING = 3
OPEN_ALWAYS = 4
TRUNCATE_EXISTING = 5

FILE_ATTRIBUTE_NORMAL = 0x00000080

INVALID_HANDLE_VALUE = -1

NULL = 0
FALSE = wintypes.BOOL(0)
TRUE = wintypes.BOOL(1)

def open_device(device_path, access, mode, creation, flags):
	"""See: CreateFile function
	http://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
	"""
	CreateFile_Fn = windll.kernel32.CreateFileW
	CreateFile_Fn.argtypes = [
	wintypes.LPWSTR,                    # _In_          LPCTSTR lpFileName
	wintypes.DWORD,                     # _In_          DWORD dwDesiredAccess
	wintypes.DWORD,                     # _In_          DWORD dwShareMode
	LPSECURITY_ATTRIBUTES,              # _In_opt_      LPSECURITY_ATTRIBUTES lpSecurityAttributes
	wintypes.DWORD,                     # _In_          DWORD dwCreationDisposition
	wintypes.DWORD,                     # _In_          DWORD dwFlagsAndAttributes
	wintypes.HANDLE]                    # _In_opt_      HANDLE hTemplateFile
	CreateFile_Fn.restype = wintypes.HANDLE

	return wintypes.HANDLE(CreateFile_Fn(device_path,
	access,
	mode,
	NULL,
	creation,
	flags,
	NULL))

def send_ioctl(devhandle, ioctl, inbuf, inbufsiz, outbuf, outbufsiz,):
	"""See: DeviceIoControl function
	http://msdn.microsoft.com/en-us/library/aa363216(v=vs.85).aspx
	"""
	DeviceIoControl_Fn = windll.kernel32.DeviceIoControl
	DeviceIoControl_Fn.argtypes = [
	wintypes.HANDLE,                    # _In_          HANDLE hDevice
	wintypes.DWORD,                     # _In_          DWORD dwIoControlCode
	wintypes.LPVOID,                    # _In_opt_      LPVOID lpInBuffer
	wintypes.DWORD,                     # _In_          DWORD nInBufferSize
	wintypes.LPVOID,                    # _Out_opt_     LPVOID lpOutBuffer
	wintypes.DWORD,                     # _In_          DWORD nOutBufferSize
	LPDWORD,                            # _Out_opt_     LPDWORD lpBytesReturned
	LPOVERLAPPED]                       # _Inout_opt_   LPOVERLAPPED lpOverlapped
	DeviceIoControl_Fn.restype = wintypes.BOOL

	# allocate a DWORD, and take its reference
	dwBytesReturned = wintypes.DWORD(0)
	lpBytesReturned = ctypes.byref(dwBytesReturned)

	status = DeviceIoControl_Fn(devhandle,
	ioctl,
	inbuf,
	inbufsiz,
	outbuf,
	outbufsiz,
	lpBytesReturned,
	None)

	return status, dwBytesReturned


if __name__ == "__main__":
	#define IOCTL_PSOD_CREATE_CALLBACK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
	create_callback_ioctl = 0x22e007
	#define IOCTL_PSOD_BUG_CHECK CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
	bug_check_ioctl = 0x22e00b
	device_handle = open_device("\\\\.\\PSOD", GENERIC_READ | GENERIC_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL)
	send_ioctl(device_handle, create_callback_ioctl, None, 0, None, 0)
	send_ioctl(device_handle, bug_check_ioctl, None, 0, None, 0)