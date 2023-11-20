# RunWithDll

A utility that can be used to launch an executable with a DLL injected.

## Building

```
cl /EHsc runwithdll.cpp
```

## Usage

```
runwithdll.exe <DllName> <TargetCommmandLine>
```

Example, running "cmd.exe" with "dbgcore.dll" loaded in the process:

```
runwithdll C:\windows\system32\dbgcore.dll cmd.exe
```

