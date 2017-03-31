#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

// http://stackoverflow.com/questions/194465/how-to-parse-a-string-to-an-int-in-c/6154614#6154614
enum STR2INT_ERROR
{
	SUCCESS, OVERFLOW, UNDERFLOW, INCONVERTIBLE
};

STR2INT_ERROR str2int(long &i, const char *s, int base = 0)
{
	char *end;
	long  l;
	errno = 0;
	l = strtol(s, &end, base);
	if ((errno == ERANGE && l == LONG_MAX) || l > INT_MAX)
	{
		return OVERFLOW;
	}
	if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN)
	{
		return UNDERFLOW;
	}
	if (*s == '\0' || *end != '\0')
	{
		return INCONVERTIBLE;
	}
	i = l;
	return SUCCESS;
}

// http://stackoverflow.com/questions/3828835/how-can-we-check-if-a-file-exists-or-not-using-win32-program
bool FileExists(char * filePath)
{
	WIN32_FIND_DATAA findFileData;
	HANDLE handle = FindFirstFileA(filePath, &findFileData);

	if (handle != INVALID_HANDLE_VALUE)
	{
		FindClose(handle);
		return true;
	}

	return false;
}

bool Inject(const char* dllPath, DWORD process_id)
{
	bool succeeded = false; // Only set to true if we succeed fully
	// Returning early skips over closing handles and deallocating, so we use this instead

	HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
		FALSE, process_id);

	if (hProcess != NULL)
	{
		// Allocate memory for the dll path
		size_t dllPathSize = sizeof(dllPath[0]) * (strlen(dllPath) + 1); // 1 for null terminator
		void* pAlloc = VirtualAllocEx(hProcess, NULL, dllPathSize, MEM_COMMIT, PAGE_READWRITE);

		if (pAlloc && WriteProcessMemory(hProcess, pAlloc, dllPath, dllPathSize, NULL))
		{
			// Find the processes loadlibrary, any windows app should have their own
			void* loadLibraryFn = GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA");

			if (loadLibraryFn)
			{
				// Call LoadLibraryA through the target process
				HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryFn, pAlloc, 0, NULL);
				if (hThread != NULL)
				{
					// Wait until the thread ends so we don't close the handle early
					WaitForSingleObject(hThread, INFINITE);
					CloseHandle(hThread);
					succeeded = true;
				}
			}
		}

		VirtualFreeEx(hProcess, pAlloc, dllPathSize, MEM_DECOMMIT);
	}

	CloseHandle(hProcess);

	return succeeded;
}

int main(int argc, const char *argv[])
{
	// Usage: interject <PID> <DLL_PATH>
	if (argc != 3)
	{
		puts("Parameter sequence invalid\nUsage: interject <PID> <DLL_PATH>");
		return 1;
	}
	
	long pid;
	if (str2int(pid, argv[1]) != SUCCESS || pid < 0)
	{
		puts("Invalid process ID");
		return 2;
	}

	// TODO: GetFullPathName isn't a safe function...
	char dllPath[MAX_PATH];
	LPSTR* lppPart = { NULL };
	if (GetFullPathNameA(argv[2], MAX_PATH, dllPath, lppPart) == 0)
	{
		puts("GetFullPathName error, sorry :/");
		return 3;
	}

	if (!FileExists(dllPath))
	{
		puts("File doesn't exist");
		return 4;
	}

	if (!Inject(dllPath, pid))
	{
		puts("Injection failed");
		return 5;
	}
}