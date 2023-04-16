//By: George Polouse

// Tables.cpp: implementation of the COdbcFields class.
//

#include "stdafx.h"
#include "odbcFields.h"

#include <atlconv.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction

IMPLEMENT_DYNAMIC(COdbcFields, CRecordsetEx)

COdbcFields::COdbcFields(CDatabase* pDatabase) : CRecordsetEx(pDatabase)
{
}

BOOL COdbcFields::Open(LPCTSTR pszTableName, LPCTSTR pszTableQualifier,
	LPCTSTR pszTableOwner, LPCTSTR pszTableType, UINT nOpenType)
{
	USES_CONVERSION;

	RETCODE nRetCode;
	UWORD   bFunctionExists;

	ASSERT(pszTableName && *pszTableName);

	if ((pszTableName == NULL) || (pszTableName[0] == 0))
		return FALSE;

	// Make sure SQLTables exists
	AFX_SQL_SYNC(::SQLGetFunctions(m_pDatabase->m_hdbc,
		SQL_API_SQLCOLUMNS,&bFunctionExists));

	if(!Check(nRetCode))
		AfxThrowDBException(nRetCode, m_pDatabase, m_hstmt);

	if(!bFunctionExists)
		throw _T("<::SQLColumns> not supported.");

	// Cache state info and allocate hstmt
	SetState(nOpenType,NULL,readOnly);

	if (!AllocHstmt())
		return FALSE;

	TRY
	{
		OnSetOptions(m_hstmt);
		AllocStatusArrays();

		// Call the ODBC function
#if _MSC_VER >= 1400
		AFX_ODBC_CALL(::SQLColumns(m_hstmt,
			(SQLWCHAR*)T2W((LPTSTR)pszTableQualifier),SQL_NTS,
			(SQLWCHAR*)T2W((LPTSTR)pszTableOwner),SQL_NTS,
			(SQLWCHAR*)T2W((LPTSTR)pszTableName),SQL_NTS,
			(SQLWCHAR*)T2W((LPTSTR)pszTableType),SQL_NTS));
#else
		AFX_ODBC_CALL(::SQLColumns(m_hstmt,
			(SQLCHAR*)T2A((LPTSTR)pszTableQualifier),SQL_NTS,
			(SQLCHAR*)T2A((LPTSTR)pszTableOwner),SQL_NTS,
			(SQLCHAR*)T2A((LPTSTR)pszTableName),SQL_NTS,
			(SQLCHAR*)T2A((LPTSTR)pszTableType),SQL_NTS));
#endif

		if (!Check(nRetCode))
			ThrowDBException(nRetCode,m_hstmt);

		// Allocate memory and cache info
		AllocAndCacheFieldInfo();
		AllocRowset();

		// Fetch the first row of data
		MoveNext();

		// If EOF, result set is empty, set BOF as well
		m_bBOF = m_bEOF;

	}

	CATCH_ALL(e)
	{
		Close();
		THROW_LAST();
	}
	END_CATCH_ALL

	return TRUE;
}

int COdbcFields::GetFieldNames(CStringArray& aFieldNames)
{
	aFieldNames.RemoveAll();

	ASSERT(IsOpen());

 	if (IsOpen() && !IsBOF())
	{
		while (!IsEOF())
		{
			aFieldNames.Add(GetFieldName());
			MoveNext();			
		}
	}

	return aFieldNames.GetSize();
}

// static version
int COdbcFields::GetFieldNames(CDatabase& db, const CString& sTableName, CStringArray& aFieldNames)
{
	COdbcFields fields(&db);

	if (fields.Open(sTableName))
		return fields.GetFieldNames(aFieldNames);

	// else
	ASSERT(0);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// COdbcFields diagnostics

#ifdef _DEBUG
void COdbcFields::AssertValid() const
{
	CRecordsetEx::AssertValid();
}

void COdbcFields::Dump(CDumpContext& dc) const
{
	CRecordsetEx::Dump(dc);
}
#endif //_DEBUG