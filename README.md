# x86inject-tool

## Features
- **x86 and x64 support**
- **User and Kernel (Kernel-Bridge) support**
- **Strings support ( "" or L"" or u8"" )**
- **[Run-Time Dynamic Linking](https://docs.microsoft.com/zh-cn/windows/win32/dlls/run-time-dynamic-linking)**
- **[c-comments](https://docs.microsoft.com/zh-cn/cpp/c-language/c-comments)**

## Examples 32bit

    /* this is example (32bit) */
    push L"example.dll"
    call [kernel32.LoadLibraryW]
    ret

## Examples 64bit

    /* this is example (64bit) */
    sub rsp, 0x28
    mov rcx, L"example.dll"
    call [kernel32.LoadLibraryW]
    add rsp, 0x28
    ret

## Credits
- [asmjit_xedparse](https://github.com/x64dbg/asmjit_xedparse)
- [Kernel-Bridge](https://github.com/HoShiMin/Kernel-Bridge)
- [BlackBone](https://github.com/DarthTon/Blackbone)

## Compile
Visual Studio 2022 (C++/CLI)

## License
x86inject-tool is licensed under the WTFPL License. Dependencies are under their respective licenses.