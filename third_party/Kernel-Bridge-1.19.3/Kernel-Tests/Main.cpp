#include "pch.h"

#include "WdkTypes.h"
#include "CtlTypes.h"
#include "FltTypes.h"
#include "User-Bridge.h"
#include "Rtl-Bridge.h"

#include <fltUser.h>
#include "CommPort.h"
#include "Flt-Bridge.h"

#include "Kernel-Tests.h"

#include <vector>
#include <string>
#include <iostream>
#include <set>
#include <fstream>

#define _NO_CVCONST_H
#include <dbghelp.h>
#include "SymParser.h"

#include <PTE.h>
#include <Registers.h>
#include <VMX.h>

#include <intrin.h>

void RunTests() {
    BeeperTest tBeeper(L"Beeper");
    IoplTest tIopl(L"IOPL");
    VirtualMemoryTest tVirtualMemory(L"VirtualMemory");
    MdlTest tMdl(L"Mdl");
    PhysicalMemoryTest tPhysicalMemory(L"PhysicalMemory");
    ProcessesTest tProcesses(L"Processes");
    ShellTest tShell(L"Shells");
    StuffTest tStuff(L"Stuff");
}

void TranslationTest(PVOID Address)
{
    using namespace VirtualMemory;
    using namespace PhysicalMemory;
    using namespace KernelShells;
    using namespace Processes::MemoryManagement;
    using namespace KbRtl;

    VIRTUAL_ADDRESS Va = {};
    Va.Value = reinterpret_cast<SIZE_T>(Address);

    printf("Target VA: %p\r\n", Address);

    VirtualLock(Address, 1);

    using REGS = struct {
        CR3 Cr3;
        CR4 Cr4;
    };
    REGS Regs = {};

    KbExecuteShellCode([](auto GetKernelProcAddress, auto Argument) -> ULONG {
        REGS* Regs = (REGS*)Argument;
        Regs->Cr3.Value = __readcr3();
        Regs->Cr4.Value = __readcr4();
        return 0;
    }, &Regs);

    PML4E Pml4e = {};
    PDPE Pdpe = {};
    PDE Pde = {};
    PTE Pte = {};

    try {
        WdkTypes::PVOID pPml4e = PFN_TO_PAGE(Regs.Cr3.x64.Bitmap.PML4) + Va.x64.NonPageSize.Page4Kb.PageMapLevel4Offset * sizeof(Pml4e);
        WdkTypes::PVOID VirtPml4e = PhysMem::GetVirtualForPhysical(pPml4e);
        Pml4e.x64.Value = VirtMem::ReadQword(VirtPml4e);
        printf("PML4E: VA = %p, PA = %p\r\n", (PVOID)VirtPml4e, (PVOID)pPml4e);
        //Pml4e.x64.Page4Kb.US = 1;
        //VirtMem::WriteQword(VirtPml4e, Pml4e.x64.Value);

        WdkTypes::PVOID pPdpe = PFN_TO_PAGE(Pml4e.x64.Page4Kb.PDP) + Va.x64.NonPageSize.Page4Kb.PageDirectoryPointerOffset * sizeof(Pdpe);
        WdkTypes::PVOID VirtPdpe = PhysMem::GetVirtualForPhysical(pPdpe);
        Pdpe.x64.Value = VirtMem::ReadQword(VirtPdpe);
        printf("PDPE: VA = %p, PA = %p\r\n", (PVOID)VirtPdpe, (PVOID)pPdpe);
        //Pdpe.x64.Page4Kb.US = 1;
        //VirtMem::WriteQword(VirtPdpe, Pdpe.x64.Value);

        WdkTypes::PVOID pPde = PFN_TO_PAGE(Pdpe.x64.NonPageSize.Page4Kb.PD) + Va.x64.NonPageSize.Page4Kb.PageDirectoryOffset * sizeof(Pde);
        WdkTypes::PVOID VirtPde = PhysMem::GetVirtualForPhysical(pPde);
        Pde.x64.Value = VirtMem::ReadQword(VirtPde);
        printf("PDE: VA = %p, PA = %p\r\n", (PVOID)VirtPde, (PVOID)pPde);
        //Pde.x64.Page4Kb.US = 1;
        //VirtMem::WriteQword(VirtPde, Pde.x64.Value);

        WdkTypes::PVOID pPte = PFN_TO_PAGE(Pde.x64.Page4Kb.PT) + Va.x64.NonPageSize.Page4Kb.PageTableOffset * sizeof(Pte);
        WdkTypes::PVOID VirtPte = PhysMem::GetVirtualForPhysical(pPte);
        Pte.x64.Value = VirtMem::ReadQword(VirtPte);
        printf("PTE: VA = %p, PA = %p\r\n", (PVOID)VirtPte, (PVOID)pPte);
        //Pte.x64.Page4Kb.US = 1;
        printf("> AVL: %i, G: %i, A: %i, D: %i\n", (int)Pte.x64.Page4Kb.AVL, (int)Pte.x64.Page4Kb.G, (int)Pte.x64.Page4Kb.A, (int)Pte.x64.Page4Kb.D);
        //Pte.x64.Page4Kb.AVL = 0b101; // Trigger CoW
        //VirtMem::WriteQword(VirtPte, Pte.x64.Value);

        WdkTypes::PVOID PhysicalAddress = PFN_TO_PAGE(Pte.x64.Page4Kb.PhysicalPageFrameNumber) + Va.x64.NonPageSize.Page4Kb.PageOffset;
        WdkTypes::PVOID ValidPhysicalAddress = PhysMem::GetPhysAddress(Va.Value);
        printf("PA = 0x%llX, VPA = 0x%llX\n", PhysicalAddress, ValidPhysicalAddress);

        //PULONG KMem = (PULONG)Address;
        //*KMem = *KMem;

        PhysicalAddress = PFN_TO_PAGE(Pte.x64.Page4Kb.PhysicalPageFrameNumber) + Va.x64.NonPageSize.Page4Kb.PageOffset;
        ValidPhysicalAddress = PhysMem::GetPhysAddress(Va.Value);
        printf("PA = 0x%llX, VPA = 0x%llX\n", PhysicalAddress, ValidPhysicalAddress);

        if (PhysicalAddress == ValidPhysicalAddress)
            printf("Addresses are matches, PA = 0x%llX\n", PhysicalAddress);
        
    } catch (DWORD LastError) {
        printf("LE: 0x%X\r\n", LastError);
    }

    //KbFreeNonCachedMemory(KernelMemory, 4096);
}

