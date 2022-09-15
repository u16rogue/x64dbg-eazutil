#pragma once

// This serves as an alternative to some MS dotNet libraries

#include <Windows.h>
#include <cor.h>
#include <mscoree.h>
#include <metahost.h>
#include <clrdata.h>
//#include <cordebug.h>

namespace xsfd
{
	template <typename T>
	class ms_releasable
	{
		using self_t = ms_releasable<T>;

	public:
		ms_releasable()
			: instance(nullptr)
		{}

		ms_releasable(T *& i)
			// : instance(i)
		{
			instance = i;
			i = nullptr;
		}

		~ms_releasable()
		{
			instance->Release();
			instance = nullptr;
		}

		// Copy
		auto operator=(const self_t & rhs) -> self_t &
		{
			if (instance)
			{
				instance->Release();
				instance = nullptr;
			}

			if (rhs.instance->AddRef())
				instance = rhs.instance;
		}

		// Move
		auto operator=(self_t && rhs) -> self_t &
		{
			if (instance)
			{
				instance->Release();
				instance = nullptr;
			}

			instance = rhs.instance;
			return *this;
		}

		auto operator->() -> T *
		{
			return instance;
		}

		auto operator*() -> T *&
		{
			return instance;
		}

	private:
		T * instance;
	};
}

// TODO: move the rest of the library in here

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

// [31/07/2022] This cordebug.h is hastenly implemented. not even gonna bother.

typedef UINT32 mdToken;
typedef mdToken mdModule;
// typedef SIZE_T mdScope;
typedef mdToken mdTypeDef;
typedef mdToken mdSourceFile;
typedef mdToken mdMemberRef;
typedef mdToken mdMethodDef;
typedef mdToken mdFieldDef;
typedef mdToken mdSignature;
//typedef ULONG CorElementType;
//typedef SIZE_T PCCOR_SIGNATURE;
//typedef SIZE_T LPDEBUG_EVENT;
//typedef SIZE_T LPSTARTUPINFOW;
//typedef SIZE_T LPPROCESS_INFORMATION;
typedef const void *LPCVOID;
typedef /* [wire_marshal] */ void *HPROCESS;
typedef /* [wire_marshal] */ void *HTHREAD;
typedef UINT64 TASKID;
typedef DWORD CONNID;
typedef ULONG64 CORDB_REGISTER;
typedef DWORD CORDB_CONTINUE_STATUS;

typedef ULONG64 CORDB_ADDRESS;
class ICorDebugAssembly;
class ICorDebugModule;
class ICorDebugFunction;
class ICorDebugFunctionBreakpoint;
class ICorDebugClass;
class ICorDebugValue;
class ICorDebugChain;
class ICorDebugThread;
class ICorDebugFrame;
class ICorDebugObjectValue;
class ICorDebugType;
class ICorDebugAppDomainEnum;

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
		/* [out] */ ICorDebugAppDomainEnum **ppAppDomains) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetObject( 
		/* [out] */ /*ICorDebugValue*/ int **ppObject) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE ThreadForFiberCookie( 
		/* [in] */ DWORD fiberCookie,
		/* [out] */ /*ICorDebugThread*/ int **ppThread) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetHelperThreadID( 
		/* [out] */ DWORD *pThreadID) = 0;
};

typedef enum CorDebugGCType
{
	CorDebugWorkstationGC	= 0,
	CorDebugServerGC	= ( CorDebugWorkstationGC + 1 ) 
} 	CorDebugGCType;

typedef struct _COR_HEAPINFO
{
	BOOL areGCStructuresValid;
	DWORD pointerSize;
	DWORD numHeaps;
	BOOL concurrent;
	CorDebugGCType gcType;
} 	COR_HEAPINFO;

class ICorDebugEnum : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Skip( 
		/* [in] */ ULONG celt) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE Clone( 
		/* [out] */ ICorDebugEnum **ppEnum) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetCount( 
		/* [out] */ ULONG *pcelt) = 0;
	
};

typedef struct COR_TYPEID
{
    UINT64 token1;
    UINT64 token2;
} 	COR_TYPEID;

