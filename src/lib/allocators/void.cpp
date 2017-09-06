#include <neurology/allocators/allocator.hpp>

using namespace Neurology;

Allocator Allocator::Instance;

Allocation::Exception::Exception
(const Allocation &allocation, const LPWSTR message)
   : Neurology::Exception(message)
   , allocation(allocation)
{
}

Allocation::NoAllocatorException::NoAllocatorException
(const Allocation &allocation)
   : Allocation::Exception(allocation, EXCSTR(L"Allocation must be tied to an Allocator."))
{
}

Allocation::DoubleAllocationException::DoubleAllocationException
(const Allocation &allocation)
   : Allocation::Exception(allocation, EXCSTR(L"Allocation has already been allocated."))
{
}

Allocation::DeadAllocationException::DeadAllocationException
(const Allocation &allocation)
   : Allocation::Exception(allocation, EXCSTR(L"Allocation is currently dead."))
{
}

Allocation::ZeroSizeException::ZeroSizeException
(const Allocation &allocation)
   : Allocation::Exception(allocation, EXCSTR(L"Size of allocation cannot be 0."))
{
}

Allocation::InsufficientSizeException::InsufficientSizeException
(const Allocation &allocation, const SIZE_T size)
   : Allocation::Exception(allocation, EXCSTR(L"Size larger than allocation."))
   , size(size)
{
}

Allocation::AddressOutOfRangeException::AddressOutOfRangeException
(const Allocation &allocation, const Address address, const SIZE_T size)
   : Allocation::Exception(allocation, EXCSTR(L"Provided address and size are out of range of the allocation."))
   , address(address)
   , size(size)
{
}

Allocation::OffsetOutOfRangeException::OffsetOutOfRangeException
(const Allocation &allocation, const SIZE_T offset, const SIZE_T size)
   : Allocation::Exception(allocation, EXCSTR(L"Provided address and size are out of range of the allocation."))
   , offset(offset)
   , size(size)
{
}

Allocation::Allocation
(Allocator *allocator, Address &address, SIZE_T size)
{
   if (allocator == NULL)
      throw NoAllocatorException(*this);

   if (!address.isNull())
      allocator->throwIfNotPooled(&address);

   this->allocator = allocator;
}

Allocation::Allocation
(void)
   : allocator(NULL)
{
}

Allocation::Allocation
(Allocator *allocator)
{
   if (allocator == NULL)
      throw NoAllocatorException(*this);

   this->allocator = allocator;
}

Allocation::Allocation
(Allocation &allocation)
{
   *this = allocation;
}

Allocation::Allocation
(const Allocation *allocation)
{
   *this = allocation;
}

Allocation::~Allocation
(void)
{
   if (this->allocator == NULL)
      return;

   if (this->isBound())
      this->allocator->unbind(this);

   this->allocator = NULL;
}

void
Allocation::operator=
(Allocation &allocation)
{
   this->allocator = allocation.allocator;

   if (allocation.isValid())
      this->copy(allocation);
   else
      this->allocation = allocation.address;
}
   
void
Allocation::operator=
(const Allocation *allocation)
{
   this->allocator = allocation->allocator;
   
   if (allocation->isValid())
      this->clone(*allocation);
   else
      this->allocation = allocation->address;
}

Address
Allocation::operator*
(void)
{
   return this->address();
}

const Address
Allocation::operator*
(void) const
{
   return this->address();
}

bool
Allocation::isNull
(void) const
{
   return this->allocator == NULL || !this->allocator->isAssociated(*this) || this->size() == 0;
}

bool
Allocation::isBound
(void) const
{
   return !this->isNull() && this->allocator->isBound(*this);
}

bool
Allocation::isValid
(void) const
{
   return this->isBound() && this->allocator->isPooled(this->address());
}

bool
Allocation::allocatedFrom
(const Allocator *allocator) const
{
   return this->allocator == allocator;
}

