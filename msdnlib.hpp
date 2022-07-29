#pragma once

// This serves as an alternative to some MS dotNet libraries

#include <Windows.h>
#include <mscoree.h>
#include <metahost.h>
#include <clrdata.h>
//#include <cordebug.h>

// [26/07/2022] Cant directly inherit from cordebug.h since im not using MSVC as a compiler
typedef enum CorDebugPlatform
{
	CORDB_PLATFORM_WINDOWS_X86	= 0,
	CORDB_PLATFORM_WINDOWS_AMD64	= ( CORDB_PLATFORM_WINDOWS_X86 + 1 ) ,
	CORDB_PLATFORM_WINDOWS_IA64	= ( CORDB_PLATFORM_WINDOWS_AMD64 + 1 ) ,
	CORDB_PLATFORM_MAC_PPC	= ( CORDB_PLATFORM_WINDOWS_IA64 + 1 ) ,
	CORDB_PLATFORM_MAC_X86	= ( CORDB_PLATFORM_MAC_PPC + 1 ) ,
	CORDB_PLATFORM_WINDOWS_ARM	= ( CORDB_PLATFORM_MAC_X86 + 1 ) ,
	CORDB_PLATFORM_MAC_AMD64	= ( CORDB_PLATFORM_WINDOWS_ARM + 1 ) ,
	CORDB_PLATFORM_WINDOWS_ARM64	= ( CORDB_PLATFORM_MAC_AMD64 + 1 ) 
} 	CorDebugPlatform;

typedef ULONG64 CORDB_ADDRESS;

MIDL_INTERFACE("FE06DC28-49FB-4636-A4A3-E80DB4AE116C")
ICorDebugDataTarget : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetPlatform( 
		/* [out] */ CorDebugPlatform *pTargetPlatform) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE ReadVirtual( 
		/* [in] */ CORDB_ADDRESS address,
		/* [length_is][size_is][out] */ BYTE *pBuffer,
		/* [in] */ ULONG32 bytesRequested,
		/* [out] */ ULONG32 *pBytesRead) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetThreadContext( 
		/* [in] */ DWORD dwThreadID,
		/* [in] */ ULONG32 contextFlags,
		/* [in] */ ULONG32 contextSize,
		/* [size_is][out] */ BYTE *pContext) = 0;
	
};

class ICorDebugController : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Stop( 
		/* [in] */ DWORD dwTimeoutIgnored) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE Continue( 
		/* [in] */ BOOL fIsOutOfBand) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE IsRunning( 
		/* [out] */ BOOL *pbRunning) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE HasQueuedCallbacks( 
            /* [in] */ /*ICorDebugThread*/ void *pThread,
            /* [out] */ BOOL *pbQueued) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateThreads( 
            /* [out] */ /*ICorDebugThreadEnum*/ void **ppThreads) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetAllThreadsDebugState( 
		/* [in] */ /*CorDebugThreadState*/ int state,
		/* [in] */ /*ICorDebugThread*/ int *pExceptThisThread) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE Detach( void) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE Terminate( 
		/* [in] */ UINT exitCode) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE CanCommitChanges( 
		/* [in] */ ULONG cSnapshots,
		/* [size_is][in] */ /*ICorDebugEditAndContinueSnapshot*/ void *pSnapshots[  ],
		/* [out] */ /*ICorDebugErrorInfoEnum*/ void **pError) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE CommitChanges( 
		/* [in] */ ULONG cSnapshots,
		/* [size_is][in] */ /*ICorDebugEditAndContinueSnapshot*/ void *pSnapshots[  ],
		/* [out] */ /*ICorDebugErrorInfoEnum*/ void **pError) = 0;
	
};

class ICorDebugProcess : public ICorDebugController
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetID( 
		/* [out] */ DWORD *pdwProcessId) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetHandle( 
		/* [out] */ /*HPROCESS*/ void **phProcessHandle) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetThread( 
		/* [in] */ DWORD dwThreadId,
		/* [out] */ /*ICorDebugThread*/ int **ppThread) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateObjects( 
		/* [out] */ /*ICorDebugObjectEnum*/ int **ppObjects) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE IsTransitionStub( 
		/* [in] */ CORDB_ADDRESS address,
		/* [out] */ BOOL *pbTransitionStub) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE IsOSSuspended( 
		/* [in] */ DWORD threadID,
		/* [out] */ BOOL *pbSuspended) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetThreadContext( 
		/* [in] */ DWORD threadID,
		/* [in] */ ULONG32 contextSize,
		/* [size_is][length_is][out][in] */ BYTE context[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetThreadContext( 
		/* [in] */ DWORD threadID,
		/* [in] */ ULONG32 contextSize,
		/* [size_is][length_is][in] */ BYTE context[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE ReadMemory( 
		/* [in] */ CORDB_ADDRESS address,
		/* [in] */ DWORD size,
		/* [length_is][size_is][out] */ BYTE buffer[  ],
		/* [out] */ SIZE_T *read) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE WriteMemory( 
		/* [in] */ CORDB_ADDRESS address,
		/* [in] */ DWORD size,
		/* [size_is][in] */ BYTE buffer[  ],
		/* [out] */ SIZE_T *written) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE ClearCurrentException( 
		/* [in] */ DWORD threadID) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnableLogMessages( 
		/* [in] */ BOOL fOnOff) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE ModifyLogSwitch( 
		/* [annotation][in] */ 
		_In_  WCHAR *pLogSwitchName,
		/* [in] */ LONG lLevel) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateAppDomains( 
		/* [out] */ /*ICorDebugAppDomainEnum*/ int **ppAppDomains) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetObject( 
		/* [out] */ /*ICorDebugValue*/ int **ppObject) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE ThreadForFiberCookie( 
		/* [in] */ DWORD fiberCookie,
		/* [out] */ /*ICorDebugThread*/ int **ppThread) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetHelperThreadID( 
		/* [out] */ DWORD *pThreadID) = 0;
};

namespace xsfd
{
	class debug_lib_provider : public ICLRDebuggingLibraryProvider
	{
	public:
		debug_lib_provider(ICLRRuntimeInfo * rinfo_);
		~debug_lib_provider();

		virtual auto STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) -> HRESULT override;
		virtual auto STDMETHODCALLTYPE AddRef(void) -> ULONG override;
		virtual auto STDMETHODCALLTYPE Release(void) -> ULONG override;
		virtual auto STDMETHODCALLTYPE ProvideLibrary(const WCHAR *pwszFileName, DWORD dwTimestamp, DWORD dwSizeOfImage, HMODULE *phModule) -> HRESULT override;

	private:
		long ref_count;
		ICLRRuntimeInfo * rinfo;
	};

	class debug_data_target : public ICorDebugDataTarget
	{
	public:
		debug_data_target(HANDLE hnd_);
		~debug_data_target();

		virtual auto STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) -> HRESULT override;
		virtual auto STDMETHODCALLTYPE AddRef(void) -> ULONG override;
		virtual auto STDMETHODCALLTYPE Release(void) -> ULONG override;
		virtual auto STDMETHODCALLTYPE GetPlatform(CorDebugPlatform *pTargetPlatform) -> HRESULT override;
		virtual auto STDMETHODCALLTYPE ReadVirtual(CORDB_ADDRESS address, BYTE *pBuffer, ULONG32 bytesRequested, ULONG32 *pBytesRead) -> HRESULT override;
		virtual auto STDMETHODCALLTYPE GetThreadContext(DWORD dwThreadID, ULONG32 contextFlags, ULONG32 contextSize, BYTE *pContext) -> HRESULT override;

	private:
		long ref_count;
		HANDLE hnd;
	};
}