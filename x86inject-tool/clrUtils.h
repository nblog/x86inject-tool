#pragma once


#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include <fstream>
#include <filesystem>

namespace fs {
	using namespace std::filesystem;
	using ifstream = std::ifstream;
	using ofstream = std::ofstream;
	using fstream = std::fstream;
}


#include <msclr\marshal.h>
#include <msclr\marshal_cppstd.h>


namespace x86injecttool {

	using namespace System;
	using namespace System::IO;
	using namespace System::Collections::Generic;
	using namespace msclr::interop;

	using namespace System::Runtime::InteropServices;
	using namespace System::Text::RegularExpressions;


	public ref struct TargetProcessInfo {
		UInt32 processId;
		Boolean processb64;
		String^ processName;
	};

}