void SmmTest() {
    SetThreadAffinityMask(GetCurrentThread(), 1);

    UINT64 MsrBaseAddress = 0x30000;
    CPU::KbReadMsr(0xC0010111, &MsrBaseAddress);

    union SMM_ADDR {
        unsigned long long Value;
        struct {
            unsigned long long Reserved0 : 17;
            unsigned long long Base : 35;
            unsigned long long Reserved1 : 12;
        } Bitmap;
    };

    union SMM_MASK {
        unsigned long long Value;
        struct {
            unsigned long long AE : 1;
            unsigned long long TE : 1;
            unsigned long long Reserved0 : 15;
            unsigned long long Base : 35;
            unsigned long long Reserved1 : 12;
        } Bitmap;
    };

    SMM_ADDR MsrProtectedBase = {};
    CPU::KbReadMsr(0xC0010112, &MsrProtectedBase.Value);

    SMM_MASK MsrProtectedMask = {};
    CPU::KbReadMsr(0xC0010113, &MsrProtectedMask.Value);

    __debugbreak();
}

void print_cpuid() {
    int regs[4] = {};
    __cpuid(regs, 0);
    char str[13] = {};
    // CPUID Vendor = RBX + RDX + RCX:
    *(int*)(str + 0) = regs[1]; // RBX
    *(int*)(str + 4) = regs[3]; // RDX
    *(int*)(str + 8) = regs[2]; // RCX
    printf("CPU: %s\r\n", str);
}

