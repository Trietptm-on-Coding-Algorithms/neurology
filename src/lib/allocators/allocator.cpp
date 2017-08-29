#include <neurology/allocators/allocator.hpp>

using namespace Neurology;

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
(const Allocation &allocation, const LPVOID address, const SIZE_T size)
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

Allocation::BadPointerException::BadPointerException
(const Allocation &allocation, const LPVOID pointer, const SIZE_T size)
   : Allocation::Exception(allocation, EXCSTR(L"The provided pointer threw a hardware exception."))
   , pointer(pointer)
   , size(size)
{
}

Allocation::Allocation
(Allocator *allocator, LPVOID pointer, SIZE_T size)
{
   if (allocator == NULL)
      throw NoAllocatorException(*this);

   allocator->throwIfNotPooled(pointer);

   this->allocator = allocator;
   this->pointer = pointer;
   this->size = size;
}

Allocation::Allocation
(void)
   : allocator(NULL)
   , pointer(NULL)
   , size(0)
{
}

Allocation::Allocation
(Allocator *allocator)
   : pointer(NULL)
   , size(0)
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
(const Allocation &allocation)
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
}

void
Allocation::operator=
(Allocation &allocation)
{
   this->allocator = allocation.allocator;

   if (allocation.isValid())
   {
      if (this->isBound())
         this->allocator->rebind(this, allocation.pointer);
      else
         this->allocator->bind(this, allocation.pointer);
   }

   this->pointer = allocation.pointer;
   this->size = allocation.size;
}
   
void
Allocation::operator=
(const Allocation &allocation)
{
   this->allocator = allocation.allocator;
   
   if (allocation.isValid())
   {
      LPVOID newPointer;

      newPointer = this->allocator->pool(allocation.size);

      if (this->isBound())
         this->allocator->rebind(this, newPointer);
      else
         this->allocator->bind(this, newPointer);
      
      this->pointer = newPointer;
      this->size = allocation.size;
      
      CopyMemory(this->pointer, allocation.pointer, allocation.size);
   }
   else
   {
      this->pointer = allocation.pointer;
      this->size = allocation.size;
   }
}

LPVOID
Allocation::operator*
(void)
{
   return this->address();
}

const LPVOID
Allocation::operator*
(void) const
{
   return this->address();
}

bool
Allocation::isValid
(void) const
{
   return this->allocator != NULL && this->pointer != NULL && this->size != 0 && this->allocator->isPooled(this->pointer);
}

bool
Allocation::isBound
(void) const
{
   return this->isValid() && this->allocator->isBound(*this);
}

bool
Allocation::isNull
(void) const
{
   return this->allocator == NULL || this->pointer == NULL || this->size == 0;
}

bool
Allocation::inRange
(const LPVOID address) const
{
   return this->pointer != NULL && this->size != 0 && address >= this->start() && address <= this->end();
}

bool
Allocation::inRange
(const LPVOID address, SIZE_T size) const
{
   return this->inRange(address) && this->inRange(static_cast<LPBYTE>(address)+size);
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
   if (this->pointer == NULL)
      throw DeadAllocationException(*this);
   if (this->size == 0)
      throw ZeroSizeException(*this);

   this->allocator->throwIfNotPooled(this->pointer);
}

LPVOID
Allocation::address
(void)
{
   return this->pointer;
}

const LPVOID
Allocation::address
(void) const
{
   return const_cast<const LPVOID>(this->pointer);
}

LPVOID
Allocation::address
(SIZE_T offset)
{
   this->throwIfInvalid();

   if (offset > this->size)
      throw OffsetOutOfRangeException(*this, offset, 0);
   
   return static_cast<LPBYTE>(this->pointer)+offset;
}

const LPVOID
Allocation::address
(SIZE_T offset) const
{
   this->throwIfInvalid();

   if (offset > this->size)
      throw OffsetOutOfRangeException(*this, offset, 0);

   return const_cast<const LPVOID>(static_cast<LPVOID>(static_cast<LPBYTE>(this->pointer)+offset));
}

LPVOID
Allocation::start
(void)
{
   return this->address();
}

const LPVOID
Allocation::start
(void) const
{
   return this->address();
}

LPVOID
Allocation::end
(void)
{
   return this->address(this->size);
}

const LPVOID
Allocation::end
(void) const
{
   return this->address(this->size);
}

SIZE_T
Allocation::getSize
(void) const
{
   return this->size;
}