bool
Allocation::inRange
(SIZE_T offset) const
{
   Address check;
   
   if (this->isNull() || this->size() == 0)
      return false;

   return this->inRange(this->start() + offset);
}

bool
Allocation::inRange
(SIZE_T offset, SIZE_T size) const
{
   /* we subtract 1 because we want to check whether or not all the content within
      this allocation is readable/writable, which means the inRange check will fail
      on the end of the range provided. */
      
   return size != 0 && this->inRange(offset) && this->inRange(offset+size-1);
}

bool
Allocation::inRange
(const Address &address) const
{
   return address >= this->start() && address < this->end();
}

bool
Allocation::inRange
(const Address &address, SIZE_T size) const
{
   return size != 0 && this->inRange(address) && this->inRange(address + size - 1);
}

void
Allocation::throwIfNoAllocator
(void) const
{
   if (this->allocator == NULL)
      throw NoAllocatorException(*this);
}

void
Allocation::throwIfInvalid
(void) const
{
   if (this->isValid())
      return;

   if (this->allocator == NULL)
      throw NoAllocatorException(*this);
   if (this->address.isNull())
      throw DeadAllocationException(*this);
   if (this->size == 0)
      throw ZeroSizeException(*this);

   this->allocator->throwIfNotPooled(this->allocation);
}

void
Allocation::throwIfNotInRange
(SIZE_T offset) const
{
   try
   {
      this->throwIfNotInRange(this->address(offset));
   }
   catch (AddressOutOfRangeException &exception)
   {
      throw OffsetOutOfRangeException(*this, offset, exception.size);
   }
}

void
Allocation::throwIfNotInRange
(SIZE_T offset, SIZE_T size) const
{
   try
   {
      this->throwIfNotInRange(this->address(offset), size);
   }
   catch (AddressOutOfRangeException &exception)
   {
      throw OffsetOutOfRangeException(*this, offset, exception.size);
   }
}

void
Allocation::throwIfNotInRange
(const Address &address) const
{
   if (!this->inRange(address))
      throw AddressOutOfRangeException(*this, address, 0);
}

void
Allocation::throwIfNotInRange
(const Address &address, SIZE_T size) const
{
   if (!this->inRange(address, size))
      throw AddressOutOfRangeException(*this, address, size);
}

Address
Allocation::address
(void)
{
   this->throwIfInvalid();
   
   return this->allocator.address(*this);
}

const Address
Allocation::address
(void) const
{
   this->throwIfInvalid();

   return this->allocator.address(*this);
}

Address
Allocation::address
(std::intptr_t offset)
{
   this->throwIfInvalid();
   
   return this->allocator.address(*this, offset)
}

const Address
Allocation::address
(std::intptr_t offset) const
{
   this->throwIfInvalid();

   return this->allocator.address(*this, offset);
}

Address
Allocation::newAddress
(void)
{
   this->throwIfInvalid();
   
   return this->allocator.newAddress(*this);
}

const Address
Allocation::newAddress
(void) const
{
   this->throwIfInvalid();

   return this->allocator.newAddress(*this);
}

Address
Allocation::newAddress
(std::intptr_t offset)
{
   this->throwIfInvalid();
   
   return this->allocator.newAddress(*this, offset)
}

const Address
Allocation::newAddress
(std::intptr_t offset) const
{
   this->throwIfInvalid();

   return this->allocator.newAddress(*this, offset);
}

Address
Allocation::start
(void)
{
   return this->address();
}

const Address
Allocation::start
(void) const
{
   return this->address();
}

Address
Allocation::end
(void)
{
   return this->address(this->getSize());
}

const Address
Allocation::end
(void) const
{
   return this->address(this->getSize());
}

const Address
Allocation::baseAddress
(void) const
{
   this->throwIfNotBound();
   
   return this->allocator->addressOf(*this);
}

SIZE_T
Allocation::offset
(const Address &address) const
{
   this->throwIfNotInRange(address);

   return address - this->address();
}

SIZE_T
Allocation::size
(void) const
{
   this->throwIfNoAllocator();
   
   return this->allocator.querySize(*this);
}

