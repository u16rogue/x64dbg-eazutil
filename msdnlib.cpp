#include "msdnlib.hpp"
#include "xsfd_utils.hpp"

xsfd::debug_lib_provider::debug_lib_provider(ICLRRuntimeInfo * rinfo_)
	: rinfo(rinfo_), ref_count(1)
{
	XSFD_DEBUG_LOG("!Debug library provider instance created.\n");
}

xsfd::debug_lib_provider::~debug_lib_provider()
{
	XSFD_DEBUG_LOG("!Debug library provider instance destroyed.\n");
	if (ref_count)
	{
		if (rinfo)
		{
			while (rinfo->Release() > 0);
			rinfo = nullptr;
		}
		while (this->Release() > 0);
	}
}

auto STDMETHODCALLTYPE xsfd::debug_lib_provider::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR * ppvObject) -> HRESULT
{
	if (!IsEqualIID(riid, __uuidof(IUnknown)) && !IsEqualIID(riid, __uuidof(ICLRDebuggingLibraryProvider)))
		return E_NOINTERFACE;
	this->AddRef();
	*ppvObject = this;
	return S_OK;
}

auto STDMETHODCALLTYPE xsfd::debug_lib_provider::AddRef(void) -> ULONG
{
	return InterlockedIncrement(&ref_count);
}

auto STDMETHODCALLTYPE xsfd::debug_lib_provider::Release(void) -> ULONG
{
	auto count = InterlockedDecrement(&ref_count);
	if (count < 1 && rinfo)
	{
		rinfo->Release();
		rinfo = nullptr;
		delete this; // Only call delete manually if there's still rinfo, if delete was called rinfo will already be cleared by the destructor. This will prevent a recursive free.
	}
	return count;
}

auto STDMETHODCALLTYPE xsfd::debug_lib_provider::ProvideLibrary(const WCHAR * pwszFileName, DWORD dwTimestamp, DWORD dwSizeOfImage, HMODULE * phModule) -> HRESULT
{
	auto r = rinfo->LoadLibraryA(pwszFileName, phModule);
	if (r != S_OK)
		xsfd::log("!Debug library provider failed to privide a requested library.\n");
	return r;
}

xsfd::debug_data_target::debug_data_target(HANDLE hnd_)
	: hnd(hnd_), ref_count(1)
{
	XSFD_DEBUG_LOG("!Debug data target instance created.\n");
}

xsfd::debug_data_target::~debug_data_target()
{
	XSFD_DEBUG_LOG("!Debug data target instance destroyed.\n");
	if (ref_count)
	{
		hnd = nullptr;
		while (this->Release() > 0);
	}
}

auto STDMETHODCALLTYPE xsfd::debug_data_target::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) -> HRESULT
{
	if (IsEqualIID(riid, __uuidof(ICLRDataTarget)) || (!IsEqualIID(riid, __uuidof(IUnknown)) && !IsEqualIID(riid, __uuidof(ICorDebugDataTarget))))
		return E_NOINTERFACE;

	this->AddRef();
	*ppvObject = this;

	return S_OK;
}

auto STDMETHODCALLTYPE xsfd::debug_data_target::AddRef(void) -> ULONG 
{
	return InterlockedIncrement(&ref_count);
}

auto STDMETHODCALLTYPE xsfd::debug_data_target::Release(void) -> ULONG 
{
	auto count = InterlockedDecrement(&ref_count);
	if (count < 1 && hnd)
		delete this;
	return count;
}

auto STDMETHODCALLTYPE xsfd::debug_data_target::GetPlatform(CorDebugPlatform * pTargetPlatform) -> HRESULT 
{
	BOOL is_wow = FALSE;
	if (!IsWow64Process(hnd, &is_wow))
	{
		xsfd::log("!Debug data target is wow 64 check failed.\n");
		return S_FALSE;
	}
	*pTargetPlatform = is_wow ? CORDB_PLATFORM_WINDOWS_X86 : CORDB_PLATFORM_WINDOWS_AMD64;
	return S_OK;
}

auto STDMETHODCALLTYPE xsfd::debug_data_target::ReadVirtual(CORDB_ADDRESS address, BYTE * pBuffer, ULONG32 bytesRequested, ULONG32 * pBytesRead) -> HRESULT 
{
	SIZE_T read = 0;
	auto r = ReadProcessMemory(hnd, reinterpret_cast<LPVOID>(address), pBuffer, bytesRequested, &read);
	if (pBytesRead)
		*pBytesRead = read;
	if (!r)
	{
		xsfd::log("!Debug data target failed to read virtual memory from target process.\n");
		return S_FALSE;
	}

	return S_OK;
}

auto STDMETHODCALLTYPE xsfd::debug_data_target::GetThreadContext(DWORD dwThreadID, ULONG32 contextFlags, ULONG32 contextSize, BYTE * pContext) -> HRESULT 
{
	if (sizeof(CONTEXT) != contextSize)
	{
		xsfd::log("!Debug data target requested thread context does not match the ideal size of %d bytes (requested %d bytes)\n", sizeof(CONTEXT), contextSize);
		return E_FAIL;
	}

	HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, dwThreadID);
	if (!thread)
	{
		xsfd::log("!Debug Data Target failed to create an open handle to thread id %d\n", dwThreadID);
		return E_FAIL;
	}

	XSFD_DEFER {
		CloseHandle(thread);
	};

	CONTEXT ctx = { .ContextFlags = contextFlags };
	if (!::GetThreadContext(thread, &ctx))
	{
		xsfd::log("!Debug data target failed to get thread id %d context\n", dwThreadID);
		return E_FAIL;
	}

	std::memcpy(pContext, &ctx, sizeof(CONTEXT));

	return S_OK;
}