void
Allocation::allocate
(SIZE_T size)
{
   this->throwIfNoAllocator();
   
   if (this->pointer != NULL && this->allocator->isPooled(this->pointer))
      throw DoubleAllocationException(*this);

   this->pointer = this->allocator->pool(size);
   this->size = size;
   this->allocator->bind(this, this->pointer);
}

void
Allocation::reallocate
(SIZE_T size)
{
   if (!this->isValid())
      return this->allocate(size);

   /* repooling should automatically rebind this object */
   this->pointer = this->allocator->repool(this->pointer, size);
   this->size = size;
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
      return this->read(static_cast<LPBYTE>(this->pointer)+offset, size);
   }
   catch (AddressOutOfRangeException &exception)
   {
      throw OffsetOutOfRangeException(*this, offset, size);
   }
}

Data
Allocation::read
(LPVOID address, SIZE_T size) const
{
   this->throwIfInvalid();
   
   if (!this->inRange(address, size))
      throw AddressOutOfRangeException(*this, address, size);

   __try
   {
      return BlockData(address, size);
   }
   __except (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
   {
      throw BadPointerException(*this, address, size);
   }
}

void
Allocation::write
(const Data data)
{
   this->write(0, data);
}

void
Allocation::write
(SIZE_T offset, const Data data)
{
   this->write(this->address(offset), const_cast<const LPVOID>(reinterpret_cast<LPVOID>(const_cast<LPBYTE>(data.data()))), data.size());
}

void
Allocation::write
(const LPVOID pointer, SIZE_T size)
{
   this->write(static_cast<SIZE_T>(0), pointer, size);
}

void
Allocation::write
(SIZE_T offset, const LPVOID pointer, SIZE_T size)
{
   try
   {
      this->write(this-address(offset), pointer, size);
   }
   catch (AddressOutOfRangeException &exception)
   {
      throw OffsetOutOfRangeException(*this, offset, size);
   }
}

void
Allocation::write
(LPVOID address, const LPVOID pointer, SIZE_T size)
{
   this->throwIfInvalid();

   if (!this->inRange(address, size))
      throw AddressOutOfRangeException(*this, address, size);

   __try
   {
      CopyMemory(address, pointer, size);
   }
   __except (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
   {
      throw BadPointerException(*this, address, size);
   }
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

Allocator::Allocator
(void)
{
}

Allocator::~Allocator
(void)
{
   /* unbind all allocation bindings */
   for (std::map<LPVOID, std::set<Allocation *> >::iterator iter=this->bindings.begin();
        iter != this->bindings.end();
        ++iter)
   {
      for (std::set<Allocation *>::iterator setIter=this->bindings[iter->first].begin();
           setIter != this->bindings[iter->first].end();
           ++setIter)
      {
         this->unbind(*setIter);
      }
   }
             
   /* if there are any allocations left, delete them */
   for (std::set<Allocation *>::iterator iter=this->allocations.begin();
        iter != this->allocations.end();
        ++iter)
   {
      this->deallocate(**iter);
   }
   
   /* if there are any entries left in the memory pool, delete them */
   for (std::map<LPVOID, SIZE_T>::iterator iter=this->memoryPool.begin();
        iter != this->memoryPool.end();
        ++iter)
   {
      this->unpool(iter->first);
   }
}

bool
Allocator::isBound
(const Allocation &allocation) const
{
   LPVOID pointer;
   std::map<LPVOID, std::set<Allocation *> >::const_iterator bindIter;
   std::set<Allocation *>::const_iterator refIter;
   const Allocation *ptr = &allocation;

   pointer = allocation.address();
   bindIter = this->bindings.find(pointer);

   if (bindIter == this->bindings.end())
      return false;

   refIter = bindIter->second.find(const_cast<Allocation *>(&allocation));

   return refIter != bindIter->second.end();
}

bool
Allocator::isPooled
(const LPVOID pointer) const
{
   return this->memoryPool.find(pointer) != this->memoryPool.end();
}

bool
Allocator::isAllocated
(const Allocation &allocation) const
{
   return this->allocations.find(const_cast<Allocation *>(&allocation)) != this->allocations.end();
}

void
Allocator::throwIfNotPooled
(const LPVOID pointer) const
{
   if (!this->isPooled(pointer))
      throw UnpooledAddressException(const_cast<Allocator &>(*this), pointer);
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

LPVOID
Allocator::pool
(SIZE_T size)
{
   LPVOID pooledData;

   if (size == 0)
      throw ZeroSizeException(*this);
   
   pooledData = static_cast<LPVOID>(new BYTE[size]);

   if (pooledData == NULL)
      throw PoolAllocationException(*this);
   
   ZeroMemory(pooledData, size);
   
   this->memoryPool[pooledData] = size;

   return pooledData;
}

LPVOID
Allocator::repool
(LPVOID address, SIZE_T size)
{
   LPVOID newPool;
   std::map<LPVOID, std::set<Allocation *> >::iterator bindIter;
   
   this->throwIfNotPooled(address);

   newPool = this->pool(size);
   CopyMemory(newPool, address, min(size, this->memoryPool[address]));

   bindIter = this->bindings.find(address);

   if (bindIter != this->bindings.end())
   {
      std::set<Allocation *>::iterator setIter;

      for (setIter = this->bindings[address].begin(); setIter != this->bindings[address].end(); ++setIter)
         this->rebind(*setIter, newPool);
   }

   /* if the address is still pooled after having all its bindings moved, destroy it-- it is no longer relevant. */
   if (this->isPooled(address))
      this->unpool(address);

   return newPool;
}

void
Allocator::unpool
(LPVOID address)
{
   std::map<LPVOID, std::set<Allocation *> >::iterator bindIter;
   
   this->throwIfNotPooled(address);

   bindIter = this->bindings.find(address);

   if (bindIter != this->bindings.end())
   {
      std::set<Allocation *>::iterator setIter;

      for (setIter = this->bindings[address].begin(); setIter != this->bindings[address].end(); ++setIter)
         this->unbind(*setIter);

      /* did unbind destroy us? return if so */
      if (!this->isPooled(address))
         return;
   }

   delete[] address;
   this->memoryPool.erase(address);
}

Allocation &
Allocator::find
(const LPVOID address)
{
   if (this->bindings.find(address) == this->bindings.end())
      return this->null();

   return **this->bindings[address].begin();
}

Allocation &
Allocator::null
(void)
{
   return Allocation(this);
}

Allocation &
Allocator::allocate
(SIZE_T size)
{
   LPVOID newPool;
   Allocation *newAllocation;

   newAllocation = new Allocation(this, this->pool(size), size);
   this->allocations.insert(newAllocation);
   this->bind(newAllocation, newPool);

   return *newAllocation;
}

void
Allocator::reallocate
(Allocation &allocation, SIZE_T size)
{
   this->throwIfNotAllocated(allocation);
   this->throwIfNotPooled(allocation.pointer);

   /* calling repool on the underlying address will rebind this allocation automatically. */
   this->repool(allocation.pointer, size);
}

void
Allocator::deallocate
(Allocation &allocation)
{
   this->throwIfNotAllocated(allocation);
   this->throwIfNotPooled(allocation.pointer);

   this->unpool(allocation.pointer);
}

void
Allocator::bind
(Allocation *allocation, LPVOID address)
{
   this->throwIfNotPooled(address);
   this->throwIfBound(*allocation);

   if (this->bindings.find(address) == this->bindings.end())
      this->bindings[address] = std::set<Allocation *>();

   this->bindings[address].insert(allocation);
   allocation->allocator = this;
   allocation->pointer = address;
   allocation->size = this->memoryPool[address];
}

void
Allocator::rebind
(Allocation *allocation, LPVOID newAddress)
{
   LPVOID oldAddress;
   
   if (!this->isBound(*allocation))
      return this->bind(allocation, newAddress);

   this->throwIfNotPooled(allocation->pointer);
   this->throwIfNotPooled(newAddress);

   if (this->bindings.find(newAddress) == this->bindings.end())
      this->bindings[newAddress] = std::set<Allocation *>();
   
   oldAddress = allocation->pointer;
   this->bindings[newAddress].insert(allocation);
   this->bindings[oldAddress].erase(allocation);

   allocation->allocator = this;
   allocation->pointer = newAddress;
   allocation->size = this->memoryPool[newAddress];

   if (this->bindings[oldAddress].size() == 0)
      this->unpool(oldAddress);
}

void
Allocator::unbind
(Allocation *allocation)
{
   std::map<LPVOID, std::set<Allocation *> >::iterator bindIter;
   LPVOID boundAddress;
   
   this->throwIfNotBound(*allocation);
   this->throwIfNotPooled(allocation->pointer);

   boundAddress = allocation->pointer;
   
   this->bindings[boundAddress].erase(allocation);

   if (this->bindings[boundAddress].size() == 0)
      this->bindings.erase(boundAddress);

   /* if we're unbinding the allocation and we created the allocation object, this is where we want to
      delete it. */
   if (this->allocations.find(allocation) != this->allocations.end())
   {
      this->allocations.erase(allocation);
      delete allocation;
   }

   if (this->isPooled(boundAddress))
      this->unpool(boundAddress);
}