void
Allocation::allocate
(SIZE_T size)
{
   this->throwIfNoAllocator();
   
   if (this->isValid())
      throw DoubleAllocationException(*this);

   this->allocator->allocate(this, size);
}

void
Allocation::reallocate
(SIZE_T size)
{
   if (!this->isValid())
      return this->allocate(size);

   /* repooling should automatically rebind this object */
   this->allocator->reallocate(this, size);
}

void
Allocation::deallocate
(void)
{
   this->throwIfInvalid();
   this->allocator->throwIfNotBound(*this);
   this->allocator->unbind(this);
}

Data
Allocation::read
(void) const
{
   return this->read(this->size);
}

Data
Allocation::read
(SIZE_T size) const
{
   return this->read(static_cast<SIZE_T>(0), size);
}

Data
Allocation::read
(SIZE_T offset, SIZE_T size) const
{
   try
   {
      return this->read(this->address(offset), size);
   }
   catch (AddressOutOfRangeException &exception)
   {
      UNUSED(exception);
      throw OffsetOutOfRangeException(*this, offset, size);
   }
}

Data
Allocation::read
(const Address &address, SIZE_T size) const
{
   this->throwIfNotValid();
   this->throwIfNotInRange(address);
   
   if (this->allocator->willSplit(address, size))
      return this->allocator->splitRead(address, size);
   else
      return this->allocator->read(this, address, size);
}

void
Allocation::write
(const Data data)
{
   this->write(static_cast<SIZE_T>(0), data);
}

void
Allocation::write
(SIZE_T offset, const Data data)
{
   try
   {
      this->write(this->address(offset), data);
   }
   catch (AddressOutOfRangeException &exception)
   {
      UNUSED(exception);
      throw OffsetOutOfRangeException(*this, offset, size);
   }
}

void
Allocation::write
(const Address &destAddress, const Data data)
{
   this->throwIfInvalid();
   this->throwIfNotInRange(destAddress);
   
   if (this->allocator->willSplit(destAddress, data.size()))
      this->allocator->splitWrite(destAddress, data);
   else
      this->allocator->write(this, destAddress, data);
}

void
Allocation::copy
(Allocation &allocation)
{
   allocation.throwIfInvalid();

   this->allocator = allocation.allocator;

   if (this->isBound())
      this->allocator->rebind(this, allocation.baseAddress());
   else
      this->allocator->bind(this, allocation.baseAddress());
}

void
Allocation::clone
(const Allocation &allocation)
{
   allocation.throwIfInvalid();

   if (this->size() != allocation.size())
      this->reallocate(allocation.size());

   this->write(allocation.read());
}

Allocator::Exception::Exception
(Allocator &allocator, const LPWSTR message)
   : Neurology::Exception(message)
   , allocator(allocator)
{
}

Allocator::ZeroSizeException::ZeroSizeException
(Allocator &allocator)
   : Allocator::Exception(allocator, EXCSTR(L"Cannot allocate or reallocate to a zero-sized object."))
{
}

Allocator::PoolAllocationException::PoolAllocationException
(Allocator &allocator)
   : Allocator::Exception(allocator, EXCSTR(L"Pooling of new address failed."))
{
}

Allocator::UnpooledAddressException::UnpooledAddressException
(Allocator &allocator, const LPVOID address)
   : Allocator::Exception(allocator, EXCSTR(L"The supplied address is not pooled by this allocator."))
   , address(address)
{
}

Allocator::BindingException::BindingException
(Allocator &allocator, Allocation &allocation, const LPWSTR message)
   : Allocator::Exception(allocator, message)
   , allocation(allocation)
{
}

Allocator::BoundAllocationException::BoundAllocationException
(Allocator &allocator, Allocation &allocation)
   : BindingException(allocator, allocation, EXCSTR(L"The allocation is already bound."))
{
}

Allocator::UnboundAllocationException::UnboundAllocationException
(Allocator &allocator, Allocation &allocation)
   : BindingException(allocator, allocation, EXCSTR(L"The allocation is not bound to the allocator."))
{
}

