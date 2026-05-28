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
	LogMessage("CPUAffinityFix Module Loaded. Starting multi-core redistribution.");

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
		// Calculate the maximum number of cores supported by the OS architecture (32 or 64)
		int maxBits = sizeof(DWORD_PTR) * 8;
		int coresProcessed = 0;

		// 2. Iterate through all possible CPU cores
		for (int i = 0; i < maxBits; ++i)
		{
			// Check if the current core (bit 'i') is part of the process affinity mask
			DWORD_PTR currentCoreMask = ((DWORD_PTR)1 << i);

			if (processAffinityMask & currentCoreMask)
			{
				// 3. Create a temporary mask with ONLY this core disabled
				DWORD_PTR tempMask = processAffinityMask & ~currentCoreMask;

				// SAFETY CHECK: If removing this core results in a mask of 0, 
				// SetProcessAffinityMask will fail. Skip to prevent locking the thread.
				if (tempMask == 0)
				{
					LogMessage("WARNING: Core " + std::to_string(i) + " is the only available core. Skipping to prevent OS rejection.");
					continue;
				}

				// 4. Apply the temporary mask to force threads off the current core
				if (SetProcessAffinityMask(hProcess, tempMask))
				{
					LogMessage("SUCCESS: Temporarily removed Core " + std::to_string(i) + ".");

					// Pause to allow the Windows scheduler to migrate the threads away from this core.
					Sleep(50);

					// 5. Restore the original mask, granting access to the core again.
					if (SetProcessAffinityMask(hProcess, processAffinityMask))
					{
						LogMessage("SUCCESS: Restored Core " + std::to_string(i) + ". Threads redistributed.");
					}
					else
					{
						LogMessage("CRITICAL ERROR: Failed to restore Core " + std::to_string(i) + " mask.");
					}

					// Extra pause: Let the threads settle into their new distribution 
					// before we yank the next core out from under them.
					Sleep(50);
				}
				else
				{
					LogMessage("ERROR: Failed to mask Core " + std::to_string(i) + ". Error Code: " + std::to_string(GetLastError()));
				}

				coresProcessed++;
			}
		}

		LogMessage("CPUAffinityFix routine completed. Processed " + std::to_string(coresProcessed) + " active cores.");
	}
	else
	{
		LogMessage("ERROR: Failed to retrieve process affinity mask. Error Code: " + std::to_string(GetLastError()));
	}
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