#include "pch.h"
#include "frmProcesses.h"


#include "ProcessesHelper.hpp"



namespace x86injecttool {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;


	ref struct clrModuleInfo {
		UIntPtr moduleBase;
		UIntPtr moduleSize;
		String^ modulePath;
	};

	ref struct clrProcessInfo {
		UInt32 processId;
		Boolean processb64;
		String^ processName;
		String^ processTitle;
		String^ processPath;

		clrProcessInfo(ptr_t id, boolean is64, fs::path path, fs::path title) {
			processId = UInt32(id); processb64 = Boolean(is64);
			processPath = marshal_as<String^>(path.wstring());
			processName = System::IO::Path::GetFileName(processPath);
			processTitle = marshal_as<String^>(title.wstring());
		}
	};


	inline System::Void frmProcesses::frmProcesses_Load(System::Object^ sender, System::EventArgs^ e) {
		return m_btn_refresh_Click(nullptr, nullptr);
	}

	inline System::Void frmProcesses::m_btn_refresh_Click(System::Object^ sender, System::EventArgs^ e) {
		m_processes->Items->Clear();

		for each (const auto& p in clsProcessesHelper::enum_processes()) {

			auto item = m_processes->Items->Add(String::Format("{0:X8}[{1}]", p.id, p.id));

			String^ path = marshal_as<String^>(p.path.wstring());

			String^ suffix = (System::Environment::Is64BitOperatingSystem && !p.is64) ? " *32" : "";

			item->SubItems->AddRange(gcnew array<String^>{ String::Format("{0}{1}", System::IO::Path::GetFileName(path), suffix), "", path });

			item->Tag = gcnew clrProcessInfo(p.id, p.is64, p.path, p.title);
		}
	}

	inline System::Void frmProcesses::m_processes_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e) {
		m_modules->Items->Clear();

		if (1 != m_processes->SelectedItems->Count || !m_processes->SelectedItems[0]->Tag) return;

		clrProcessInfo^ proc = static_cast<clrProcessInfo^>(m_processes->SelectedItems[0]->Tag);

		for each (const auto& m in clsProcessesHelper::enum_modules(ptr_t(proc->processId))) {
			auto item = m_modules->Items->Add(String::Format("{0:X16}", m.base));
			item->SubItems->AddRange(gcnew array<String^>{ String::Format("{0:X8}", m.size), marshal_as<String^>(m.path.wstring()) });
		}

	}

	inline System::Void frmProcesses::m_processes_DoubleClick(System::Object^ sender, System::EventArgs^ e) {

		//if (Windows::Forms::MouseButtons::Left != static_cast<Windows::Forms::MouseEventArgs^>(e)->Button) return;

		if (1 != m_processes->SelectedItems->Count || !m_processes->SelectedItems[0]->Tag) return;

		clrProcessInfo^ proc = static_cast<clrProcessInfo^>(m_processes->SelectedItems[0]->Tag);

		target.processId = proc->processId; target.processb64 = proc->processb64; target.processName = proc->processName;

		Form::Close();
	}


}
