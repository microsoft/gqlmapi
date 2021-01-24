// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"

namespace graphql::mapi {

Session::Session(bool useDefaultProfile)
{
	// Initialize and logon to the MAPI profile.
	MAPIINIT_0 MAPIINIT { 0, 0 };

	CORt(MAPIInitialize(&MAPIINIT));
	CORt(MAPILogonEx(NULL,
		nullptr,
		nullptr,
		MAPI_EXTENDED | MAPI_UNICODE | MAPI_LOGON_UI | (useDefaultProfile ? MAPI_USE_DEFAULT : 0),
		&m_session));
}

Session::~Session()
{
	// Release all of the MAPI objects we opened and free any MAPI memory allocations.
	m_session.Release();

	// Shutdown MAPI.
	MAPIUninitialize();
}

const CComPtr<IMAPISession>& Session::session() const
{
	return m_session;
}

} // namespace graphql::mapi