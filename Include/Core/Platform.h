#pragma once

#include <functional>

#include "Core.h"

#ifdef _WIN32
	#define THREAD_LOCAL thread_local
	#define DLL_EXPORT __declspec(dllexport)
	#define DLL_IMPORT __declspec(dllimport)
	#define FORCEINLINE __forceinline
	#define UNUSED
	#include <intrin.h>
#else
	// Use __thread instead of the C++11 thread_local because Apple's clang doesn't support thread_local yet.
	#define THREAD_LOCAL __thread
	#define DLL_EXPORT
	#define DLL_IMPORT
	#define FORCEINLINE inline __attribute__((always_inline))
	#define UNUSED __attribute__((unused))
#endif

namespace Platform
{
	#ifdef _WIN32
		inline uint64 floorLogTwo(uint64 value) { unsigned long result; return _BitScanReverse64(&result,value) ? result : 0; }
		inline uint32 floorLogTwo(uint32 value) { unsigned long result; return _BitScanReverse(&result,value) ? result : 0; }
	#else
		inline uint64 floorLogTwo(uint64 value) { return 63 - __builtin_clzll(value); }
		inline uint32 floorLogTwo(uint32 value) { return 31 - __builtin_clz(value); }
	#endif
	inline uint64 ceilLogTwo(uint64 value) { return floorLogTwo(value * 2 - 1); }
	inline uint32 ceilLogTwo(uint32 value) { return floorLogTwo(value * 2 - 1); }

	// A platform-independent mutex. Allows calling the constructor during static initialization, unlike std::mutex.
	struct Mutex
	{
		CORE_API Mutex();
		CORE_API ~Mutex();

		CORE_API void Lock();
		CORE_API void Unlock();

	private:
		void* handle;
	};

	// RAII-style lock for Mutex.
	struct Lock
	{
		Lock(Mutex& inMutex) : mutex(&inMutex) { mutex->Lock(); }
		~Lock() { Release(); }

		void Release()
		{
			if(mutex)
			{
				mutex->Unlock();
			}
			mutex = NULL;
		}

	private:
		Mutex* mutex;
	};

	// Describes allowed memory accesses.
	enum class MemoryAccess
	{
		None,
		ReadOnly,
		ReadWrite,
		Execute,
		ReadWriteExecute
	};

	// Returns the base 2 logarithm of the smallest virtual page size.
	CORE_API uintp getPageSizeLog2();

	// Allocates virtual addresses without commiting physical pages to them.
	// Returns the base virtual address of the allocated addresses, or nullptr if the virtual address space has been exhausted.
	CORE_API uint8* allocateVirtualPages(size_t numPages);

	// Commits physical memory to the specified virtual pages.
	// baseVirtualAddress must be a multiple of the preferred page size.
	// Return true if successful, or false if physical memory has been exhausted.
	CORE_API bool commitVirtualPages(uint8* baseVirtualAddress,size_t numPages,MemoryAccess access = MemoryAccess::ReadWrite);

	// Changes the allowed access to the specified virtual pages.
	// baseVirtualAddress must be a multiple of the preferred page size.
	// Return true if successful, or false if the access-level could not be set.
	CORE_API bool setVirtualPageAccess(uint8* baseVirtualAddress,size_t numPages,MemoryAccess access);

	// Decommits the physical memory that was committed to the specified virtual pages.
	// baseVirtualAddress must be a multiple of the preferred page size.
	CORE_API void decommitVirtualPages(uint8* baseVirtualAddress,size_t numPages);

	// Frees virtual addresses. Any physical memory committed to the addresses must have already been decommitted.
	// baseVirtualAddress must be a multiple of the preferred page size.
	CORE_API void freeVirtualPages(uint8* baseVirtualAddress,size_t numPages);

	// Describes an instruction pointer.
	CORE_API bool describeInstructionPointer(uintp ip,std::string& outDescription);

	// Describes a call stack.
	struct CallStack
	{
		struct Frame
		{
			uintp ip;
		};
		std::vector<Frame> stackFrames;
	};

	// Captures the execution context of the caller.
	CORE_API CallStack captureCallStack(uintp numOmittedFramesFromTop = 0);

	#ifdef _WIN32
		// Registers/deregisters the data used by Windows SEH to unwind stack frames.
		CORE_API void* registerSEHUnwindInfo(uintp imageLoadAddress,uintp textLoadAddress,uintp xdataLoadAddress,uintp pdataLoadAddress,size_t pdataNumBytes);
		CORE_API void deregisterSEHUnwindInfo(void* registerResult);
	#endif

	// Initializes thread-specific state.
	CORE_API void initThread();

	// Calls a thunk, and if it causes any of some specific hardware traps, returns true.
	// If a trap was caught, the outCause, outContext, and outOperand parameters are set to describe the trap.
	enum HardwareTrapType
	{
		none,
		accessViolation,
		stackOverflow,
		intDivideByZeroOrOverflow
	};
	CORE_API HardwareTrapType catchHardwareTraps(
		CallStack& outTrapCallStack,
		uintp& outTrapOperand,
		const std::function<void()>& thunk
		);
}