Allocator::UnmanagedAllocationException::UnmanagedAllocationException
(Allocator &allocator, Allocation &allocation)
   : Allocator::Exception(allocator, EXCSTR(L"The provided allocation is not being managed by the allocator."))
   , allocation(allocation)
{
}

Allocator::InsufficientSizeException::InsufficientSizeException
(Allocator &allocator, const SIZE_T size)
   : Allocator::Exception(allocator, EXCSTR(L"Supplied size not large enough for supplied type."))
   , size(size)
{
}

Allocator::Allocator
(void)
   : split(true)
{
}

Allocator::Allocator
(bool split)
   : split(split)
{
}

Allocator::~Allocator
(void)
{
   /* unbind all allocation bindings */
   BindingMap::iterator iter = this->bindings.begin();

   while (iter != this->bindings.end())
   {
      if (iter->second.begin() != iter->second.end())
         this->unbind(iter->second.begin());
      else
      {
         /* the binding set is empty, check if it's still pooled.
            if so, erase it. */
         if (this->isPooled(iter->first))
            this->unpool(iter->first);
         
         this->bindings.erase(iter->first);
      }

      /* unbinding may have misplaced the iterator-- start over from the beginning */
      iter = this->bindings.begin();
   }
   
   /* if there are any entries left in the memory pool, delete them */
   for (MemoryPool::iterator iter=this->pooledMemory.begin();
        iter != this->pooledMemory.end();
        ++iter)
   {
      this->unpool(iter->first);
   }
}

void
Allocator::allowSplitting
(void)
{
   this->split = true;
}

void
Allocator::denySplitting
(void)
{
   this->split = false;
}

bool
Allocator::splits
(void) const
{
   return this->split;
}

bool
Allocator::isPooled
(const Address &address) const
{
   return this->pooledMemory.count(const_cast<Address>(address)) > 0;
}

bool
Allocator::isAssociated
(const Allocation &allocation) const
{
   return this->associations.count(const_cast<Allocation *>(&allocation)) > 0;
}

bool
Allocator::isBound
(const Allocation &allocation) const
{
   Address address;
   std::map<LPVOID, std::set<Allocation *> >::const_iterator bindIter;
   std::set<Allocation *>::const_iterator refIter;
   const Allocation *ptr = &allocation;

   address = allocation.address();

   if (this->bindings.count(address) == 0)
      return false;

   bindIter = this->bindings.find(address);
   refIter = bindIter->second.find(const_cast<Allocation *>(&allocation));

   return refIter != bindIter->second.end();
}

bool
Allocator::hasAddress
(const Address &address) const
{
   return !this->find(address).isNull();
}

bool
Allocator::willSplit
(const Address &address, SIZE_T size) const;
{
   Allocation &foundAllocation = this->find(address);
   MemoryPool::iterator poolIter;
   Address endAddr;

   /* technically false-- you won't split an allocation that doesn't exist. */
   if (foundAllocation.isNull())
      return false;

   /* address and size are within range of the allocation, won't split */
   if (foundAllocation.inRange(address, size))
      return false;

   /* now, find the key tied to this allocation and see if the next allocation
      has the same start address as the prior allocation's end address. if it
      does, it will split, we just don't know how many times. */
   endAddr = foundAllocation.end();
   poolIter = this->pooledMemory.upper_bound(address);

   /* return the determination of whether or not this has one or more splits */
   return poolIter != this->pooledMemory.end() && endAddr == poolIter->first;
}
void
Allocator::throwIfNotPooled
(const Address &address) const
{
   if (!this->isPooled(address))
      throw UnpooledAddressException(const_cast<Allocator &>(*this), address);
}

void
Allocator::throwIfNotAllocated
(const Allocation &allocation) const
{
   if (!this->isAllocated(allocation))
      throw UnmanagedAllocationException(const_cast<Allocator &>(*this), const_cast<Allocation &>(allocation));
}

