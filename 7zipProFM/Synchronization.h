#pragma once
#include "afxmt.h"


#include "C/7zTypes.h"


class CBaseEvent :
	public CEvent
{
	DECLARE_DYNAMIC(CBaseEvent)

// Constructor
public:
	explicit CBaseEvent(BOOL bInitiallyOwn = FALSE, BOOL bManualReset = FALSE,
		LPCTSTR lpszNAme = NULL, LPSECURITY_ATTRIBUTES lpsaAttribute = NULL)
		: CEvent(bInitiallyOwn, bManualReset, lpszNAme, lpsaAttribute) {}

// Operations
public:
	BOOL IsCreated() { return m_hObject != 0; }

	BOOL Set() { return SetEvent(); }
	BOOL Reset() { return ResetEvent(); }

// Implementation
public:
	virtual ~CBaseEvent() {}
};


class CManualResetEvent : public CBaseEvent
{
	DECLARE_DYNAMIC(CManualResetEvent)

// Constructor
public:
	explicit CManualResetEvent(BOOL bInitiallyOwn = FALSE,
		LPCTSTR lpszNAme = NULL)
		: CBaseEvent(bInitiallyOwn, TRUE, lpszNAme) {}

// Operations
public:

// Implementation
public:
	virtual ~CManualResetEvent() {}
};


class CAutoResetEvent : public CBaseEvent
{
	DECLARE_DYNAMIC(CAutoResetEvent)

// Constructor
public:
	explicit CAutoResetEvent()
		: CBaseEvent(FALSE, FALSE) {}

// Operations
public:

// Implementation
public:
	virtual ~CAutoResetEvent() {}
};

typedef
#ifdef UNDER_CE
DWORD
#else
unsigned
#endif
THREAD_FUNC_RET_TYPE;

#define THREAD_FUNC_CALL_TYPE MY_STD_CALL
#define THREAD_FUNC_DECL THREAD_FUNC_RET_TYPE THREAD_FUNC_CALL_TYPE
typedef THREAD_FUNC_RET_TYPE(THREAD_FUNC_CALL_TYPE * THREAD_FUNC_TYPE)(void *);

