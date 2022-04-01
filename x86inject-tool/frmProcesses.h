#pragma once

#include "clrUtils.h"

namespace x86injecttool {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// frmProcesses 摘要
	/// </summary>
	public ref class frmProcesses : public System::Windows::Forms::Form
	{
	public:
		frmProcesses(void)
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
		~frmProcesses()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::ListView^ m_processes;
	protected:
	private: System::Windows::Forms::ColumnHeader^ columnHeader1;
	private: System::Windows::Forms::ColumnHeader^ columnHeader2;
	private: System::Windows::Forms::ColumnHeader^ columnHeader3;
	private: System::Windows::Forms::ColumnHeader^ columnHeader4;
	private: System::Windows::Forms::ListView^ m_modules;
	private: System::Windows::Forms::ColumnHeader^ columnHeader5;
	private: System::Windows::Forms::ColumnHeader^ columnHeader6;
	private: System::Windows::Forms::ColumnHeader^ columnHeader7;
	private: System::Windows::Forms::Button^ m_btn_refresh;

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
			this->m_processes = (gcnew System::Windows::Forms::ListView());
			this->columnHeader1 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader2 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader3 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader4 = (gcnew System::Windows::Forms::ColumnHeader());
			this->m_modules = (gcnew System::Windows::Forms::ListView());
			this->columnHeader5 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader6 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader7 = (gcnew System::Windows::Forms::ColumnHeader());
			this->m_btn_refresh = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// m_processes
			// 
			this->m_processes->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(4) {
				this->columnHeader1,
					this->columnHeader2, this->columnHeader3, this->columnHeader4
			});
			this->m_processes->FullRowSelect = true;
			this->m_processes->GridLines = true;
			this->m_processes->HideSelection = false;
			this->m_processes->Location = System::Drawing::Point(12, 54);
			this->m_processes->MultiSelect = false;
			this->m_processes->Name = L"m_processes";
			this->m_processes->Size = System::Drawing::Size(886, 372);
			this->m_processes->TabIndex = 1;
			this->m_processes->UseCompatibleStateImageBehavior = false;
			this->m_processes->View = System::Windows::Forms::View::Details;
			this->m_processes->SelectedIndexChanged += gcnew System::EventHandler(this, &frmProcesses::m_processes_SelectedIndexChanged);
			this->m_processes->DoubleClick += gcnew System::EventHandler(this, &frmProcesses::m_processes_DoubleClick);
			// 
			// columnHeader1
			// 
			this->columnHeader1->Text = L"Id";
			this->columnHeader1->Width = 120;
			// 
			// columnHeader2
			// 
			this->columnHeader2->Text = L"Name";
			this->columnHeader2->Width = 150;
			// 
			// columnHeader3
			// 
			this->columnHeader3->Text = L"Title";
			this->columnHeader3->Width = 150;
			// 
			// columnHeader4
			// 
			this->columnHeader4->Text = L"Path";
			this->columnHeader4->Width = 200;
			// 
			// m_modules
			// 
			this->m_modules->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(3) {
				this->columnHeader5, this->columnHeader6,
					this->columnHeader7
			});
			this->m_modules->FullRowSelect = true;
			this->m_modules->GridLines = true;
			this->m_modules->HideSelection = false;
			this->m_modules->Location = System::Drawing::Point(12, 432);
			this->m_modules->MultiSelect = false;
			this->m_modules->Name = L"m_modules";
			this->m_modules->Size = System::Drawing::Size(886, 229);
			this->m_modules->TabIndex = 2;
			this->m_modules->UseCompatibleStateImageBehavior = false;
			this->m_modules->View = System::Windows::Forms::View::Details;
			// 
			// columnHeader5
			// 
			this->columnHeader5->Text = L"Base";
			this->columnHeader5->Width = 120;
			// 
			// columnHeader6
			// 
			this->columnHeader6->Text = L"Size";
			this->columnHeader6->Width = 120;
			// 
			// columnHeader7
			// 
			this->columnHeader7->Text = L"Path";
			this->columnHeader7->Width = 400;
			// 
			// m_btn_refresh
			// 
			this->m_btn_refresh->Location = System::Drawing::Point(12, 12);
			this->m_btn_refresh->Name = L"m_btn_refresh";
			this->m_btn_refresh->Size = System::Drawing::Size(83, 36);
			this->m_btn_refresh->TabIndex = 3;
			this->m_btn_refresh->Text = L"Refresh";
			this->m_btn_refresh->UseVisualStyleBackColor = true;
			this->m_btn_refresh->Click += gcnew System::EventHandler(this, &frmProcesses::m_btn_refresh_Click);
			// 
			// frmProcesses
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 15);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(913, 675);
			this->Controls->Add(this->m_btn_refresh);
			this->Controls->Add(this->m_modules);
			this->Controls->Add(this->m_processes);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
			this->MaximizeBox = false;
			this->Name = L"frmProcesses";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
			this->Text = L"选择目标";
			this->Load += gcnew System::EventHandler(this, &frmProcesses::frmProcesses_Load);
			this->ResumeLayout(false);

		}
#pragma endregion
	public: TargetProcessInfo target;
	private: System::Void frmProcesses_Load(System::Object^ sender, System::EventArgs^ e);
	private: System::Void m_btn_refresh_Click(System::Object^ sender, System::EventArgs^ e);
	private: System::Void m_processes_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e);
	private: System::Void m_processes_DoubleClick(System::Object^ sender, System::EventArgs^ e);
	};
}