void
Allocator::throwIfBound
(const Allocation &allocation) const
{
   if (this->isBound(allocation))
      throw BoundAllocationException(const_cast<Allocator &>(*this), const_cast<Allocation &>(allocation));
}

void
Allocator::throwIfNotBound
(const Allocation &allocation) const
{
   if (!this->isBound(allocation))
      throw UnboundAllocationException(const_cast<Allocator &>(*this), const_cast<Allocation &>(allocation));
}

void
Allocator::throwIfNoAllocation
(const Address &address) const
{
   if (!this->hasAddress(address))
      throw AddressHasNoAllocationException(const_cast<Allocator>(*this), const_cast<Address>(address));
}

const Address
Allocator::addressOf
(const Allocation &allocation)
{
   this->throwIfNotBound(allocation);

   return const_cast<const Address>(this->associations[&allocation]);
}

Address
Allocator::address
(const Allocation &allocation)
{
   return this->address(allocation, 0);
}

const Address
Allocator::address
(const Allocation &allocation) const
{
   return this->address(allocation, 0);
}

Address
Allocator::address
(const Allocation &allocation, SIZE_T offset)
{
   Address baseAddress;
   
   this->throwIfNotBound(allocation);
   baseAddress = this->associations[const_cast<Allocation *>(&allocation)];
   return this->addressPools[baseAddress].address(baseAddress.label() + offset);
}

const Address
Allocator::address
(const Allocation &allocation, SIZE_T offset) const
{
   Address baseAddress;
   
   this->throwIfNotBound(allocation);
   baseAddress = this->associations[const_cast<Allocation *>(&allocation)];
   return this->addressPools[baseAddress].address(baseAddress.label() + offset);
}

Address
Allocator::newAddress
(const Allocation &allocation)
{
   return this->newAddress(allocation, 0);
}

const Address
Allocator::newAddress
(const Allocation &allocation) const
{
   return this->newAddress(allocation, 0);
}

Address
Allocator::newAddress
(const Allocation &allocation, SIZE_T offset)
{
   Address baseAddress;
   
   this->throwIfNotBound(allocation);
   baseAddress = this->associations[const_cast<Allocation *>(&allocation)];
   return this->addressPools[baseAddress].newAddress(baseAddress.label() + offset);
}

const Address
Allocator::newAddress
(const Allocation &allocation, SIZE_T offset) const
{
   Address baseAddress;
   
   this->throwIfNotBound(allocation);
   baseAddress = this->associations[const_cast<Allocation *>(&allocation)];
   return this->addressPools[baseAddress].newAddress(baseAddress.label() + offset);
}

SIZE_T
Allocator::bindCount
(const Address &address) const
{
   std::map<const LPVOID, std::set<Allocation *> >::const_iterator bindIter;

   bindIter = this->bindings.find(const_cast<Address>(address));

   if (bindIter == this->bindings.end())
      return 0;

   return bindIter->second.size();
}

SIZE_T
Allocator::querySize
(const Allocation &allocation) const
{
   Address baseAddress;
   
   this->throwIfNotBound(allocation);
   baseAddress = this->associations[const_cast<Allocation *>(&allocation)];
   return this->addressPools[baseAddress].size();
}

LPVOID
Allocator::pool
(SIZE_T size)
{
   throw VoidAllocatorException(*this);
}

LPVOID
Allocator::repool
(Address &address, SIZE_T size)
{
   throw VoidAllocatorException(*this);
}

void
Allocator::unpool
(Address &address)
{
   throw VoidAllocatorException(*this);
}

Allocation
Allocator::find
(const Address address)
{
   BindingMap::iterator bindIter;
   
   if (this->bindings.count(address) > 0)
      return **this->bindings[address].begin();

   /* we didn't find a binding bound to that specific address. try another method. */
   bindIter = this->bindings.upper_bound(address);

   if (bindIter == this->bindings.begin())
      return this->null();

   --bindIter;

   if (bindIter->second.begin()->inRange(address))
      return *bindIter->second.begin();

   /* couldn't find it in our allocations, return a null allocation. */
   return this->null();
}

