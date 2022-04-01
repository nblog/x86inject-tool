#pragma once

#include "frmProcesses.h"

namespace x86injecttool {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// frmInjecter 摘要
	/// </summary>
	public ref class frmInjecter : public System::Windows::Forms::Form
	{
	public:
		frmInjecter(void)
		{
			InitializeComponent();
			//
			//TODO:  在此处添加构造函数代码
			//
		}

	protected:
		/// <summary>
		/// 清理所有正在使用的资源。
		/// </summary>
		~frmInjecter()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Button^ btn_processes;
	private: System::Windows::Forms::RichTextBox^ m_richtxt_assembler;

	private: System::Windows::Forms::Label^ label1;
	private: System::Windows::Forms::Label^ label2;
	private: System::Windows::Forms::TextBox^ m_txt_processid;
	private: System::Windows::Forms::TextBox^ m_txt_processname;
	private: System::Windows::Forms::Label^ label3;
	private: System::Windows::Forms::Label^ label4;
	private: System::Windows::Forms::TextBox^ m_txt_allocaddr;
	private: System::Windows::Forms::TextBox^ m_txt_resultvalue;
	private: System::Windows::Forms::Button^ m_btn_inject;






	protected:

	private:
		/// <summary>
		/// 必需的设计器变量。
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// 设计器支持所需的方法 - 不要修改
		/// 使用代码编辑器修改此方法的内容。
		/// </summary>
		void InitializeComponent(void)
		{
			System::ComponentModel::ComponentResourceManager^ resources = (gcnew System::ComponentModel::ComponentResourceManager(frmInjecter::typeid));
			this->btn_processes = (gcnew System::Windows::Forms::Button());
			this->m_richtxt_assembler = (gcnew System::Windows::Forms::RichTextBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->m_txt_processid = (gcnew System::Windows::Forms::TextBox());
			this->m_txt_processname = (gcnew System::Windows::Forms::TextBox());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->m_txt_allocaddr = (gcnew System::Windows::Forms::TextBox());
			this->m_txt_resultvalue = (gcnew System::Windows::Forms::TextBox());
			this->m_btn_inject = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// btn_processes
			// 
			this->btn_processes->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"btn_processes.Image")));
			this->btn_processes->Location = System::Drawing::Point(12, 12);
			this->btn_processes->Name = L"btn_processes";
			this->btn_processes->Size = System::Drawing::Size(48, 45);
			this->btn_processes->TabIndex = 0;
			this->btn_processes->UseVisualStyleBackColor = true;
			this->btn_processes->Click += gcnew System::EventHandler(this, &frmInjecter::btn_processes_Click);
			// 
			// m_richtxt_assembler
			// 
			this->m_richtxt_assembler->Location = System::Drawing::Point(12, 63);
			this->m_richtxt_assembler->Name = L"m_richtxt_assembler";
			this->m_richtxt_assembler->Size = System::Drawing::Size(680, 678);
			this->m_richtxt_assembler->TabIndex = 1;
			this->m_richtxt_assembler->Text = L"";
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(80, 27);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(87, 15);
			this->label1->TabIndex = 2;
			this->label1->Text = L"ProcessId:";
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(305, 27);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(103, 15);
			this->label2->TabIndex = 3;
			this->label2->Text = L"ProcessName:";
			// 
			// m_txt_processid
			// 
			this->m_txt_processid->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->m_txt_processid->Location = System::Drawing::Point(173, 27);
			this->m_txt_processid->Name = L"m_txt_processid";
			this->m_txt_processid->ReadOnly = true;
			this->m_txt_processid->Size = System::Drawing::Size(126, 18);
			this->m_txt_processid->TabIndex = 4;
			// 
			// m_txt_processname
			// 
			this->m_txt_processname->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->m_txt_processname->Location = System::Drawing::Point(414, 27);
			this->m_txt_processname->Name = L"m_txt_processname";
			this->m_txt_processname->ReadOnly = true;
			this->m_txt_processname->Size = System::Drawing::Size(356, 18);
			this->m_txt_processname->TabIndex = 5;
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(695, 618);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(119, 15);
			this->label3->TabIndex = 6;
			this->label3->Text = L"Allocate Addr:";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(695, 699);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(111, 15);
			this->label4->TabIndex = 7;
			this->label4->Text = L"Result Value:";
			// 
			// m_txt_allocaddr
			// 
			this->m_txt_allocaddr->Location = System::Drawing::Point(695, 636);
			this->m_txt_allocaddr->Name = L"m_txt_allocaddr";
			this->m_txt_allocaddr->ReadOnly = true;
			this->m_txt_allocaddr->Size = System::Drawing::Size(175, 25);
			this->m_txt_allocaddr->TabIndex = 8;
			// 
			// m_txt_resultvalue
			// 
			this->m_txt_resultvalue->Location = System::Drawing::Point(695, 717);
			this->m_txt_resultvalue->Name = L"m_txt_resultvalue";
			this->m_txt_resultvalue->ReadOnly = true;
			this->m_txt_resultvalue->Size = System::Drawing::Size(175, 25);
			this->m_txt_resultvalue->TabIndex = 9;
			// 
			// m_btn_inject
			// 
			this->m_btn_inject->Location = System::Drawing::Point(698, 64);
			this->m_btn_inject->Name = L"m_btn_inject";
			this->m_btn_inject->Size = System::Drawing::Size(172, 42);
			this->m_btn_inject->TabIndex = 10;
			this->m_btn_inject->Text = L"Inject";
			this->m_btn_inject->UseVisualStyleBackColor = true;
			this->m_btn_inject->Click += gcnew System::EventHandler(this, &frmInjecter::m_btn_inject_Click);
			// 
			// frmInjecter
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 15);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(882, 753);
			this->Controls->Add(this->m_btn_inject);
			this->Controls->Add(this->m_txt_resultvalue);
			this->Controls->Add(this->m_txt_allocaddr);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->m_txt_processname);
			this->Controls->Add(this->m_txt_processid);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->m_richtxt_assembler);
			this->Controls->Add(this->btn_processes);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
			this->Name = L"frmInjecter";
			this->FormClosed += gcnew System::Windows::Forms::FormClosedEventHandler(this, &frmInjecter::frmInjecter_FormClosed);
			this->Load += gcnew System::EventHandler(this, &frmInjecter::frmInjecter_Load);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	protected: frmProcesses processes;
	private: System::Void frmInjecter_FormClosed(System::Object^ sender, System::Windows::Forms::FormClosedEventArgs^ e);
	private: System::Void frmInjecter_Load(System::Object^ sender, System::EventArgs^ e);
	private: System::Void btn_processes_Click(System::Object^ sender, System::EventArgs^ e);
	private: System::Void m_btn_inject_Click(System::Object^ sender, System::EventArgs^ e);
};
}
