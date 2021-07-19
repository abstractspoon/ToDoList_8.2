#if !defined(AFX_TDLCUSTOMATTRIBUTEDLG_H__E0C8D9C7_40BA_4571_B412_C8A5491050FB__INCLUDED_)
#define AFX_TDLCUSTOMATTRIBUTEDLG_H__E0C8D9C7_40BA_4571_B412_C8A5491050FB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TDLCustomAttributeDlg.h : header file
//

#include "tdcstruct.h"
#include "todoctrl.h"
#include "tdcimagelist.h"
#include "TDLCustomAttribFeatureComboBox.h"
#include "TDLDialog.h"

#include "..\shared\fileedit.h"
#include "..\shared\entoolbar.h"
#include "..\shared\toolbarhelper.h"
#include "..\shared\menubutton.h"
#include "..\shared\iconbutton.h"
#include "..\shared\checklistboxex.h"

#include "..\Interfaces\uithemefile.h"

/////////////////////////////////////////////////////////////////////////////
// CCustomAttributeListPage dialog

class CCustomAttributeListPage : public CDialog
{
// Construction
public:
	CCustomAttributeListPage(const CToDoCtrl& tdc);

	BOOL Create(CWnd* pParent);

	BOOL SetDataType(DWORD dwDataType);
	BOOL SetListType(DWORD dwListType);
	void SetDefaultListData(const  CStringArray& aData);

	DWORD GetListType() const { return m_dwListType; }
	int GetDefaultListData(CStringArray& aData) const;
	
	static BOOL BuildSymbolPopupMenu(CMenu& menu);

protected:
// Dialog Data
	//{{AFX_DATA(CCustomAttributeListPage)
	CComboBox	m_cbListType;
	CString	m_sDefaultListData;
	//}}AFX_DATA
	CMenuButton	m_btInsertSymbol;
	CMaskEdit	m_eListData;
	DWORD m_dwListType, m_dwDataType;
	CIconButton m_btBrowseImages;

	const CToDoCtrl& m_tdc;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCustomAttributeListPage)
protected:
	//}}AFX_VIRTUAL
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCustomAttributeListPage)
	afx_msg void OnSelchangeListtype();
	afx_msg void OnChangeDefaultlistdata();
	afx_msg void OnBrowseimages();
	afx_msg void OnInsertsymbol();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void BuildListCombo();
	void UpdateListDataMask();
	void EnableControls();

};

/////////////////////////////////////////////////////////////////////////////
// CCustomAttributeCalcPage dialog

class CCustomAttributeCalcPage : public CDialog
{
// Construction
public:
	CCustomAttributeCalcPage();

	BOOL Create(CWnd* pParent);

	// TODO

protected:
// Dialog Data
	//{{AFX_DATA(CTDLCustomAttributeDlg)
	CComboBox	m_cbFirstOperand;
	CComboBox	m_cbOperators;
	CComboBox	m_cbSecondOperand;
	//}}AFX_DATA
	CString m_sFirstOperand;
	CString m_sOperator;
	CString m_sSecondOperand;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCustomAttributeCalcPage)
protected:
	//}}AFX_VIRTUAL
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCustomAttributeCalcPage)
	afx_msg void OnSelChangeFirstOperand();
	afx_msg void OnSelChangeOperator();
	afx_msg void OnSelChangeSecondOperand();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void BuildFirstOperandCombo();
	void BuildOperatorCombo();
	void BuildSecondOperandCombo();
	void EnableControls();

};

/////////////////////////////////////////////////////////////////////////////
// CTDLCustomAttributeDlg dialog

class CTDLCustomAttributeDlg : public CTDLDialog
{
// Construction
public:
	CTDLCustomAttributeDlg(const CToDoCtrl& tdc, 
							CWnd* pParent = NULL);   // standard constructor

	int GetAttributes(CTDCCustomAttribDefinitionArray& aAttrib) const;

protected:
// Dialog Data
	//{{AFX_DATA(CTDLCustomAttributeDlg)
	enum { IDD = IDD_ADDCUSTOMATTRIB_DIALOG };
	CString	m_sTaskFile;
	CString	m_sColumnTitle;
	int		m_nAlignment;
	CString	m_sUniqueID;
	//}}AFX_DATA
	DWORD m_dwDataType;
	DWORD m_dwFeatures;

	CTDCCustomAttribDefinitionArray m_aAttrib;

	CListCtrl	m_lcAttributes;
	CMaskEdit	m_eUniqueID;
	CComboBox	m_cbDataType;
	CComboBox	m_cbAlign;
	CTDLCustomAttribFeatureComboBox	m_cbFeatures;
	CEnToolBar m_toolbar;
	CToolbarHelper m_tbHelper;
	CFileEdit	m_eTaskfile;
	CEnEdit		m_eColumnTitle;

	CCustomAttributeListPage m_pageList;
	CCustomAttributeCalcPage m_pageCalc;

	const CToDoCtrl& m_tdc;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTDLCustomAttributeDlg)
protected:
	//}}AFX_VIRTUAL
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual BOOL OnInitDialog();

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTDLCustomAttributeDlg)
	afx_msg void OnItemchangedAttriblist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDoubleClickItem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeDatatype();
	afx_msg void OnSelchangeAlignment();
	afx_msg void OnSelchangeListtype();
	afx_msg void OnChangeColumntitle();
	afx_msg void OnChangeDefaultlistdata();
	afx_msg void OnEndlabeleditAttributelist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginlabeleditAttributelist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeUniqueid();
	afx_msg void OnImport();
	afx_msg void OnClickAttributelist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNewAttribute();
	afx_msg void OnUpdateNewAttribute(CCmdUI* pCmdUI);
	afx_msg void OnDeleteAttribute();
	afx_msg void OnUpdateDeleteAttribute(CCmdUI* pCmdUI);
	afx_msg void OnMoveAttributeDown();
	afx_msg void OnUpdateMoveAttributeDown(CCmdUI* pCmdUI);
	afx_msg void OnMoveAttributeUp();
	afx_msg void OnUpdateMoveAttributeUp(CCmdUI* pCmdUI);
	afx_msg void OnChangeFeatures();
	//}}AFX_MSG
	afx_msg void OnUpdateEditAttribute(CCmdUI* pCmdUI);
	afx_msg void OnEditAttribute();
	afx_msg LRESULT OnEEClick(WPARAM wp, LPARAM lp);
	DECLARE_MESSAGE_MAP()

	void BuildDataTypeCombo();
	BOOL AddAttributeToListCtrl(const TDCCUSTOMATTRIBUTEDEFINITION& attrib, BOOL bNew, int nPos = -1);
	void EnableControls();
	int GetCurSel();
	BOOL UniqueIDExists(const CString& sID, int nIgnore = -1) const;
	void MakeUniqueID(CString& sID, int nIgnore = -1) const;
	BOOL InitializeToolbar();
	void MoveAttribute(int nRows = 1);

	static CString FormatFeatureList(DWORD dwFeatures);
	static void GetTypeStrings(const TDCCUSTOMATTRIBUTEDEFINITION& attrib, CString& sDataType, CString& sListType);
	static CString MakeID(const CString& sLabel);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDLCUSTOMATTRIBUTEDLG_H__E0C8D9C7_40BA_4571_B412_C8A5491050FB__INCLUDED_)
