# Interject
Super simple LoadLibrary injector with safe Windows API usage (no handle/memory leaks on fail)

Usage: `Interject.exe <Process_id> <DLL_Path>`  
To get the PID of the target process, use tasklist

OR with a powershell script:
$processId = ps <Process_name> | select -expand Id
.\Interject.exe $processId .\<dll>.dll
