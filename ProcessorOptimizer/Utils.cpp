#include "Utils.h"

namespace Utils {
    std::string GetCurrentDirectoryPath(HMODULE hModule) {
        char path[MAX_PATH];
        // Get the full path of the loaded DLL
        GetModuleFileNameA(hModule, path, MAX_PATH);

        std::string fullPath(path);
        size_t pos = fullPath.find_last_of("\\/");

        // Return the directory path without the filename
        return (std::string::npos != pos) ? fullPath.substr(0, pos) : "";
    }

    bool IsProcessRunAsAdmin() {
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
}