typedef struct _COR_HEAPOBJECT
{
    CORDB_ADDRESS address;
    ULONG64 size;
    COR_TYPEID type;
} 	COR_HEAPOBJECT;

class ICorDebugHeapEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ COR_HEAPOBJECT objects[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
};

class ICorDebugBreakpoint : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Activate( 
		/* [in] */ BOOL bActive) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE IsActive( 
		/* [out] */ BOOL *pbActive) = 0;
	
};

class ICorDebugValueBreakpoint : public ICorDebugBreakpoint
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetValue( 
		/* [out] */ ICorDebugValue **ppValue) = 0;
	
};

class ICorDebugValue : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetType( 
		/* [out] */ CorElementType *pType) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetSize( 
		/* [out] */ ULONG32 *pSize) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetAddress( 
		/* [out] */ CORDB_ADDRESS *pAddress) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE CreateBreakpoint( 
		/* [out] */ ICorDebugValueBreakpoint **ppBreakpoint) = 0;
	
};

class ICorDebugAssemblyEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ ICorDebugAssembly *values[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
};

class ICorDebugBreakpointEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ ICorDebugBreakpoint *breakpoints[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
};

typedef struct COR_DEBUG_STEP_RANGE
{
    ULONG32 startOffset;
    ULONG32 endOffset;
} 	COR_DEBUG_STEP_RANGE;

typedef enum CorDebugIntercept
{
	INTERCEPT_NONE	= 0,
	INTERCEPT_CLASS_INIT	= 0x1,
	INTERCEPT_EXCEPTION_FILTER	= 0x2,
	INTERCEPT_SECURITY	= 0x4,
	INTERCEPT_CONTEXT_POLICY	= 0x8,
	INTERCEPT_INTERCEPTION	= 0x10,
	INTERCEPT_ALL	= 0xffff
} 	CorDebugIntercept;

typedef enum CorDebugUnmappedStop
{
	STOP_NONE	= 0,
	STOP_PROLOG	= 0x1,
	STOP_EPILOG	= 0x2,
	STOP_NO_MAPPING_INFO	= 0x4,
	STOP_OTHER_UNMAPPED	= 0x8,
	STOP_UNMANAGED	= 0x10,
	STOP_ALL	= 0xffff
} 	CorDebugUnmappedStop;

class ICorDebugStepper : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE IsActive( 
		/* [out] */ BOOL *pbActive) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE Deactivate( void) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetInterceptMask( 
		/* [in] */ CorDebugIntercept mask) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetUnmappedStopMask( 
		/* [in] */ CorDebugUnmappedStop mask) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE Step( 
		/* [in] */ BOOL bStepIn) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE StepRange( 
		/* [in] */ BOOL bStepIn,
		/* [size_is][in] */ COR_DEBUG_STEP_RANGE ranges[  ],
		/* [in] */ ULONG32 cRangeCount) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE StepOut( void) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetRangeIL( 
		/* [in] */ BOOL bIL) = 0;
	
};

class ICorDebugStepperEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ ICorDebugStepper *steppers[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
};

