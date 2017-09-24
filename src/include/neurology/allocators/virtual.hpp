#pragma once

#include <windows.h>

#include <neurology/address.hpp>
#include <neurology/allocators/void.hpp>
#include <neurology/win32/access.hpp>
#include <neurology/win32/handle.hpp>

namespace Neurology
{
   class VirtualAllocator;
   
   class Page : public Allocation
   {
      friend VirtualAllocator;
      
   public:
      struct Protection
      {
         union
         {
            struct
            {
               BYTE noAccess : 1;
               BYTE readOnly : 1;
               BYTE readWrite : 1;
               BYTE writeCopy : 1;
               BYTE execute : 1;
               BYTE executeRead : 1;
               BYTE executeReadWrite : 1;
               BYTE executeWriteCopy : 1;
               BYTE guard : 1;
               BYTE noCache : 1;
               BYTE writeCombine : 1;
               BYTE __padding : 19;
               BYTE targetsInvalid : 1;
               BYTE revertToFileMap : 1;
            };
            DWORD mask;
         };

         Protection(void) { this->mask = 0; }
         Protection(DWORD mask) { this->mask = mask; }
         operator DWORD (void) { return this->mask; }
      };

      struct State
      {
         union
         {
            struct
            {
               BYTE __padding : 12;
               BYTE commit : 1;
               BYTE reserve : 1;
               BYTE decommit : 1;
               BYTE release : 1;
               BYTE free : 1;
               BYTE memPrivate : 1;
               BYTE mapped : 1;
               BYTE reset : 1;
               BYTE topDown : 1;
               BYTE writeWatch : 1;
               BYTE physical : 1;
               BYTE rotate : 1;
               BYTE resetUndo : 1;
               BYTE __padding2 : 5;
               BYTE largePages : 1;
               BYTE __undefined : 1;
               BYTE fourMBPages : 1;
            };
            DWORD mask;
         };

         State(void) { this->mask = 0; }
         State(DWORD mask) { this->mask = mask; }
         operator DWORD (void) { return this->mask; }
      };
      
   protected:
      bool ownedAllocation;
      VirtualAllocator *allocator;
      Object<MEMORY_BASIC_INFORMATION> memoryInfo;

   public:
      Page(void);
      Page(VirtualAllocator *allocator);
      Page(VirtualAllocator *allocator, Address address);
      Page(Page &page);
      ~Page(void) {}

      Page &operator=(Page &page);
      
      void query(void);

      Address allocationBase(void);
      Protection allocationProtect(void);
      State state(void);
      Protection protection(void);
      State type(void);
   };

   class VirtualAllocator : public Allocator
   {
      friend Page;
      
   public:
      class Exception : public Allocator::Exception
      {
      public:
         VirtualAllocator &allocator;

         Exception(VirtualAllocator &allocator, const LPWSTR message);
      };

      typedef std::set<Page> PageObjectSet;
      typedef std::map<const Address, Page *> PageObjectMap;

   protected:
      PageObjectSet pageObjects;
      PageObjectMap pages;
      Handle processHandle;
      Page::State defaultAllocation;
      Page::State defaultProtection;
      
   public:
      static VirtualAllocator Instance;

      VirtualAllocator(void);
      VirtualAllocator(Handle &processHandle);

      bool hasPage(Page &page) const noexcept;

      void throwIfNoPage(Page &page) const;
      
      void setProcessHandle(Handle &handle);
      void setDefaultAllocation(Page::State state);
      void setDefaultProtection(Page::State state);

      Page &pageOf(Address address);
      Page &allocate(SIZE_T size, Page::State allocationType, Page::State protection);
      Page &allocate(Address address, SIZE_T size, Page::State allocationType, Page::State protection);
      
      void lock(Page &page);
      void unlock(Page &page);
      
      void protect(Page &page, Page::Protection protection);
      
      SIZE_T query(Page &page);
      SIZE_T query(Address address, PMEMORY_BASIC_INFORMATION buffer, SIZE_T length);

      void enumerate(void);

   protected:
      virtual Address poolAddress(SIZE_T size);
      Address poolAddress(Address address, SIZE_T size, Page::State allocationType, Page::State protection);
      
      virtual Address repoolAddress(Address &address, SIZE_T size);
      virtual void unpoolAddress(Address &address);

      void createPage(Address &address, bool owned);
      void freePage(Page &page);

      virtual void allocate(Allocation *allocation, SIZE_T size);
   };
}
