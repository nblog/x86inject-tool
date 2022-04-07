#include "pch.h"
#include "frmInjecter.h"

#include "ProcessesHelper.hpp"


namespace x86injecttool {


	inline System::Void frmInjecter::frmInjecter_FormClosed(System::Object^ sender, System::Windows::Forms::FormClosedEventArgs^ e) {
		KbLoader::KbUnload();
	}

	inline System::Void frmInjecter::frmInjecter_Load(System::Object^ sender, System::EventArgs^ e) {

		fs::path kernelDrv = x86injecttool::current_path() / L"Kernel-Bridge.sys";
		if (fs::exists(kernelDrv)) KbLoader::KbLoadAsDriver(kernelDrv.wstring().c_str());

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

		clsUserProcessWrapper process(processes.target.processId);

		clsAssembleJit asmjit(process.process_64(), process.virtual_alloc(process.pagelength(), true));

		std::string codeBuffer = std::string();


		// .idata
		Dictionary<String^, IntPtr>^ importsTable = gcnew Dictionary<String^, IntPtr>();
		// .rdata
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


			System::Text::Encoding^ coder = System::Text::Encoding::Default;
			String^ c = String::Empty; String^ m = String::Empty;
			Boolean matched = false;


			// https://docs.microsoft.com/en-us/windows/win32/dlls/run-time-dynamic-linking
			matched = String::Empty != (m = Regex::Match(s, "\\[.*?\\]")->Value);

			if (m->StartsWith("[") && m->EndsWith("]")) {
				c = m->Substring(1, m->Length - 2);
			}

			if (matched) {
				String^ module_name = c->Substring(0, c->LastIndexOf('.'));
				String^ module_function = c->Substring(c->LastIndexOf('.') + 1);

				ptr_t fnPtr = process.get_proc_address(
					process.get_module_handle(marshal_as<std::wstring>(module_name)), marshal_as<std::string>(module_function)
				);

				if (fnPtr) {
					auto label = String::Format("l{0:x}", fnv::hashRuntime(marshal_as<std::string>(c).c_str()));
					importsTable[label] = IntPtr( int64_t(fnPtr) );

					s = s->Replace(c, label);
				}
			}

			// https://docs.microsoft.com/zh-cn/cpp/c-language/c-string-literals
			// https://zh.cppreference.com/w/cpp/language/string_literal
			matched = String::Empty != (m = Regex::Match(s, "(u8\".*\")|(L\".*\")|(\".*\")")->Value);

			if (m->StartsWith("\"")) {
				coder = System::Text::Encoding::Default;
				c = m->Substring(1, m->Length - 2);
			}
			else if (m->StartsWith("L\"")) {
				coder = System::Text::Encoding::Unicode;
				c = m->Substring(2, m->Length - 3);
			}
			else if (m->StartsWith("u8\"")) {
				coder = System::Text::Encoding::UTF8;
				c = m->Substring(3, m->Length - 4);
			}

			if (matched) {
				auto label = String::Format("l{0:x}", fnv::hashRuntime(marshal_as<std::string>(m).c_str()));
				stringsTable[label] = coder->GetBytes(c);

				s = s->Replace(m, label);
			}


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
		if (codeBuffer.empty() || process.pagelength() < codeBuffer.length()) {
			MessageBox::Show("failed to make code.", "error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			goto _cleanup;
		}
		
		if (!process.write_memory(asmjit.baseAddr(), codeBuffer.data(), codeBuffer.length())) {
			MessageBox::Show("failed to write shellcode.", "error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			goto _cleanup;
		}

		ptr_t result = 0;
		{
			ptr_t threadId = 0;

			if (!process.create_thread(threadId, asmjit.execAddr()) || !threadId) {
				MessageBox::Show("failed to create thread.", "error", MessageBoxButtons::OK, MessageBoxIcon::Error);
				goto _cleanup;
			}

			process.wait_for_thread(threadId);

			result = process.process_64() ? process.read_memory_t<uint64_t>(asmjit.resultAddr())
				: process.read_memory_t<uint32_t>(asmjit.resultAddr());
		}

		m_txt_allocaddr->Text = String::Format("{0:X16}", asmjit.baseAddr());
		m_txt_resultvalue->Text = String::Format("{0:X16}", result);

	_cleanup:

		if (asmjit.baseAddr()) process.virtual_free(asmjit.baseAddr());

		return;
	}


}

