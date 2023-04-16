// IEncryption.h: interface for the IEncryption class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IENCRYPTION_H__7741547B_BA15_4851_A41B_2B4EC1DC12D5__INCLUDED_)
#define AFX_IENCRYPTION_H__7741547B_BA15_4851_A41B_2B4EC1DC12D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4189)
#pragma warning(disable:4127)

//////////////////////////////////////////////////////////////////////

// function to be exported from dll to create instance of interface
#ifdef _EXPORTING // declare this in project settings for dll _only_
#	define DLL_DECLSPEC __declspec(dllexport)
#else
#	define DLL_DECLSPEC __declspec(dllimport)
#endif 

//////////////////////////////////////////////////////////////////////

const UINT IENCRYPTION_VERSION = 0;

//////////////////////////////////////////////////////////////////////

class IEncryption;

//////////////////////////////////////////////////////////////////////

typedef IEncryption* (*PFNCREATE)(); // function prototype
extern "C" DLL_DECLSPEC IEncryption* CreateEncryptionInterface();

typedef int (*PFNGETVERSION)(); // function prototype
extern "C" DLL_DECLSPEC int GetInterfaceVersion();

//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4505)

//////////////////////////////////////////////////////////////////////

// helper methods
static IEncryption* CreateEncryptionInterface(LPCWSTR szDllPath)
{
    IEncryption* pInterface = NULL;
    HMODULE hDll = LoadLibraryW(szDllPath);
	
    if (hDll)
    {
        PFNCREATE pCreate = (PFNCREATE)GetProcAddress(hDll, "CreateEncryptionInterface");
		
        if (pCreate)
		{
			try
			{
				// check version
				PFNGETVERSION pVersion = (PFNGETVERSION)GetProcAddress(hDll, "GetInterfaceVersion");

				if (!IENCRYPTION_VERSION || (pVersion && pVersion() >= IENCRYPTION_VERSION))
					pInterface = pCreate();
			}
			catch (...)
			{
			}
		}
    }
	
    return pInterface;
}

static BOOL IsEncryptionDll(LPCWSTR szDllPath)
{
    HMODULE hDll = LoadLibraryW(szDllPath);
	
    if (hDll)
    {
        PFNCREATE pCreate = (PFNCREATE)GetProcAddress(hDll, "CreateEncryptionInterface");
		FreeLibrary(hDll);

		return (NULL != pCreate);
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////

class IEncryption
{
public:
    virtual void Release() = 0; // releases the interface
	
    // returns a dynamically allocated buffer to the encrypted text
    // caller responsible for calling FreeBuffer on the returned buffer
    virtual bool Encrypt(const unsigned char* szInput, int nLenInput, const char* szPassword,
						 unsigned char*& pOutput, int& nLenOutput) = 0;
	
    // returns a dynamically allocated buffer to the decrypted text
    // caller responsible for calling FreeBuffer on the returned buffer
    virtual bool Decrypt(const unsigned char* pInput, int nLenInput, const char* szPassword,
						 unsigned char*& pOutput, int& nLenOutput) = 0;
	
    // frees a previously returned buffer and sets the ptr to NULL
    // eg for buffer allocated with 'new' use 'delete []'
    // eg for buffer allocated with 'strdup' use 'free'
    virtual void FreeBuffer(unsigned char*& pBuffer) = 0;
	
};

//////////////////////////////////////////////////////////////////////

static void ReleaseEncryptionInterface(IEncryption*& pInterface)
{
    if (pInterface)
    {
        pInterface->Release();
        pInterface = NULL;
    }
}

//////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_IENCRYPTION_H__7741547B_BA15_4851_A41B_2B4EC1DC12D5__INCLUDED_)