class ICorDebugAppDomain : public ICorDebugController
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetProcess( 
		/* [out] */ ICorDebugProcess **ppProcess) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateAssemblies( 
		/* [out] */ ICorDebugAssemblyEnum **ppAssemblies) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetModuleFromMetaDataInterface( 
		/* [in] */ IUnknown *pIMetaData,
		/* [out] */ ICorDebugModule **ppModule) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateBreakpoints( 
		/* [out] */ ICorDebugBreakpointEnum **ppBreakpoints) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateSteppers( 
		/* [out] */ ICorDebugStepperEnum **ppSteppers) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE IsAttached( 
		/* [out] */ BOOL *pbAttached) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetName( 
		/* [in] */ ULONG32 cchName,
		/* [out] */ ULONG32 *pcchName,
		/* [length_is][size_is][out] */ WCHAR szName[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetObject( 
		/* [out] */ ICorDebugValue **ppObject) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE Attach( void) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetID( 
		/* [out] */ ULONG32 *pId) = 0;
	
};

class ICorDebugModuleEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ ICorDebugModule *modules[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
};

class ICorDebugAssembly : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetProcess( 
		/* [out] */ ICorDebugProcess **ppProcess) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetAppDomain( 
		/* [out] */ ICorDebugAppDomain **ppAppDomain) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateModules( 
		/* [out] */ ICorDebugModuleEnum **ppModules) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetCodeBase( 
		/* [in] */ ULONG32 cchName,
		/* [out] */ ULONG32 *pcchName,
		/* [length_is][size_is][out] */ WCHAR szName[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetName( 
		/* [in] */ ULONG32 cchName,
		/* [out] */ ULONG32 *pcchName,
		/* [length_is][size_is][out] */ WCHAR szName[  ]) = 0;
	
};

class ICorDebugModuleBreakpoint : public ICorDebugBreakpoint
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetModule( 
		/* [out] */ ICorDebugModule **ppModule) = 0;
	
};

typedef struct COR_DEBUG_IL_TO_NATIVE_MAP
{
    ULONG32 ilOffset;
    ULONG32 nativeStartOffset;
    ULONG32 nativeEndOffset;
} 	COR_DEBUG_IL_TO_NATIVE_MAP;

class ICorDebugCode : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE IsIL( 
		/* [out] */ BOOL *pbIL) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetFunction( 
		/* [out] */ ICorDebugFunction **ppFunction) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetAddress( 
		/* [out] */ CORDB_ADDRESS *pStart) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetSize( 
		/* [out] */ ULONG32 *pcBytes) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE CreateBreakpoint( 
		/* [in] */ ULONG32 offset,
		/* [out] */ ICorDebugFunctionBreakpoint **ppBreakpoint) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetCode( 
		/* [in] */ ULONG32 startOffset,
		/* [in] */ ULONG32 endOffset,
		/* [in] */ ULONG32 cBufferAlloc,
		/* [length_is][size_is][out] */ BYTE buffer[  ],
		/* [out] */ ULONG32 *pcBufferSize) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetVersionNumber( 
		/* [out] */ ULONG32 *nVersion) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetILToNativeMapping( 
		/* [in] */ ULONG32 cMap,
		/* [out] */ ULONG32 *pcMap,
		/* [length_is][size_is][out] */ COR_DEBUG_IL_TO_NATIVE_MAP map[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetEnCRemapSequencePoints( 
		/* [in] */ ULONG32 cMap,
		/* [out] */ ULONG32 *pcMap,
		/* [length_is][size_is][out] */ ULONG32 offsets[  ]) = 0;
	
};

class ICorDebugFunctionBreakpoint : public ICorDebugBreakpoint
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetFunction( 
		/* [out] */ ICorDebugFunction **ppFunction) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetOffset( 
		/* [out] */ ULONG32 *pnOffset) = 0;
	
};

class ICorDebugFunction : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetModule( 
		/* [out] */ ICorDebugModule **ppModule) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetClass( 
		/* [out] */ ICorDebugClass **ppClass) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetToken( 
		/* [out] */ mdMethodDef *pMethodDef) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetILCode( 
		/* [out] */ ICorDebugCode **ppCode) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetNativeCode( 
		/* [out] */ ICorDebugCode **ppCode) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE CreateBreakpoint( 
		/* [out] */ ICorDebugFunctionBreakpoint **ppBreakpoint) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetLocalVarSigToken( 
		/* [out] */ mdSignature *pmdSig) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetCurrentVersionNumber( 
		/* [out] */ ULONG32 *pnCurrentVersion) = 0;
	
};

typedef struct _COR_IL_MAP
{
	ULONG32 oldOffset;
	ULONG32 newOffset;
	BOOL fAccurate;
} 	COR_IL_MAP;

