#pragma once

#include "clrUtils.h"



// https://github.com/DarthTon/Blackbone
#include <BlackBone/Include/NativeStructures.h>


// https://github.com/HoShiMin/Kernel-Bridge
#ifndef _WIN64
#pragma comment(lib, "../third_party/Kernel-Bridge-1.19.3/i386/Release/User-Bridge.lib")
#else
#pragma comment(lib, "../third_party/Kernel-Bridge-1.19.3/x64/Release/User-Bridge.lib")
#endif
#include <SharedTypes/WdkTypes.h>
#include <SharedTypes/CtlTypes.h>
#include <User-Bridge/API/User-Bridge.h>




namespace x86injecttool {


	typedef uint64_t ptr_t;


	struct clsProcessWrapper {

		~clsProcessWrapper() {

		};

		clsProcessWrapper(ptr_t processId, bool is64)
			: _process_id(processId), _is64(is64) {

		};


		inline const ptr_t process_id() { return _process_id; };

		inline const bool is64() { return _is64; };

		inline const size_t ptrlength() { return is64() ? 8 : 4; }

		ptr_t read_pointer(ptr_t addr) {
			return 8 == ptrlength() ? read_memory_t<uint64_t>(addr) : read_memory_t<uint32_t>(addr);
		}

		template<typename T>
		T read_memory_t(ptr_t addr) {
			T buffer = { };
			return read_memory(addr, ptr_t(&buffer), sizeof(T)) ? buffer : T();
		};

		uint32_t wait_for_thread(ptr_t thread_id, uint32_t u32ms = INFINITE) noexcept {
			HANDLE hThread = ::OpenThread(SYNCHRONIZE | THREAD_QUERY_LIMITED_INFORMATION, FALSE, DWORD(thread_id));
			if (hThread) {
				BOOL isOk = FALSE; DWORD dwExitCode = 0;
				::WaitForSingleObject(hThread, DWORD(u32ms));
				isOk = ::GetExitCodeThread(hThread, &dwExitCode);
				::CloseHandle(hThread);
				return isOk ? dwExitCode : 0;
			}
			return 0;
		}
		/*
		
		ptr_t peb_ptr() {
			// ntdll.RtlGetCurrentPeb (64): 65 48 8B 04 25 30 00 00 00 48 8B 40 60 C3
			// ntdll.RtlGetCurrentPeb (32): 64 A1 18 00 00 00 8B 40 30 C3
		}

		inline auto peb64() {
			return rread<blackbone::_PEB64>(get_peb_ptr());
		};
		inline auto peb32() {
			return rread<blackbone::_PEB32>(get_peb_ptr());
		};
		inline auto ldrData64() {
			return rread<blackbone::_PEB_LDR_DATA264>(peb64().Ldr);
		}
		inline auto ldrData32() {
			return rread<blackbone::_PEB_LDR_DATA232>(peb32().Ldr);
		}

		inline auto entry64(ptr_t link) {
			return rread<blackbone::_LDR_DATA_TABLE_ENTRY_BASE64>(ptr_t(CONTAINING_RECORD(link, blackbone::_LDR_DATA_TABLE_ENTRY_BASE64, InMemoryOrderLinks)));
		}
		inline auto entry32(ptr_t link) {
			return rread<blackbone::_LDR_DATA_TABLE_ENTRY_BASE32>(ptr_t(CONTAINING_RECORD(link, blackbone::_LDR_DATA_TABLE_ENTRY_BASE32, InMemoryOrderLinks)));
		}
		
		*/

		ptr_t get_module_handle(std::string module_name) {
			return 0;
		}
		ptr_t get_proc_address(ptr_t module_addr, std::string proc_name) {
			return 0;
		}


		virtual bool virtual_free(ptr_t addr) noexcept = 0;

		virtual ptr_t virtual_alloc(size_t size, bool exec = false, ptr_t desired = 0)  noexcept = 0;

		virtual bool read_memory(ptr_t addr, ptr_t buffer, ptr_t size) noexcept = 0;

		virtual bool write_memory(ptr_t addr, ptr_t buffer, ptr_t size) noexcept = 0;

		virtual bool create_thread(ptr_t& thread_id, ptr_t addr, ptr_t argument = 0) noexcept = 0;

	private:
		bool _is64 = 8 == sizeof(intptr_t); ptr_t _process_id = 0;
	};



	struct clsKernelProcessWrapper final : public clsProcessWrapper {

