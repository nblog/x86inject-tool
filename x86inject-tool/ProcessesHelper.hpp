#pragma once


#include <Windows.h>
#include <Winternl.h>
#include <Psapi.h>
#include <tlhelp32.h>
#pragma comment(lib, "ntdll")

EXTERN_C CONST IMAGE_DOS_HEADER __ImageBase;




// https://github.com/x64dbg/asmjit_xedparse
#ifndef _WIN64
#pragma comment(lib, "../third_party/XEDParse/XEDParse_x86.lib")
#else
#pragma comment(lib, "../third_party/XEDParse/XEDParse_x64.lib")
#endif
#include <XEDParse.h>



#include "ProcessWrapper.hpp"






namespace x86injecttool {


	struct module_info {
		ptr_t base, size;
		fs::path path;
	};

	struct process_info {
		ptr_t id; boolean is64;
		fs::path path, title;
	};

	struct thread_info {
		ptr_t id; ptr_t startaddr;
		long pri;
	};

	typedef std::vector<thread_info> threads;
	typedef std::vector<process_info> processes;
	typedef std::vector<module_info> modules;



	inline fs::path current_path() {
		WCHAR exec[MAX_PATH] = { };
		GetModuleFileNameW(HINSTANCE(&__ImageBase), exec, MAX_PATH);
		return fs::path(exec).parent_path();
	}


	template <typename T>
	struct Callback;

	template <typename Ret, typename... Params>
	struct Callback<Ret(Params...)> {
		template <typename... Args>
		static Ret callback(Args... args) {
			return func(args...);
		}
		static std::function<Ret(Params...)> func;
	};

	template <typename Ret, typename... Params>
	std::function<Ret(Params...)> Callback<Ret(Params...)>::func;


	struct clsAssembleJit
	{
	public:
		const ptr_t baseAddr() { return symbols()["base_addr"]; };
		const ptr_t execAddr() { return symbols()["wrapper_addr"]; };
		const ptr_t resultAddr() { return symbols()["result_addr"]; };

		const std::string makeCode() { return _makeCode; };
		const std::string lastErr() { return _lastErr; };


		inline size_t _align_(size_t val, size_t alignment) {
			return (val % alignment == 0) ? val : (val / alignment + 1) * alignment;
		}


		clsAssembleJit(bool b64, ptr_t baseAddr=0)
			: is64(b64) {
			// https://docs.microsoft.com/zh-cn/cpp/build/reference/base-base-address
			label("base_addr", 0 == baseAddr ? (b64 ? 0x140000000 : 0x400000) : baseAddr);


			label("result_addr");

			ptr_t retAddr = 0; 
			buildCode().append((const char *)(&retAddr), is64 ? 8 : 4);
		};


		bool wrapper() {
			label("wrapper_addr");

			if (!assembler("call user_addr") ||
				!assembler(std::string("mov [result_addr], ") + (is64 ? "rax" : "eax")) ||
				!assembler("ret")) {
				_lastErr = "wrapper error"; return false;
			}
			return true;
		}

		bool assembler(List<String^>^ contents) {
			label("user_addr");

			for (int i = 0; i < contents->Count; i++) {
				if (!assembler(marshal_as<std::string>(contents[i]))) {
					_lastErr = "[line: " + std::to_string(i + 1) + "]: " + _lastErr;
					return false;
				}
			}
			return wrapper();
		}

		bool strings(Dictionary<String^, array<Byte>^>^ contents) {
			for each (auto c in contents) {
				label(marshal_as<std::string>(c.Key));

				std::string bs(size_t(c.Value->Length), '\00'); 
				Marshal::Copy(c.Value, 0, IntPtr(bs.data()), c.Value->Length);

				buildCode().append(bs); buildCode().append(2, '\00');
			}
			return true;
		}

		bool imports(Dictionary<String^, IntPtr>^ contents) {
			for each (auto c in contents) {
				label(marshal_as<std::string>(c.Key));

				ptr_t fnPtr = ptr_t(c.Value.ToPointer());
				buildCode().append((const char*)(&fnPtr), is64 ? 8 : 4);
			}
			return true;
		}

	private:

		bool XEDPARSE_CALL sym_resolver(const char* text, ULONGLONG* value) {
			if (symbols().end() != symbols().find(text)) {
				*value = symbols()[text];
				return true;
			}
			return false;
		}

		bool assembler(const std::string content) {
			XEDPARSE parse = {  }; _lastErr = "";

			memcpy(parse.instr, content.data(), content.length());

			//
			Callback<bool(const char*, ULONGLONG*)>::func = std::bind(&clsAssembleJit::sym_resolver, this, std::placeholders::_1, std::placeholders::_2);
			parse.cbUnknown = static_cast<CBXEDPARSE_UNKNOWN>(Callback<bool(const char*, ULONGLONG*)>::callback);

			parse.x64 = bool(is64); 
			parse.cip = baseAddr() + buildCode().length();

			if (XEDPARSE_STATUS::XEDPARSE_OK == XEDParseAssemble(&parse)) {
				buildCode().append((const char*)(parse.dest), size_t(parse.dest_size));
				return true;
			}
			_lastErr = parse.error;
			return false;
		}

