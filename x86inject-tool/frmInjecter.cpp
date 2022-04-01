#include "pch.h"
#include "frmInjecter.h"

#include "ProcessesHelper.hpp"


namespace x86injecttool {


	inline System::Void frmInjecter::frmInjecter_FormClosed(System::Object^ sender, System::Windows::Forms::FormClosedEventArgs^ e) {
		KbLoader::KbUnload();
	}

	inline System::Void frmInjecter::frmInjecter_Load(System::Object^ sender, System::EventArgs^ e) {

		fs::path kernelDrv = x86injecttool::current_path() / L"Kernel-Bridge.sys";
		if (fs::exists(kernelDrv)) KbLoader::KbLoadAsDriver(kernelDrv.c_str());

		return;
	}

	inline System::Void frmInjecter::btn_processes_Click(System::Object^ sender, System::EventArgs^ e) {
		processes.ShowDialog();

		if (0 == processes.target.processId) return;

		m_txt_processid->Text = String::Format("{0}", processes.target.processId);
		m_txt_processname->Text = String::Format("{0} {1}", processes.target.processName, processes.target.processb64 ? "(64)" : "(32)");
	}

	inline System::Void frmInjecter::m_btn_inject_Click(System::Object^ sender, System::EventArgs^ e) {

		if (0 == processes.target.processId) return;


		m_txt_allocaddr->Text = ""; m_txt_resultvalue->Text = "";

		const size_t pageSize = 4096;

		clsUserProcessWrapper process(processes.target.processId, processes.target.processb64);

		clsAssembleJit asmjit(process.is64(), process.virtual_alloc(pageSize, true));

		std::string codeBuffer = std::string();


		// .rdata
		Dictionary<String^, IntPtr>^ importsTable = gcnew Dictionary<String^, IntPtr>();
		// .data
		Dictionary<String^, array<Byte>^>^ stringsTable = gcnew Dictionary<String^, array<Byte>^>();
		// .code
		List<String^>^ codesTable = gcnew List<String^>();


		StringReader reader(m_richtxt_assembler->Text);
		while (String^ s = reader.ReadLine()) {

			if (String::Empty == s) continue;

			// https://docs.microsoft.com/zh-cn/cpp/c-language/c-comments 
			// ( "/* */" or "//" )
			s = Regex::Replace(s, "(/\\*.*?\\*/)|(//.*)", String::Empty)->Trim();

			if (String::Empty == s) continue;

			String^ content = String::Empty; String^ m = String::Empty;
			System::Text::Encoding^ coder = System::Text::Encoding::Default;

			// https://docs.microsoft.com/zh-cn/cpp/c-language/c-string-literals
			// https://zh.cppreference.com/w/cpp/language/string_literal
			m = Regex::Match(s, "(u8\".*\")|(L\".*\")|(\".*\")")->Value;
			if (m->StartsWith("\"")) {
				coder = System::Text::Encoding::Default;
				content = m->Substring(1, m->Length - 2);

				stringsTable[m] = coder->GetBytes(content);
			}
			else if (m->StartsWith("L\"")) {
				coder = System::Text::Encoding::Unicode;
				content = m->Substring(2, m->Length - 3);

				stringsTable[m] = coder->GetBytes(content);
			}
			else if (m->StartsWith("u8\"")) {
				coder = System::Text::Encoding::UTF8;
				content = m->Substring(3, m->Length - 4);

				stringsTable[m] = coder->GetBytes(content);
			}

			//m = Regex::Match(s, "\\[.*?\\]")->Value;
			//if (m->StartsWith("[") && m->EndsWith("]")) {
			//	content = m->Substring(1, m->Length - 2);

			//	auto mm = content->Split('.');
			//	if (2 == mm->Length) {
			//		auto fnPtr = process.get_proc_address( process.get_module_handle(""), "" );
			//		if (fnPtr) importsTable->Add(content, IntPtr(intptr_t(fnPtr)));
			//	}
			//}

			//
			codesTable->Add(s);
		}

		if (0 == codesTable->Count || 0 == asmjit.baseAddr()) {
			goto _cleanup;
		}

		// 
		if (!codesTable[codesTable->Count - 1]->StartsWith("ret", true, nullptr)) {
			if (Windows::Forms::DialogResult::No ==  \
				MessageBox::Show("doesn't seem to return (\"ret\"). continue?", "warn", MessageBoxButtons::YesNo, MessageBoxIcon::Warning)) {
				goto _cleanup;
			}
		}

		if (!asmjit.imports(importsTable) || !asmjit.strings(stringsTable) || !asmjit.assembler(codesTable)) {
			MessageBox::Show(marshal_as<String^>(asmjit.lastErr()), "error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			goto _cleanup;
		}

		codeBuffer = asmjit.makeCode();
		if (codeBuffer.empty() || pageSize < codeBuffer.length()) {
			MessageBox::Show("make code failed.", "error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			goto _cleanup;
		}
		
		process.write_memory(asmjit.baseAddr(), (ptr_t)(codeBuffer.data()), (ptr_t)(codeBuffer.length()));

		ptr_t result = 0;
		{
			ptr_t threadId = 0;

			process.create_thread(threadId, asmjit.execAddr());

			process.wait_for_thread(threadId);

			result = process.read_pointer(asmjit.resultAddr());
		}

		m_txt_allocaddr->Text = String::Format("{0:X16}", asmjit.baseAddr());
		m_txt_resultvalue->Text = String::Format("{0:X16}", result);

	_cleanup:

		if (asmjit.baseAddr()) process.virtual_free(asmjit.baseAddr());

		return;
	}


}