Allocation
Allocator::null
(void)
{
   return Allocation(this);
}

Allocation
Allocator::allocate
(SIZE_T size)
{
   Allocation newAllocation = this->null();
   newAllocation.allocate(size);
   return newAllocation;
}

void
Allocator::reallocate
(Allocation &allocation, SIZE_T size)
{
   this->throwIfNotAllocated(allocation);
   this->throwIfNotPooled(allocation.baseAddress());

   /* calling repool on the underlying address will rebind all allocations automatically. */
   this->repool(allocation.baseAddress(), size);
}

void
Allocator::deallocate
(Allocation &allocation)
{
   this->throwIfNotAllocated(allocation);
   this->throwIfNotPooled(allocation.baseAddress());

   this->unpool(allocation.baseAddress());
}

Data
Allocator::read
(const Address &address, SIZE_T size) const
{
   this->throwIfNoAllocation(address);

   if (this->willSplit(address, size))
      return this->splitRead(address, size);
   else
      return this->read(this->find(address), address, size);
}

void
Allocator::write
(Address &address, const Data data)
{
   this->throwIfNoAllocation(address);

   if (this->willSplit(address, size))
      return this->splitWrite(address, data);
   else
      return this->read(this->find(address), address, data);
}

void
Allocator::bind
(Allocation *allocation, Address address)
{
   this->throwIfNotPooled(address);
   this->throwIfBound(*allocation);

   if (this->bindings.count(address) == 0)
      this->bindings[address] = std::set<Allocation *>();

   this->bindings[address].insert(allocation);
   allocation->allocator = this;

   if (this->addressPools.count(address) == 0)
      this->addressPools[address] = AddressPool(address.label(), (address+this->pooledMemory[address]).label());

   this->associations[allocation] = address;
}

void
Allocator::rebind
(Allocation *allocation, Address &newAddress)
{
   Address oldAddress;
   
   if (!this->isBound(*allocation))
      return this->bind(allocation, newAddress);

   this->throwIfNotAssociated(*allocation);
   this->throwIfNotPooled(allocation->address());
   this->throwIfNotPooled(newAddress);

   if (this->bindings.count(newAddress) == 0)
      this->bindings[newAddress] = std::set<Allocation *>();
   
   oldAddress = this->associations[allocation];
   this->bindings[newAddress].insert(allocation);
   this->bindings[oldAddress].erase(allocation);

   allocation->allocator = this;
   this->associations[allocation] = newAddress;

   if (this->bindings[oldAddress].size() == 0)
   {
      this->bindings.erase(oldAddress);
      this->unpool(oldAddress);
   }
   else
      /* moves all identifiers in the address's binding to the new address */
      oldAddress.moveIdentifier(newAddress.label());
}

void
Allocator::unbind
(Allocation *allocation)
{
   std::map<LPVOID, std::set<Allocation *> >::iterator bindIter;
   LPVOID boundAddress;
   SIZE_T bindings;
   
   this->throwIfNotBound(*allocation);
   this->throwIfNotPooled(allocation->pointer);

   boundAddress = allocation->pointer;

   this->addressPools.erase(boundAddress);
   this->associations.erase(allocation);
   this->bindings[boundAddress].erase(allocation);
   
   if (this->bindings[boundAddress].size() == 0)
   {
      this->bindings.erase(boundAddress);
      
      if (this->isPooled(boundAddress))
          this->unpool(boundAddress);
   }
}

void
Allocator::allocate
(Allocation *allocation, SIZE_T size)
{
   throw VoidAllocatorException(*this);
}

void
Allocator::reallocate
(Allocation *allocation, SIZE_T size)
{
   throw VoidAllocatorException(*this);
}

void
Allocator::deallocate
(Allocation *allocation)
{
   throw VoidAllocatorException(*this);
}

