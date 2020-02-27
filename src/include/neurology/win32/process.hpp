#pragma once

#include <windows.h>

#include <string>

#include <neurology/exception.hpp>
#include <neurology/win32/handle.hpp>

namespace Neurology
{
   typedef DWORD PID;
      
   struct AccessMask
   {
      union
      {
         struct
         {
            BYTE terminate : 1;
            BYTE createThread : 1;
            BYTE setSessionID : 1;
            BYTE vmOperation : 1;
            BYTE vmRead : 1;
            BYTE vmWrite : 1;
            BYTE dupHandle : 1;
            BYTE createProcess : 1;
            BYTE setQuota : 1;
            BYTE setInformation : 1;
            BYTE queryInformation : 1;
            BYTE suspendResume : 1;
            BYTE queryLimitedInformation : 1;
            BYTE setLimitedInformation : 1;
         };
         DWORD mask;
      };

      AccessMask(void) { this->mask = 0; }
      AccessMask(DWORD mask) { this->mask = mask; }
      operator DWORD (void) { return this->mask; }
   };

   class Process
   {
   public:
      class Exception : public Neurology::Exception
      {
      public:
         const Process &process;

         Exception(const Process &process, const LPWSTR message);
      };

   protected:
      Handle handle;

   public:
      Process(void);
      Process(Handle handle);
      Process(AccessMask access, PID pid);
      Process(AccessMask access, BOOL inheritHandle, PID pid);
      Process(Process &process);

      static Process Spawn(std::wstring cmdLine);
      static Process Spawn(std::wstring cmdLine, DWORD flags);
      static Handle CurrentProcessHandle(void);
      static Process CurrentProcess(void);

      Process &operator=(Process &process);
      
      Handle &getHandle(void);
      const Handle &getHandle(void) const;

      bool isAlive(void) const;

      PID pid(void) const;

      void open(PID pid);
      void open(AccessMask access);
      void open(AccessMask access, PID pid);
      void open(AccessMask access, BOOL inheritHandle, PID pid);
      void close(void);
      void kill(UINT exitCode);
   };
}
