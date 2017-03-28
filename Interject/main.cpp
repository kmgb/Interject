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
int fileExists(char * file)
{
	WIN32_FIND_DATAA FindFileData;
	HANDLE handle = FindFirstFileA(file, &FindFileData);
	int found = handle != INVALID_HANDLE_VALUE;
	if (found)
	{
		//FindClose(&handle); this will crash
		FindClose(handle);
	}
	return found;
}

bool Inject(const char* dll_path, DWORD process_id)
{
	bool return_value = false; // Only set to true if we succeed fully

	HANDLE process_handle = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
		FALSE, process_id);

	if (process_handle != NULL)
	{
		size_t path_size = strlen(dll_path) + 1; // 1 for null terminator
		void* path_alloc = VirtualAllocEx(process_handle, NULL, path_size, MEM_COMMIT, PAGE_READWRITE);

		if (path_alloc && WriteProcessMemory(process_handle, path_alloc, dll_path, path_size, NULL))
		{
			void* load_library_fn = GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA");

			if (load_library_fn)
			{
				// Call LoadLibraryA through the target process
				HANDLE thread_handle = CreateRemoteThread(process_handle, NULL, 0, (LPTHREAD_START_ROUTINE)load_library_fn, path_alloc, 0, NULL);
				if (thread_handle != NULL)
				{
					// Wait for the remote thread to end
					WaitForSingleObject(thread_handle, INFINITE);
					CloseHandle(thread_handle);
					return_value = true;
				}
			}
		}
		VirtualFreeEx(process_handle, path_alloc, path_size, MEM_DECOMMIT);
	}

	CloseHandle(process_handle);

	return return_value;
}

int main(int argc, const char *argv[])
{
	// Usage: interject <PID> <DLL_PATH>
	// You can
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
	char dll_path[MAX_PATH];
	LPSTR* lppPart = { NULL };
	if (GetFullPathNameA(argv[2], MAX_PATH, dll_path, lppPart) == 0)
	{
		puts("GetFullPathName error, sorry :/");
		return 3;
	}

	if (!fileExists(dll_path))
	{
		puts("File doesn't exist");
		return 4;
	}

	if (!Inject(dll_path, pid))
	{
		puts("Injection failed");
		return 5;
	}
}