void RandomRpmTest() {
    using namespace VirtualMemory;
    using namespace Processes::MemoryManagement;

    PVOID Buffer = VirtualAlloc(NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    VirtualLock(Buffer, 4096);

    const WdkTypes::PVOID Base = 0xFFFFFFFFFFFFFFFF;
    for (auto i = Base; i >= 0x7FFFFFFFFFFFFFFF; i -= 4096) {
        BOOL Status = KbReadProcessMemory(GetCurrentProcessId(), Base, Buffer, 4096);
        if (Status) {
            printf("[%p] OK\r\n", (PVOID)i);
            break;
        }
    }

    VirtualFree(Buffer, 4096, MEM_FREE);
    printf("Random RPM OK\r\n");
}

//#define FLT_TEST

#ifdef FLT_TEST
CommPortListener<KB_FLT_OB_CALLBACK_INFO, KbObCallbacks> ObCallbacks;

void TestObCallbacks()
{
    // Prevent to open our process with PROCESS_VM_READ rights:
    BOOL Status = ObCallbacks.Subscribe([](CommPort & Port, MessagePacket<KB_FLT_OB_CALLBACK_INFO> & Message) -> VOID {
        auto Data = static_cast<PKB_FLT_OB_CALLBACK_INFO>(Message.GetData());
        if (Data->Target.ProcessId == GetCurrentProcessId()) {
            Data->CreateResultAccess &= ~(PROCESS_VM_READ | PROCESS_TERMINATE | PROCESS_SUSPEND_RESUME);
            Data->DuplicateResultAccess &= ~(PROCESS_VM_READ | PROCESS_TERMINATE | PROCESS_SUSPEND_RESUME);
            printf("Access attempted from %i\r\n", static_cast<int>(Data->Client.ProcessId));
        }
        ReplyPacket<KB_FLT_OB_CALLBACK_INFO> Reply(Message, ERROR_SUCCESS, *Data);
        Port.Reply(Reply); // Reply info to driver
    });

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
}

#endif


DWORD GetPidByName(LPCWSTR Name)
{
    DWORD ProcessId = 0xFFFFFFFF;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return ProcessId;

    PROCESSENTRY32 ProcessEntry = {};
    ProcessEntry.dwSize = sizeof(ProcessEntry);
    if (Process32First(hSnapshot, &ProcessEntry)) do {
        if (wcsstr(ProcessEntry.szExeFile, Name)) {
            ProcessId = ProcessEntry.th32ProcessID;
            break;
        }
    } while (Process32Next(hSnapshot, &ProcessEntry));

    CloseHandle(hSnapshot);
    return ProcessId;
}

VOID ThreadingTests()
{
    using namespace Processes;
    DWORD ExplorerPid = GetPidByName(L"explorer.exe");
    printf("PID of explorer.exe is %u\r\n", ExplorerPid);
    PVOID Proc = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "Sleep");
    WdkTypes::CLIENT_ID ClientId = {};
    WdkTypes::HANDLE hThread = NULL;
    printf("Creating a thread...\r\n");
    BOOL Status = Threads::KbCreateUserThread(ExplorerPid, reinterpret_cast<WdkTypes::PVOID>(Proc), 10000, FALSE, &ClientId, &hThread);
    if (Status) {
        printf("PID:%I64u, TID:%I64u, hThread = 0x%I64X\r\n", ClientId.ProcessId, ClientId.ThreadId, hThread);
        WaitForSingleObject(reinterpret_cast<HANDLE>(hThread), INFINITE);
        printf("Thread is finished!\r\n");
        Descriptors::KbCloseHandle(hThread);
        printf("Handle is closed!\r\n");
    }
    else {
        printf("Unable to create a thread!\r\n");
    }
}

void* GetFuncPtr(const void* Func)
{
    if (!Func) return nullptr;
    const auto* FuncDataPtr = reinterpret_cast<const unsigned char*>(Func);
    if (*FuncDataPtr == 0xE9)
    {
        auto Offset = *reinterpret_cast<const int*>(FuncDataPtr + 1);
        return const_cast<unsigned char*>((FuncDataPtr + 5) + Offset);
    }
    else
    {
        return const_cast<void*>(Func);
    }
}

#pragma section(".hidden", read, execute, nopage)
__declspec(code_seg(".hidden")) unsigned int HiddenFunc()
{
    printf("Called from hidden func!\n");
    for (unsigned int i = 0; i < 100; ++i)
    {
        volatile BYTE* Self = reinterpret_cast<PBYTE>(GetFuncPtr(HiddenFunc));
        *Self = 0x55;
    }
    return 0x1EE7C0DE;
}