		std::string& buildCode() { return _makeCode; }
		std::unordered_map<std::string, ptr_t>& symbols() { return _symbol_table; }
		void label(const std::string l, ptr_t r=0) { symbols()[l] = r ? r : (baseAddr() + buildCode().length()); }



		bool is64;
		std::string _makeCode, _lastErr;
		std::unordered_map<std::string, ptr_t> _symbol_table;
	};


	struct clsProcessesHelper {

		static \
			inline bool is64exec(fs::path exec)
		{
			if (!fs::exists(exec)) return 8 == sizeof(intptr_t);

			std::string content(1024, '\x00');
			fs::ifstream(exec, std::ios::binary).read(content.data(), content.length());

			return is64exec(content);
		}

		// https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-image-only
		static \
			inline bool is64exec(std::string content)
		{
			if (1024 > content.length() || !('M' == content[0] && 'Z' == content[1])) {
				throw std::runtime_error("not executable program");
			}

			const uintptr_t buffer = uintptr_t(content.data());

			PIMAGE_NT_HEADERS inth = reinterpret_cast<PIMAGE_NT_HEADERS>(buffer + *reinterpret_cast<PDWORD>(buffer + 0x3c));

			return inth->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
		}


		// https://docs.microsoft.com/en-us/windows/win32/toolhelp/traversing-the-thread-list
		static \
			threads enum_threads(ptr_t processId)
		{
			threads ts; ts.clear();

			THREADENTRY32 te32 = { sizeof(THREADENTRY32) };

			HANDLE hThreadSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
			if (INVALID_HANDLE_VALUE == hThreadSnap) {
				goto _cleanup;
			}

			if (!::Thread32First(hThreadSnap, &te32)) {
				goto _cleanup;
			}

			do {
				if (!processId || te32.th32OwnerProcessID == DWORD(processId)) {

					ptr_t startAddr = 0;

					{
						HANDLE hThread = ::OpenThread(THREAD_QUERY_INFORMATION, FALSE, te32.th32ThreadID);

						// https://docs.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationthread
						if (hThread) {
							::NtQueryInformationThread(hThread, THREADINFOCLASS(NtTypes::ThreadQuerySetWin32StartAddress), &startAddr, sizeof(startAddr), NULL);
							::CloseHandle(hThread);
						}
					}

					ts.push_back({
						ptr_t(te32.th32ThreadID), startAddr,
						long(te32.tpBasePri) });
				}
			} while (::Thread32Next(hThreadSnap, &te32));

		_cleanup:
			if (hThreadSnap) ::CloseHandle(hThreadSnap);
			return ts;
		}


		// https://docs.microsoft.com/en-us/windows/win32/psapi/enumerating-all-processes
		// https://docs.microsoft.com/en-us/windows/win32/toolhelp/taking-a-snapshot-and-viewing-processes
		static \
			processes enum_processes() 
		{
			processes ps; ps.clear();

			PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };

			HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (INVALID_HANDLE_VALUE == hProcessSnap) {
				goto _cleanup;
			}

			if (!::Process32First(hProcessSnap, &pe32)) {
				goto _cleanup;
			}

			do {
				// skip self
				if (::GetCurrentProcessId() == pe32.th32ProcessID) continue;

				fs::path path = pe32.szExeFile;

				{
					DWORD dwSize = MAX_PATH;
					WCHAR szExeName[MAX_PATH] = { };

					HANDLE hProcess = ::OpenProcess( \
						PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
					if (hProcess) {
						// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-queryfullprocessimagenamew
						if (::QueryFullProcessImageNameW(hProcess, 0, szExeName, &dwSize) && dwSize) {
							path = szExeName;
						}
						::CloseHandle(hProcess);
					}
				}

				ps.push_back({ \
					ptr_t(pe32.th32ProcessID),
					is64exec(path),
					path, "" });

			} while (::Process32Next(hProcessSnap, &pe32));

		_cleanup:
			if (hProcessSnap) ::CloseHandle(hProcessSnap);
			return ps;
		}


		// https://docs.microsoft.com/en-us/windows/win32/psapi/enumerating-all-modules-for-a-process
		static \
			modules enum_modules(ptr_t processId)
		{
			modules ms; ms.clear();

			HMODULE hMods[1024] = { };
			DWORD cbNeeded = 0;

			HANDLE hProcess = ::OpenProcess( \
				PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, DWORD(processId));

			// https://docs.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumprocessmodulesex
			if (NULL == hProcess || !::EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_DEFAULT)) {
				goto _cleanup;
			}

			for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
				if (!hMods[i]) continue;

				WCHAR szModName[MAX_PATH] = { };
				if (!::GetModuleFileNameExW(hProcess, hMods[i], szModName, MAX_PATH))
					continue;

				MODULEINFO moduleInfo = { };
				if (!::GetModuleInformation(hProcess, hMods[i], &moduleInfo, sizeof(MODULEINFO)))
					continue;

				ms.push_back({ \
					ptr_t(moduleInfo.lpBaseOfDll), ptr_t(moduleInfo.SizeOfImage),
					fs::path(szModName)});
			}

		_cleanup:
			if (hProcess) ::CloseHandle(hProcess);
			return ms;
		}

	};


}