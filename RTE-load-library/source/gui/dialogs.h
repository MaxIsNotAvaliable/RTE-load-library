#pragma once
#include <base.h>
#include <shobjidl.h>
#include <string>

struct FileDialog
{
	static std::string Open()
	{
		std::string filename = "";
		IFileOpenDialog* pFileOpen;
		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			COMDLG_FILTERSPEC rgSpec[] = 
			{
				   { L"DLL Files", L"*.dll" },
			};
			pFileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
			hr = pFileOpen->Show(NULL);
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					filename = convert::WideToANSI(pszFilePath);
					CoTaskMemFree(pszFilePath);
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		return filename;
	}
};