VOID TestHvPageInterception()
{
    using namespace Hypervisor;
    using namespace PhysicalMemory;

    constexpr unsigned int PageSize = 4096;

    volatile PBYTE Read = reinterpret_cast<PBYTE>(VirtualAlloc(NULL, PageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    volatile PBYTE Write = reinterpret_cast<PBYTE>(VirtualAlloc(NULL, PageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    volatile PBYTE WriteExecute = reinterpret_cast<PBYTE>(VirtualAlloc(NULL, PageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    volatile PBYTE Execute = reinterpret_cast<PBYTE>(GetFuncPtr(HiddenFunc));

    __assume(Read != 0);
    __assume(Write != 0);
    __assume(WriteExecute != 0);

    VirtualLock(Read, PageSize);
    VirtualLock(Write, PageSize);
    VirtualLock(Execute, PageSize);
    VirtualLock(WriteExecute, PageSize);

    memset(Read, 0xFF, PageSize);
    memset(Write, 0xEE, PageSize);
    memcpy(WriteExecute, Execute, PageSize);

    DWORD OldProtect = 0;
    VirtualProtect(Execute, PageSize, PAGE_EXECUTE_READWRITE, &OldProtect);
    *Execute = *Execute;

    WdkTypes::PVOID64 ReadPa = 0, WritePa = 0, WriteExecutePa = 0, ExecutePa = 0;
    KbGetPhysicalAddress(NULL, reinterpret_cast<WdkTypes::PVOID>(Read), &ReadPa);
    KbGetPhysicalAddress(NULL, reinterpret_cast<WdkTypes::PVOID>(Write), &WritePa);
    KbGetPhysicalAddress(NULL, reinterpret_cast<WdkTypes::PVOID>(WriteExecute), &WriteExecutePa);
    KbGetPhysicalAddress(NULL, reinterpret_cast<WdkTypes::PVOID>(Execute), &ExecutePa);
    
    printf("Original bytes: 0x%X\n", static_cast<unsigned int>(*Execute));

    KbVmmInterceptPage(ExecutePa, ReadPa, WritePa, ExecutePa, ExecutePa, WriteExecutePa);    
    printf("Bytes after intercept: 0x%X\n", static_cast<unsigned int>(*Execute));
    *Execute = 0x40;
    HiddenFunc();
    printf("Bytes after call: 0x%X\n", static_cast<unsigned int>(*Execute));
    printf("Write-interceptor: W:0x%X WX:0x%X\n", static_cast<unsigned int>(*Write), static_cast<unsigned int>(*WriteExecute));
    KbVmmDeinterceptPage(ExecutePa);

    printf("Bytes after deintercept: 0x%X\n", static_cast<unsigned int>(*Execute));

    VirtualUnlock(Execute, PageSize);
    VirtualUnlock(Write, PageSize);
    VirtualUnlock(Read, PageSize);

    VirtualFree(Write, 0, MEM_RELEASE);
    VirtualFree(Read, 0, MEM_RELEASE);
}

WdkTypes::PVOID GetPhysAddr(OPTIONAL WdkTypes::PEPROCESS Process, PVOID VirtualAddress)
{
    using namespace PhysicalMemory;
    WdkTypes::PVOID64 Pa = 0;
    KbGetPhysicalAddress(Process, reinterpret_cast<WdkTypes::PVOID>(VirtualAddress), &Pa);
    return Pa;
}

VOID VirtualMemoryTests()
{
    using namespace Processes::MemoryManagement;

    HMODULE hModule = GetModuleHandle(L"ntdll.dll");
    __assume(hModule != 0);
    PVOID Address = GetProcAddress(hModule, "NtTestAlert");

    printf("NtTestAlert: VA:%p, PA:0x%I64X\n", Address, GetPhysAddr(NULL, Address));
    KbTriggerCopyOnWrite(0, reinterpret_cast<WdkTypes::PVOID>(Address));
    printf("NtTestAlert: CoW-PA:0x%I64X\n", GetPhysAddr(NULL, Address));

    KbTriggerCopyOnWrite(0, reinterpret_cast<WdkTypes::PVOID>(Address));
    printf("NtTestAlert: Double CoW-PA:0x%I64X\n", GetPhysAddr(NULL, Address));

    for (unsigned long long i = 0; i < 100000; ++i)
    {
        KbTriggerCopyOnWrite(0, reinterpret_cast<WdkTypes::PVOID>(Address) + i * 4096ull);
    }
}

VOID RunAllTests()
{
    VirtualMemoryTests();
    return;

    //ThreadingTests();
    //return;

#ifdef FLT_TEST
    TestObCallbacks();
#endif

    //RunTests();
    //RandomRpmTest();

    //PVOID Addr = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "SetLastError");
    //TranslationTest(Addr);

    if (Hypervisor::KbVmmEnable())
    {
        printf("VMM enabled!\r\n");

        TestHvPageInterception();

        printf(
            "Commands:\n"
            "  exit: disable the hypervisor and exit\n"
        );
        while (true)
        {
            std::wstring Command;
            std::wcin >> Command;
            if (Command == L"exit")
            {
                break;
            }
            else
            {
                printf("Unknown command!\n");
            }
        }
        Hypervisor::KbVmmDisable();
        printf("VMM disabled!\r\n");
        print_cpuid();
    }
    else
    {
        printf("Unable to start VMM!\r\n");
    }
}

std::wstring GetCurrentFolder()
{
    WCHAR Path[MAX_PATH] = {};
    DWORD PathLength = ARRAYSIZE(Path);
    BOOL Status = QueryFullProcessImageName(GetCurrentProcess(), 0, Path, &PathLength);
    return Status ? std::wstring(Path, PathLength) : std::wstring();
}

int main()
{
    std::wstring CurrentFolder = GetCurrentFolder();
    if (CurrentFolder.empty())
    {
        printf("Unable to determine current directory!\n");
        return 0;
    }

    printf("[Kernel-Tests]: PID: %i, TID: %i\r\n", GetCurrentProcessId(), GetCurrentThreadId());

    if (KbLoader::KbLoadAsFilter(
        L"C:\\Temp\\Kernel-Bridge\\Kernel-Bridge.sys",
        L"260000" // Altitude of minifilter
    )) {
        RunAllTests();
        KbLoader::KbUnload();
    } else {
        std::wcout << L"Unable to load driver! LastError: 0x" << std::hex << GetLastError() << std::endl;
    }

    std::wcout << L"Press any key to exit..." << std::endl;
    std::cin.get();

    return 0;
}