#include <Windows.h>
#include <stdio.h>

// http://stackoverflow.com/questions/194465/how-to-parse-a-string-to-an-int-in-c/6154614#6154614
enum STR2INT_ERROR
{
	SUCCESS, OVERFLOW, UNDERFLOW, INCONVERTIBLE
};

STR2INT_ERROR str2int(int &i, char const *s, int base = 0)
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

int main(int argc, char** argv)
{
	// Usage: interject <PID> <DLL_PATH>
	// You can
	if (argc != 2)
	{
		puts("Parameter sequence invalid\nUsage: interject <PID> <DLL_PATH>");
		return 1;
	}

	int pid;
	if (str2int(pid, argv[2]) != SUCCESS || pid < 0)
	{
		puts("Invalid process ID");
		return 2;
	}
}