		~clsKernelProcessWrapper() {
			if (_process_handle) Processes::Descriptors::KbCloseHandle(_process_handle);
		};

		clsKernelProcessWrapper(ptr_t processId, bool is64)
			: clsProcessWrapper(processId, is64) {
			Processes::Descriptors::KbOpenProcess(ULONG(processId), &_process_handle);
		};

		bool virtual_free(ptr_t addr) noexcept {
			return Processes::MemoryManagement::KbFreeUserMemory(ULONG(process_id()), WdkTypes::PVOID(addr));
		};
		ptr_t virtual_alloc(size_t size, bool exec = false, ptr_t desired = 0)  noexcept {
			DWORD dwProtect = exec ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
			return Processes::MemoryManagement::KbAllocUserMemory(ULONG(process_id()), ULONG(dwProtect), ULONG(size), (WdkTypes::PVOID*)(&desired)) ? desired : 0;
		};
		bool read_memory(ptr_t addr, ptr_t buffer, ptr_t size) noexcept {
			return Processes::MemoryManagement::KbReadProcessMemory(ULONG(process_id()), WdkTypes::PVOID(addr), PVOID(buffer), ULONG(size));
		};
		bool write_memory(ptr_t addr, ptr_t buffer, ptr_t size) noexcept {
			return Processes::MemoryManagement::KbWriteProcessMemory(ULONG(process_id()), WdkTypes::PVOID(addr), PVOID(buffer), ULONG(size));
		};
		bool create_thread(ptr_t& thread_id, ptr_t addr, ptr_t argument = 0) noexcept {
			WdkTypes::HANDLE hThread = NULL;
			WdkTypes::CLIENT_ID clientId = { UINT64(process_id()) };
			return bool(thread_id = Processes::Threads::KbCreateUserThread(ULONG(process_id()), WdkTypes::PVOID(addr), WdkTypes::PVOID(argument), FALSE, &clientId, &hThread) ? ptr_t(clientId.ThreadId) : 0);
		};

	private:
		const WdkTypes::HANDLE hProcess() { return _process_handle; };
		WdkTypes::HANDLE _process_handle = NULL;
	};


	struct clsUserProcessWrapper final : public clsProcessWrapper {

		~clsUserProcessWrapper() {
			if (_process_handle) ::CloseHandle(_process_handle);
		};

		clsUserProcessWrapper(ptr_t processId, bool is64)
			: clsProcessWrapper(processId, is64) {
			// https://docs.microsoft.com/en-us/windows/win32/procthread/process-security-and-access-rights
			_process_handle = ::OpenProcess(\
				PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE \
				| PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD \
				| SYNCHRONIZE | PROCESS_DUP_HANDLE, \
				FALSE, DWORD(processId)
			);
		};

		bool virtual_free(ptr_t addr) noexcept {
			return ::VirtualFreeEx(hProcess(), PVOID(addr), NULL, MEM_RELEASE);
		};
		ptr_t virtual_alloc(size_t size, bool exec = false, ptr_t desired = 0)  noexcept {
			DWORD dwProtect = exec ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
			return ptr_t(::VirtualAllocEx(hProcess(), PVOID(desired), SIZE_T(size), MEM_COMMIT | MEM_RESERVE, dwProtect));
		};
		bool read_memory(ptr_t addr, ptr_t buffer, ptr_t size) noexcept {
			SIZE_T numberOfBytes = 0;
			return ::ReadProcessMemory(hProcess(), PVOID(addr), PVOID(buffer), SIZE_T(size), &numberOfBytes) && size == ptr_t(numberOfBytes);
		};
		bool write_memory(ptr_t addr, ptr_t buffer, ptr_t size) noexcept {
			SIZE_T numberOfBytes = 0;
			return ::WriteProcessMemory(hProcess(), PVOID(addr), PVOID(buffer), SIZE_T(size), &numberOfBytes) && size == ptr_t(numberOfBytes);
		};
		bool create_thread(ptr_t& thread_id, ptr_t addr, ptr_t argument = 0) noexcept {
			HANDLE hThread = NULL; DWORD dwThreadId = 0;
			hThread = ::CreateRemoteThread(hProcess(), NULL, NULL, PTHREAD_START_ROUTINE(addr), PVOID(argument), 0, &dwThreadId);
			return bool(thread_id = ((hThread && dwThreadId) ? dwThreadId : 0));
		};

	private:
		const HANDLE hProcess() { return _process_handle; };
		HANDLE _process_handle = NULL;
	};

}