Data
Allocator::splitRead
(const Address &startAddress, SIZE_T size)
{
   BindingMap::iterator bindIter;
   Data result;
   Address addressIter;
   SIZE_T oldSize = size;
   
   this->throwIfNoAllocation(startAddress);

   /* won't split, do a normal read */
   if (!this->willSplit(startAddress, size))
      this->read(startAddress, size);

   /* find the lower bound of the address and wind it back one if it's not exact. */
   bindIter = this->bindings.lower_bound(startAddress);

   if (bindIter == this->bindings.end() || bindIter->first != startAddress)
      --bindIter;

   addressIter = startAddress;

   while (bindIter != this->bindings.end())
   {
      Allocation *boundAlloc = bindIter->second.begin();
      Allocation *nextAlloc;
      Data stackData;
      bool inRange;

      inRange = boundAlloc->inRange(addressIter, size);

      if (inRange)
      {
         stackData = this->read(boundAlloc, addressIter, size);
         size = 0;
      }
      else
      {
         SIZE_T newSize;

         newSize = boundAlloc->end() - addressIter;
         stackData = this->read(boundAlloc, addressIter, newSize);
         size -= newSize;
      }

      result.resize(result.size() + stackData.size());
      MoveMemory(result.data() + result.size(), stackData.data(), stackData.size());

      if (size == 0)
         break;

      ++bindIter;

      if (bindIter == this->bindings.end())
         continue;

      nextAlloc = bindIter->second.begin();

      /* we've reached a splitting boundary, break out */
      if (boundAlloc->end() != nextAlloc->begin())
         break;

      addressIter = bindIter->first;
   }

   if (size != 0)
      throw SplitsExceededException(*this, startAddress, oldSize);

   return result;
}

Data
Allocator::splitWrite
(const Address &destination, const Data data)
{
   BindingMap::iterator bindIter;
   Address addressIter;
   SIZE_T size = data.size();
   SIZE_T windowLeft, windowRight;
   
   this->throwIfNoAllocation(destination);

   /* won't split, do a normal read */
   if (!this->willSplit(destination, size))
      this->write(destination, data);

   /* find the lower bound of the address and wind it back one if it's not exact. */
   bindIter = this->bindings.lower_bound(destination);

   if (bindIter == this->bindings.end() || bindIter->first != destination)
      --bindIter;

   addressIter = destination;
   windowLeft = windowRight = 0;

   while (bindIter != this->bindings.end())
   {
      Allocation *boundAlloc = bindIter->second.begin();
      Allocation *nextAlloc;
      Data dataSlice;
      bool inRange;

      inRange = boundAlloc->inRange(addressIter, size);

      if (inRange)
         windowRight = size;
      else
         windowRight += boundAlloc->end() - addressIter;
      
      dataSlice = BlockData(data.data()+windowLeft, windowRight - windowLeft);
      this->write(boundAlloc, addressIter, dataSlice);
      windowLeft = windowRight;

      if (windowLeft == data.size())
         break;

      ++bindIter;

      if (bindIter == this->bindings.end())
         continue;

      nextAlloc = bindIter->second.begin();

      /* we've reached a splitting boundary, break out */
      if (boundAlloc->end() != nextAlloc->begin())
         break;

      addressIter = bindIter->first;
   }

   if (windowLeft != data.size())
      throw SplitsExceededException(*this, startAddress, size);

   return result;
}

Data
Allocator::read
(const Allocation *allocation, const Address &address, SIZE_T size) const
{
   allocation->throwIfNotInRange(address, size);
   return this->readAddress(address, size);
}

Data
Allocator::write
(const Allocation *allocation, const Address &destination, const Data data) const;
{
   allocation->throwIfNotInRange(destination, data.size());
   return this->writeAddress(destination, data);
}

Data
Allocator::readAddress
(const Address &address, SIZE_T size) const
{
   throw VoidAllocatorException(*this);
}

Data
Allocator::writeAddress
(const Address &destination, const Data data) const
{
   throw VoidAllocatorException(*this);
}
