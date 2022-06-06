#pragma once

class StringHelper
{
public:
	static std::wstring StringToWString(const std::string& s);
	static std::string WStringToString(const std::wstring& s);

};