class ICorDebugEditAndContinueSnapshot : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE CopyMetaData( 
		/* [in] */ IStream *pIStream,
		/* [out] */ GUID *pMvid) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetMvid( 
		/* [out] */ GUID *pMvid) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetRoDataRVA( 
		/* [out] */ ULONG32 *pRoDataRVA) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetRwDataRVA( 
		/* [out] */ ULONG32 *pRwDataRVA) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetPEBytes( 
		/* [in] */ IStream *pIStream) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetILMap( 
		/* [in] */ mdToken mdFunction,
		/* [in] */ ULONG cMapSize,
		/* [size_is][in] */ COR_IL_MAP map[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetPESymbolBytes( 
		/* [in] */ IStream *pIStream) = 0;
	
};

class ICorDebugModule : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetProcess( 
		/* [out] */ ICorDebugProcess **ppProcess) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetBaseAddress( 
		/* [out] */ CORDB_ADDRESS *pAddress) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetAssembly( 
		/* [out] */ ICorDebugAssembly **ppAssembly) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetName( 
		/* [in] */ ULONG32 cchName,
		/* [out] */ ULONG32 *pcchName,
		/* [length_is][size_is][out] */ WCHAR szName[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnableJITDebugging( 
		/* [in] */ BOOL bTrackJITInfo,
		/* [in] */ BOOL bAllowJitOpts) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnableClassLoadCallbacks( 
		/* [in] */ BOOL bClassLoadCallbacks) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetFunctionFromToken( 
		/* [in] */ mdMethodDef methodDef,
		/* [out] */ ICorDebugFunction **ppFunction) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetFunctionFromRVA( 
		/* [in] */ CORDB_ADDRESS rva,
		/* [out] */ ICorDebugFunction **ppFunction) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetClassFromToken( 
		/* [in] */ mdTypeDef typeDef,
		/* [out] */ ICorDebugClass **ppClass) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE CreateBreakpoint( 
		/* [out] */ ICorDebugModuleBreakpoint **ppBreakpoint) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetEditAndContinueSnapshot( 
		/* [out] */ ICorDebugEditAndContinueSnapshot **ppEditAndContinueSnapshot) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetMetaDataInterface( 
		/* [in] */ REFIID riid,
		/* [out] */ IUnknown **ppObj) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetToken( 
		/* [out] */ mdModule *pToken) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE IsDynamic( 
		/* [out] */ BOOL *pDynamic) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetGlobalVariableValue( 
		/* [in] */ mdFieldDef fieldDef,
		/* [out] */ ICorDebugValue **ppValue) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetSize( 
		/* [out] */ ULONG32 *pcBytes) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE IsInMemory( 
		/* [out] */ BOOL *pInMemory) = 0;
	
};

typedef 
enum CorDebugThreadState
{
	THREAD_RUN	= 0,
	THREAD_SUSPEND	= ( THREAD_RUN + 1 ) 
} 	CorDebugThreadState;

typedef enum CorDebugUserState
{
	USER_STOP_REQUESTED	= 0x1,
	USER_SUSPEND_REQUESTED	= 0x2,
	USER_BACKGROUND	= 0x4,
	USER_UNSTARTED	= 0x8,
	USER_STOPPED	= 0x10,
	USER_WAIT_SLEEP_JOIN	= 0x20,
	USER_SUSPENDED	= 0x40,
	USER_UNSAFE_POINT	= 0x80,
	USER_THREADPOOL	= 0x100
} 	CorDebugUserState;

class ICorDebugChainEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ ICorDebugChain *chains[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
};

class ICorDebugRegisterSet : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetRegistersAvailable( 
		/* [out] */ ULONG64 *pAvailable) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetRegisters( 
		/* [in] */ ULONG64 mask,
		/* [in] */ ULONG32 regCount,
		/* [length_is][size_is][out] */ CORDB_REGISTER regBuffer[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetRegisters( 
		/* [in] */ ULONG64 mask,
		/* [in] */ ULONG32 regCount,
		/* [size_is][in] */ CORDB_REGISTER regBuffer[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetThreadContext( 
		/* [in] */ ULONG32 contextSize,
		/* [size_is][length_is][out][in] */ BYTE context[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetThreadContext( 
		/* [in] */ ULONG32 contextSize,
		/* [size_is][length_is][in] */ BYTE context[  ]) = 0;
	
};

class ICorDebugEval : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE CallFunction( 
		/* [in] */ ICorDebugFunction *pFunction,
		/* [in] */ ULONG32 nArgs,
		/* [size_is][in] */ ICorDebugValue *ppArgs[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE NewObject( 
		/* [in] */ ICorDebugFunction *pConstructor,
		/* [in] */ ULONG32 nArgs,
		/* [size_is][in] */ ICorDebugValue *ppArgs[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE NewObjectNoConstructor( 
		/* [in] */ ICorDebugClass *pClass) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE NewString( 
		/* [in] */ LPCWSTR string) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE NewArray( 
		/* [in] */ CorElementType elementType,
		/* [in] */ ICorDebugClass *pElementClass,
		/* [in] */ ULONG32 rank,
		/* [size_is][in] */ ULONG32 dims[  ],
		/* [size_is][in] */ ULONG32 lowBounds[  ]) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE IsActive( 
		/* [out] */ BOOL *pbActive) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE Abort( void) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetResult( 
		/* [out] */ ICorDebugValue **ppResult) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetThread( 
		/* [out] */ ICorDebugThread **ppThread) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE CreateValue( 
		/* [in] */ CorElementType elementType,
		/* [in] */ ICorDebugClass *pElementClass,
		/* [out] */ ICorDebugValue **ppValue) = 0;
	
};

class ICorDebugThread : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetProcess( 
		/* [out] */ ICorDebugProcess **ppProcess) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetID( 
		/* [out] */ DWORD *pdwThreadId) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetHandle( 
		/* [out] */ HTHREAD *phThreadHandle) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetAppDomain( 
		/* [out] */ ICorDebugAppDomain **ppAppDomain) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetDebugState( 
		/* [in] */ CorDebugThreadState state) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetDebugState( 
		/* [out] */ CorDebugThreadState *pState) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetUserState( 
		/* [out] */ CorDebugUserState *pState) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetCurrentException( 
		/* [out] */ ICorDebugValue **ppExceptionObject) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE ClearCurrentException( void) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE CreateStepper( 
		/* [out] */ ICorDebugStepper **ppStepper) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateChains( 
		/* [out] */ ICorDebugChainEnum **ppChains) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetActiveChain( 
		/* [out] */ ICorDebugChain **ppChain) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetActiveFrame( 
		/* [out] */ ICorDebugFrame **ppFrame) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetRegisterSet( 
		/* [out] */ ICorDebugRegisterSet **ppRegisters) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE CreateEval( 
		/* [out] */ ICorDebugEval **ppEval) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetObject( 
		/* [out] */ ICorDebugValue **ppObject) = 0;
	
};

/*
class ICorDebugContext : public ICorDebugObjectValue
{
public:
};
*/

class ICorDebugFrameEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ ICorDebugFrame *frames[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
};

typedef enum CorDebugChainReason
{
	CHAIN_NONE	= 0,
	CHAIN_CLASS_INIT	= 0x1,
	CHAIN_EXCEPTION_FILTER	= 0x2,
	CHAIN_SECURITY	= 0x4,
	CHAIN_CONTEXT_POLICY	= 0x8,
	CHAIN_INTERCEPTION	= 0x10,
	CHAIN_PROCESS_START	= 0x20,
	CHAIN_THREAD_START	= 0x40,
	CHAIN_ENTER_MANAGED	= 0x80,
	CHAIN_ENTER_UNMANAGED	= 0x100,
	CHAIN_DEBUGGER_EVAL	= 0x200,
	CHAIN_CONTEXT_SWITCH	= 0x400,
	CHAIN_FUNC_EVAL	= 0x800
} 	CorDebugChainReason;

class ICorDebugChain : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetThread( 
		/* [out] */ ICorDebugThread **ppThread) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetStackRange( 
		/* [out] */ CORDB_ADDRESS *pStart,
		/* [out] */ CORDB_ADDRESS *pEnd) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetContext( 
		/* [out] */ /*ICorDebugContext*/void **ppContext) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetCaller( 
		/* [out] */ ICorDebugChain **ppChain) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetCallee( 
		/* [out] */ ICorDebugChain **ppChain) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetPrevious( 
		/* [out] */ ICorDebugChain **ppChain) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetNext( 
		/* [out] */ ICorDebugChain **ppChain) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE IsManaged( 
		/* [out] */ BOOL *pManaged) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateFrames( 
		/* [out] */ ICorDebugFrameEnum **ppFrames) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetActiveFrame( 
		/* [out] */ ICorDebugFrame **ppFrame) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetRegisterSet( 
		/* [out] */ ICorDebugRegisterSet **ppRegisters) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetReason( 
		/* [out] */ CorDebugChainReason *pReason) = 0;
	
};

class ICorDebugFrame : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetChain( 
		/* [out] */ ICorDebugChain **ppChain) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetCode( 
		/* [out] */ ICorDebugCode **ppCode) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetFunction( 
		/* [out] */ ICorDebugFunction **ppFunction) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetFunctionToken( 
		/* [out] */ mdMethodDef *pToken) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetStackRange( 
		/* [out] */ CORDB_ADDRESS *pStart,
		/* [out] */ CORDB_ADDRESS *pEnd) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetCaller( 
		/* [out] */ ICorDebugFrame **ppFrame) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetCallee( 
		/* [out] */ ICorDebugFrame **ppFrame) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE CreateStepper( 
		/* [out] */ ICorDebugStepper **ppStepper) = 0;
	
};

class ICorDebugClass : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetModule( 
		/* [out] */ ICorDebugModule **pModule) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetToken( 
		/* [out] */ mdTypeDef *pTypeDef) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetStaticFieldValue( 
		/* [in] */ mdFieldDef fieldDef,
		/* [in] */ ICorDebugFrame *pFrame,
		/* [out] */ ICorDebugValue **ppValue) = 0;
	
};

class ICorDebugObjectValue : public ICorDebugValue
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetClass( 
		/* [out] */ ICorDebugClass **ppClass) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetFieldValue( 
		/* [in] */ ICorDebugClass *pClass,
		/* [in] */ mdFieldDef fieldDef,
		/* [out] */ ICorDebugValue **ppValue) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetVirtualMethod( 
		/* [in] */ mdMemberRef memberRef,
		/* [out] */ ICorDebugFunction **ppFunction) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetContext( 
		/* [out] */ /*ICorDebugContext*/void **ppContext) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE IsValueClass( 
		/* [out] */ BOOL *pbIsValueClass) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetManagedCopy( 
		/* [out] */ IUnknown **ppObject) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE SetFromManagedCopy( 
		/* [in] */ IUnknown *pObject) = 0;
	
};

typedef enum CorDebugGenerationTypes
{
	CorDebug_Gen0	= 0,
	CorDebug_Gen1	= 1,
	CorDebug_Gen2	= 2,
	CorDebug_LOH	= 3
} 	CorDebugGenerationTypes;

typedef struct _COR_SEGMENT
{
    CORDB_ADDRESS start;
    CORDB_ADDRESS end;
    CorDebugGenerationTypes type;
    ULONG heap;
} 	COR_SEGMENT;

class ICorDebugHeapSegmentEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ COR_SEGMENT segments[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
};

typedef enum CorGCReferenceType
{
	CorHandleStrong	= ( 1 << 0 ) ,
	CorHandleStrongPinning	= ( 1 << 1 ) ,
	CorHandleWeakShort	= ( 1 << 2 ) ,
	CorHandleWeakLong	= ( 1 << 3 ) ,
	CorHandleWeakRefCount	= ( 1 << 4 ) ,
	CorHandleStrongRefCount	= ( 1 << 5 ) ,
	CorHandleStrongDependent	= ( 1 << 6 ) ,
	CorHandleStrongAsyncPinned	= ( 1 << 7 ) ,
	CorHandleStrongSizedByref	= ( 1 << 8 ) ,
	CorHandleWeakWinRT	= ( 1 << 9 ) ,
	CorReferenceStack	= 0x80000001,
	CorReferenceFinalizer	= 80000002,
	CorHandleStrongOnly	= 0x1e3,
	CorHandleWeakOnly	= 0x21c,
	CorHandleAll	= 0x7fffffff
} 	CorGCReferenceType;

typedef struct COR_GC_REFERENCE
{
    ICorDebugAppDomain *Domain;
    ICorDebugValue *Location;
    CorGCReferenceType Type;
    UINT64 ExtraData;
} 	COR_GC_REFERENCE;

class ICorDebugGCReferenceEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ COR_GC_REFERENCE roots[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
};

class ICorDebugTypeEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ ICorDebugType *values[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
};

class ICorDebugType : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetType( 
		/* [out] */ CorElementType *ty) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetClass( 
		/* [out] */ ICorDebugClass **ppClass) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateTypeParameters( 
		/* [out] */ ICorDebugTypeEnum **ppTyParEnum) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetFirstTypeParameter( 
		/* [out] */ ICorDebugType **value) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetBase( 
		/* [out] */ ICorDebugType **pBase) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetStaticFieldValue( 
		/* [in] */ mdFieldDef fieldDef,
		/* [in] */ ICorDebugFrame *pFrame,
		/* [out] */ ICorDebugValue **ppValue) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetRank( 
		/* [out] */ ULONG32 *pnRank) = 0;
	
};

typedef struct COR_ARRAY_LAYOUT
{
	COR_TYPEID componentID;
	CorElementType componentType;
	ULONG32 firstElementOffset;
	ULONG32 elementSize;
	ULONG32 countOffset;
	ULONG32 rankSize;
	ULONG32 numRanks;
	ULONG32 rankOffset;
} 	COR_ARRAY_LAYOUT;

typedef struct COR_TYPE_LAYOUT
{
	COR_TYPEID parentID;
	ULONG32 objectSize;
	ULONG32 numFields;
	ULONG32 boxOffset;
	CorElementType type;
} 	COR_TYPE_LAYOUT;

typedef struct COR_FIELD
{
	mdFieldDef token;
	ULONG32 offset;
	COR_TYPEID id;
	CorElementType fieldType;
} 	COR_FIELD;

typedef 
enum CorDebugNGENPolicy
{
	DISABLE_LOCAL_NIC	= 1
} 	CorDebugNGENPolicy;

class ICorDebugProcess5 : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetGCHeapInformation( 
		/* [out] */ COR_HEAPINFO *pHeapInfo) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateHeap( 
		/* [out] */ ICorDebugHeapEnum **ppObjects) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateHeapRegions( 
		/* [out] */ ICorDebugHeapSegmentEnum **ppRegions) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetObject( 
		/* [in] */ CORDB_ADDRESS addr,
		/* [out] */ ICorDebugObjectValue **pObject) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateGCReferences( 
		/* [in] */ BOOL enumerateWeakReferences,
		/* [out] */ ICorDebugGCReferenceEnum **ppEnum) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnumerateHandles( 
		/* [in] */ CorGCReferenceType types,
		/* [out] */ ICorDebugGCReferenceEnum **ppEnum) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetTypeID( 
		/* [in] */ CORDB_ADDRESS obj,
		/* [out] */ COR_TYPEID *pId) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetTypeForTypeID( 
		/* [in] */ COR_TYPEID id,
		/* [out] */ ICorDebugType **ppType) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetArrayLayout( 
		/* [in] */ COR_TYPEID id,
		/* [out] */ COR_ARRAY_LAYOUT *pLayout) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetTypeLayout( 
		/* [in] */ COR_TYPEID id,
		/* [out] */ COR_TYPE_LAYOUT *pLayout) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE GetTypeFields( 
		/* [in] */ COR_TYPEID id,
		ULONG32 celt,
		COR_FIELD fields[  ],
		ULONG32 *pceltNeeded) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EnableNGENPolicy( 
		/* [in] */ CorDebugNGENPolicy ePolicy) = 0;
	
};

class ICorDebugAppDomainEnum : public ICorDebugEnum
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( 
		/* [in] */ ULONG celt,
		/* [length_is][size_is][out] */ ICorDebugAppDomain *values[  ],
		/* [out] */ ULONG *pceltFetched) = 0;
	
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