#include <Windows.h>
#include <fstream>
#include <string>

// Writes a message with a timestamp to a log file in the current working directory.
void LogMessage(const std::string& message)
{
	std::ofstream logFile("ProcessorOptimizer.log", std::ios::app);
	if (logFile.is_open())
	{
		SYSTEMTIME st;
		GetLocalTime(&st);

		char timeBuffer[64];
		sprintf_s(timeBuffer, sizeof(timeBuffer), "[%04d-%02d-%02d %02d:%02d:%02d] ",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

		logFile << timeBuffer << message << std::endl;
		logFile.close();
	}
}

// Checks if the current process is running with Administrator privileges.
bool IsProcessRunAsAdmin()
{
	BOOL fIsRunAsAdmin = FALSE;
	PSID pAdministratorsGroup = NULL;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

	// Allocate and initialize a SID of the administrators group.
	if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdministratorsGroup))
	{
		// Check whether the token has administrator privileges.
		CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin);
		FreeSid(pAdministratorsGroup);
	}
	return fIsRunAsAdmin == TRUE;
}

void ApplyCPUAffinityFix()
{
	LogMessage("CPUAffinityFix Module Loaded.");

	// Delay execution to give the game engine enough time to initialize its main threads.
	// 8000ms is usually enough to bypass the initial loading/splash screens.
	Sleep(8000);
	/*
	// Enforce Administrator privileges constraint
	if (!IsProcessRunAsAdmin())
	{
		LogMessage("ERROR: The process does NOT have Administrator privileges.");
		LogMessage("ABORT: Mod requires Admin rights to modify CPU dependencies. Please run the game as Administrator.");
		return;
	}

	LogMessage("Admin privileges verified. Proceeding with CPU affinity adjustment...");
	*/
	HANDLE hProcess = GetCurrentProcess();
	DWORD_PTR processAffinityMask;
	DWORD_PTR systemAffinityMask;

	// 1. Retrieve the current CPU masks for the system and the process.
	if (GetProcessAffinityMask(hProcess, &processAffinityMask, &systemAffinityMask))
	{
		// 2. Use bitwise operations to strip CPU Core 0 from the mask.
		// ((DWORD_PTR)1) is 00...001. Bitwise NOT (~) makes it 11...110.
		// Bitwise AND (&) applies it, effectively disabling Core 0 only.
		DWORD_PTR tempMask = processAffinityMask & ~((DWORD_PTR)1);

		// 3. Apply the temporary mask to force the OS to migrate threads away from Core 0.
		if (SetProcessAffinityMask(hProcess, tempMask))
		{
			LogMessage("SUCCESS: Temporarily removed Core 0 from process affinity.");

			// Short pause to allow the Windows scheduler to migrate the threads.
			Sleep(50);

			// 4. Restore the original mask, granting access to all cores again.
			if (SetProcessAffinityMask(hProcess, processAffinityMask))
			{
				LogMessage("SUCCESS: Original CPU affinity restored. Threads successfully redistributed.");
			}
			else
			{
				LogMessage("CRITICAL ERROR: Failed to restore original CPU affinity mask.");
			}
		}
		else
		{
			LogMessage("ERROR: Failed to set temporary CPU affinity mask. Error Code: " + std::to_string(GetLastError()));
		}
	}
	else
	{
		LogMessage("ERROR: Failed to retrieve process affinity mask. Error Code: " + std::to_string(GetLastError()));
	}

	LogMessage("CPUAffinityFix routine completed.");
}

// Thread wrapper to prevent Loader Lock during DLL initialization.
DWORD WINAPI MainThread(LPVOID lpParam)
{
	ApplyCPUAffinityFix();

	// Free the thread once the fix is applied so it doesn't consume resources.
	FreeLibraryAndExitThread((HMODULE)lpParam, 0);
	return 0;
}

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:

		DisableThreadLibraryCalls(hinstDLL);

		CreateThread(nullptr, 0, MainThread, hinstDLL, 0, nullptr);

		break;

	case DLL_THREAD_ATTACH:

		break;

	case DLL_THREAD_DETACH:

		break;

	case DLL_PROCESS_DETACH:

		if (lpvReserved != nullptr)
		{
			break;
		}

		break;
	}
	return TRUE;
}