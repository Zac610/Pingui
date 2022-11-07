#ifndef _SYSPLATFORM_H_
#define _SYSPLATFORM_H_

#ifdef PLATFORM_LINUX
#endif // PLATFORM_LINUX

#ifdef PLATFORM_WIN32
#endif // PLATFORM_WIN32

void Execute(const std::string &_cmd, const std::string &_param)
{
#ifdef PLATFORM_WIN32
   // additional information
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   // set the size of the structures
   ZeroMemory( &si, sizeof(si) );
   si.cb = sizeof(si);
   ZeroMemory( &pi, sizeof(pi) );

   std::string cmdFull = _cmd + " " + _param;

  // start the program up
  CreateProcess( NULL,   // the path
    cmdFull.c_str(),        // Command line
    NULL,           // Process handle not inheritable
    NULL,           // Thread handle not inheritable
    FALSE,          // Set handle inheritance to FALSE
    0,              // No creation flags
    NULL,           // Use parent's environment block
    NULL,           // Use parent's starting directory
    &si,            // Pointer to STARTUPINFO structure
    &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );
    // Close process and thread handles.
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
#endif
}

#endif
