#pragma once


#include "clrUtils.h"



// https://github.com/DarthTon/Blackbone
#include <BlackBone/Include/NativeStructures.h>
#include <BlackBone/PE/PEImage.h>


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

	private:

		// https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-system_info
		SYSTEM_INFO _sysInfo = { };
		VOID safeGetNativeSystemInfo(
			__out LPSYSTEM_INFO lpSystemInfo
		)
		{
			HMODULE hModule = NULL;
			if (NULL == lpSystemInfo || !(hModule = GetModuleHandleW(L"kernel32"))) return;

			typedef VOID(WINAPI* fnGetNativeSystemInfo)(LPSYSTEM_INFO lpSystemInfo);
			fnGetNativeSystemInfo pfnGetNativeSystemInfo = \
				(fnGetNativeSystemInfo) \
				GetProcAddress(hModule, "GetNativeSystemInfo");
			if (pfnGetNativeSystemInfo)
				pfnGetNativeSystemInfo(lpSystemInfo);
			else
				GetSystemInfo(lpSystemInfo);

			return;
		}

		inline auto peb64() {
			return read_memory_t<blackbone::_PEB64>(process_peb());
		};
		inline auto peb32() {
			return read_memory_t<blackbone::_PEB32>(process_peb());
		};
		inline auto ldrData64() {
			return read_memory_t<blackbone::_PEB_LDR_DATA264>(peb64().Ldr);
		}
		inline auto ldrData32() {
			return read_memory_t<blackbone::_PEB_LDR_DATA232>(peb32().Ldr);
		}
		inline auto entry64(ptr_t link) {
			return read_memory_t<blackbone::_LDR_DATA_TABLE_ENTRY_BASE64>(ptr_t(CONTAINING_RECORD(link, blackbone::_LDR_DATA_TABLE_ENTRY_BASE64, InMemoryOrderLinks)));
		}
		inline auto entry32(ptr_t link) {
			return read_memory_t<blackbone::_LDR_DATA_TABLE_ENTRY_BASE32>(ptr_t(CONTAINING_RECORD(link, blackbone::_LDR_DATA_TABLE_ENTRY_BASE32, InMemoryOrderLinks)));
		}


		// https://docs.microsoft.com/zh-cn/windows/win32/api/psapi/ns-psapi-moduleinfo
		struct module_info_ex {
			ptr_t lpBaseOfDll;
			uint32_t SizeOfImage;
			ptr_t EntryPoint;
			fs::path fsFullPath;
		};

		using Handler = std::function<bool(module_info_ex&)>;

		bool compare_module(Handler handler) {

			bool b64 = process_64();

			ptr_t firstlink = b64 ? ldrData64().InMemoryOrderModuleList.Flink \
				: ldrData32().InMemoryOrderModuleList.Flink;

			ptr_t templink = firstlink;

			do {
				wchar_t fullName[MAX_PATH] = { };

				module_info_ex module_info = { };

				if (b64) {
					auto e64 = entry64(templink);
					read_memory(e64.FullDllName.Buffer, fullName, e64.FullDllName.MaximumLength);

					module_info = {
						ptr_t(e64.DllBase),
						uint32_t(e64.SizeOfImage),
						ptr_t(e64.EntryPoint),
						fullName
					};

					templink = e64.InMemoryOrderLinks.Flink;
				}
				else {
					auto e32 = entry32(templink);
					read_memory(e32.FullDllName.Buffer, fullName, e32.FullDllName.MaximumLength);

					module_info = {
						ptr_t(e32.DllBase),
						uint32_t(e32.SizeOfImage),
						ptr_t(e32.EntryPoint),
						fullName,
					};

					templink = e32.InMemoryOrderLinks.Flink;
				}
				if (handler(module_info)) return true;
			} while (firstlink != (b64 ? entry64(templink).InMemoryOrderLinks.Flink : entry32(templink).InMemoryOrderLinks.Flink));

			return false;
		}


	public:

		~clsProcessWrapper() {

		};

		clsProcessWrapper(ptr_t processId)
			: _process_id(processId) {

			safeGetNativeSystemInfo(&_sysInfo);
		};


		inline const bool os64() { 
			return _sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 
				|| _sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64; 
		};

		inline const size_t pagelength() { return size_t(_sysInfo.dwPageSize); }

		inline const ptr_t process_id() { return _process_id; };

		inline const bool process_64() { return os64() && (isWow64(&_wow64) && !_wow64); };


		template<typename T>
		T read_memory_t(ptr_t addr) {
			T buffer = { };
			return read_memory(addr, &buffer, sizeof(T)) ? buffer : T();
		};

		std::string read_memory_string(ptr_t addr, size_t length=-1) {
			std::string tmpstr(-1 == length ? 1024 : length, 0);
			return read_memory(addr, tmpstr.data(), tmpstr.length()) ? (-1 == length ? tmpstr.c_str() : tmpstr) : std::string();
		}

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


		ptr_t get_module_handle(fs::path module_name) {
			ptr_t module_base = 0;

			bool isOk = compare_module(
				[&](module_info_ex& info) {

					// https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlew
					fs::path m = module_name.has_extension() ? module_name : (module_name.wstring() + L".dll");

					if (module_name.empty()) {
						module_base = info.lpBaseOfDll;
						return true;
					}

					bool isFull = module_name.has_parent_path();

					auto srcPath = (isFull ? m : m.filename()).wstring();
					auto dstPath = (isFull ? info.fsFullPath : info.fsFullPath.filename()).wstring();
					
					if (CSTR_EQUAL == CompareStringOrdinal(srcPath.c_str(), -1, dstPath.c_str(), -1, TRUE)) {
						module_base = info.lpBaseOfDll;
						return true;
					}

					return false;
				}
			);
			
			return isOk ? module_base : 0;
		}
		ptr_t get_proc_address(ptr_t module_addr, std::string proc_name) {

			ptr_t procAddr = 0;

			if (!module_addr || proc_name.empty()) return 0;

			bool isOk = compare_module(
				[&](module_info_ex& info) {
					return (info.lpBaseOfDll == module_addr);
				}
			);

			if (!isOk) return 0;

			// PE Parse
			PVOID PEBuffer = VirtualAlloc(NULL, pagelength(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if (read_memory(module_addr, PEBuffer, pagelength())) {

				blackbone::pe::PEImage pe_helper;

				if (!NT_SUCCESS(pe_helper.Parse(PEBuffer))) {
					goto _cleanup;
				}

				auto pExportDir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
					pe_helper.DirectoryAddress(IMAGE_DIRECTORY_ENTRY_EXPORT, blackbone::pe::AddressType::RVA)
				);
				if (NULL == pExportDir) {
					goto _cleanup;
				}

				IMAGE_EXPORT_DIRECTORY ExportDir = { };
				ExportDir = read_memory_t<IMAGE_EXPORT_DIRECTORY>(module_addr + ptr_t(pExportDir));

				// 0 == OptionalHeader.AddressOfEntryPoint (IMAGE_EXPORT_DIRECTORY::Base)
				for (DWORD idx = 0; idx < ExportDir.NumberOfNames; ++idx) {

					DWORD u8NameOffset = read_memory_t<DWORD>(module_addr + (ExportDir.AddressOfNames + (sizeof(DWORD) * idx)));
					std::string u8Name = read_memory_string(module_addr + u8NameOffset);

					if (u8Name == proc_name) {

						WORD nameOrdOffset = read_memory_t<WORD>(module_addr + (ExportDir.AddressOfNameOrdinals + (sizeof(WORD) * idx)));

						DWORD fnOffset = read_memory_t<DWORD>(module_addr + (ExportDir.AddressOfFunctions + (sizeof(DWORD) * nameOrdOffset)));

						procAddr = fnOffset ? module_addr + fnOffset : 0;
					}
				}

			}

		_cleanup:

			return procAddr;
		}


		virtual bool virtual_free(ptr_t addr) noexcept = 0;

		virtual ptr_t virtual_alloc(size_t size, bool exec = false, ptr_t desired = 0)  noexcept = 0;

		virtual bool read_memory(ptr_t addr, void* buffer, size_t size) noexcept = 0;

		virtual bool write_memory(ptr_t addr, void* buffer, size_t size) noexcept = 0;

		virtual bool create_thread(ptr_t& thread_id, ptr_t addr, ptr_t argument = 0) noexcept = 0;

		virtual bool isWow64(bool* bWow64) noexcept = 0;

		// ntdll.RtlGetCurrentPeb (64): 65 48 8B 04 25 30 00 00 00 48 8B 40 60 C3
		// ntdll.RtlGetCurrentPeb (32): 64 A1 18 00 00 00 8B 40 30 C3
		virtual ptr_t process_peb() noexcept = 0;

private:
		bool _is64 = 8 == sizeof(intptr_t);
		bool _wow64 = false; ptr_t _process_id = 0;
	};



	struct clsKernelProcessWrapper final : public clsProcessWrapper {

		~clsKernelProcessWrapper() {
			if (_process_handle) Processes::Descriptors::KbCloseHandle(_process_handle);
		};

		clsKernelProcessWrapper(ptr_t processId)
			: clsProcessWrapper(processId) {
			Processes::Descriptors::KbOpenProcess(ULONG(processId), &_process_handle);
		};

		bool virtual_free(ptr_t addr) noexcept {
			return Processes::MemoryManagement::KbFreeUserMemory(ULONG(process_id()), WdkTypes::PVOID(addr));
		};
		ptr_t virtual_alloc(size_t size, bool exec = false, ptr_t desired = 0)  noexcept {
			DWORD dwProtect = exec ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
			return Processes::MemoryManagement::KbAllocUserMemory(ULONG(process_id()), ULONG(dwProtect), ULONG(size), (WdkTypes::PVOID*)(&desired)) ? desired : 0;
		};
		bool read_memory(ptr_t addr, void* buffer, size_t size) noexcept {
			return Processes::MemoryManagement::KbReadProcessMemory(ULONG(process_id()), WdkTypes::PVOID(addr), buffer, ULONG(size));
		};
		bool write_memory(ptr_t addr, void* buffer, size_t size) noexcept {
			return Processes::MemoryManagement::KbWriteProcessMemory(ULONG(process_id()), WdkTypes::PVOID(addr), buffer, ULONG(size));
		};
		bool create_thread(ptr_t& thread_id, ptr_t addr, ptr_t argument = 0) noexcept {
			WdkTypes::HANDLE hThread = NULL;
			WdkTypes::CLIENT_ID clientId = { UINT64(process_id()) };
			return bool(thread_id = Processes::Threads::KbCreateUserThread(ULONG(process_id()), WdkTypes::PVOID(addr), WdkTypes::PVOID(argument), FALSE, &clientId, &hThread) ? ptr_t(clientId.ThreadId) : 0);
		};


		bool isWow64(bool* bWow64) noexcept {
			ULONG_PTR wow64 = { };
			bool isOk = Processes::Information::KbQueryInformationProcess(hProcess(), NtTypes::PROCESSINFOCLASS::ProcessWow64Information, &wow64, sizeof(ULONG_PTR));
			if (isOk && bWow64) *bWow64 = !!wow64;
			return isOk;
		}

		ptr_t process_peb() noexcept {
			bool wow64 = false; bool isOk = isWow64(&wow64);

			if (isOk && wow64) {
				ULONG_PTR wow64Env = { };
				if (Processes::Information::KbQueryInformationProcess(hProcess(), NtTypes::PROCESSINFOCLASS::ProcessWow64Information, &wow64Env, sizeof(ULONG_PTR), NULL)) {
					return wow64Env;
				}
			}
			else {
				PROCESS_BASIC_INFORMATION basicInfo = { };
				if (Processes::Information::KbQueryInformationProcess(hProcess(), NtTypes::PROCESSINFOCLASS::ProcessBasicInformation, &basicInfo, sizeof(PROCESS_BASIC_INFORMATION), NULL)) {
					return ptr_t(basicInfo.PebBaseAddress);
				}
			}
			return 0;
		}

	private:
		const WdkTypes::HANDLE hProcess() { return _process_handle; };
		WdkTypes::HANDLE _process_handle = NULL;
	};


	struct clsUserProcessWrapper final : public clsProcessWrapper {

		~clsUserProcessWrapper() {
			if (_process_handle) ::CloseHandle(_process_handle);
		};

		clsUserProcessWrapper(ptr_t processId)
			: clsProcessWrapper(processId) {
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
		bool read_memory(ptr_t addr, void* buffer, size_t size) noexcept {
			return ::ReadProcessMemory(hProcess(), PVOID(addr), buffer, size, NULL);
		};
		bool write_memory(ptr_t addr, void* buffer, size_t size) noexcept {
			SIZE_T numberOfBytes = 0;
			return ::WriteProcessMemory(hProcess(), PVOID(addr), buffer, size, NULL);
		};
		bool create_thread(ptr_t& thread_id, ptr_t addr, ptr_t argument = 0) noexcept {
			HANDLE hThread = NULL; DWORD dwThreadId = 0;
			hThread = ::CreateRemoteThread(hProcess(), NULL, NULL, PTHREAD_START_ROUTINE(addr), PVOID(argument), 0, &dwThreadId);
			return bool(thread_id = ((hThread && dwThreadId) ? dwThreadId : 0));
		};


		bool isWow64(bool* bWow64) noexcept {
			ULONG_PTR wow64 = { };
			bool isOk = NT_SUCCESS(NtQueryInformationProcess(hProcess(), PROCESSINFOCLASS::ProcessWow64Information, &wow64, sizeof(ULONG_PTR), NULL));
			if (isOk && bWow64) *bWow64 = !!wow64;
			return isOk;
		}

		ptr_t process_peb() noexcept {
			bool wow64 = false; bool isOk = isWow64(&wow64);

			if (isOk && wow64) {
				ULONG_PTR wow64Env = { };
				if (NT_SUCCESS(NtQueryInformationProcess(hProcess(), PROCESSINFOCLASS::ProcessWow64Information, &wow64Env, sizeof(ULONG_PTR), NULL))) {
					return wow64Env;
				}
			}
			else {
				PROCESS_BASIC_INFORMATION basicInfo = { };
				if (NT_SUCCESS(NtQueryInformationProcess(hProcess(), PROCESSINFOCLASS::ProcessBasicInformation, &basicInfo, sizeof(PROCESS_BASIC_INFORMATION), NULL))) {
					return ptr_t(basicInfo.PebBaseAddress);
				}
			}
			return 0;
		}

	private:
		const HANDLE hProcess() { return _process_handle; };
		HANDLE _process_handle = NULL;
	};

}