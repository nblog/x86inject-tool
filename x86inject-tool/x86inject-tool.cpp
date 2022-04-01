#include "pch.h"
#include "frmInjecter.h"


#ifndef _DEBUG
// https://docs.microsoft.com/en-us/cpp/build/reference/entry-entry-point-symbol
#pragma comment( linker, "/subsystem:windows /entry:main" )
#endif


using namespace System;


int main(array<System::String ^> ^args)
{
    x86injecttool::Application::EnableVisualStyles();
    x86injecttool::Application::Run(gcnew x86injecttool::frmInjecter());

    return 0;
}
