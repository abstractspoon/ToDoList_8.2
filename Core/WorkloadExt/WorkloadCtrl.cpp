// WorkloadTreeList.cpp: implementation of the CWorkloadTreeList class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "WorkloadExt.h" // for WORKLOAD_TYPEID
#include "WorkloadCtrl.h"
#include "WorkloadMsg.h"
#include "WorkloadStatic.h"

#include "..\shared\DialogHelper.h"
#include "..\shared\DateHelper.h"
#include "..\shared\timeHelper.h"
#include "..\shared\holdredraw.h"
#include "..\shared\graphicsMisc.h"
#include "..\shared\autoflag.h"
#include "..\shared\misc.h"
#include "..\shared\enstring.h"
#include "..\shared\localizer.h"
#include "..\shared\enbitmap.h"
#include "..\shared\copywndcontents.h"
#include "..\shared\CopyWndContents.h"

#include "..\3rdparty\shellicons.h"

#include <float.h> // for DBL_MAX
#include <math.h>  // for fabs()

//////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////

#ifndef CDRF_SKIPPOSTPAINT
#	define CDRF_SKIPPOSTPAINT	(0x00000100)
#endif

#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER 0x00010000
#endif

//////////////////////////////////////////////////////////////////////

const int LV_COLPADDING			= GraphicsMisc::ScaleByDPIFactor(3);
const int HD_COLPADDING			= GraphicsMisc::ScaleByDPIFactor(6);

//////////////////////////////////////////////////////////////////////

#define GET_WI_RET(id, wi, ret)	\
{								\
	if (id == 0) return ret;	\
	wi = GetWorkloadItem(id);		\
	ASSERT(wi);					\
	if (wi == NULL) return ret;	\
}

#define GET_WI(id, wi)		\
{							\
	if (id == 0) return;	\
	wi = GetWorkloadItem(id);	\
	ASSERT(wi);				\
	if (wi == NULL)	return;	\
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWorkloadCtrl::CWorkloadCtrl() 
	:
	m_dwOptions(WLCF_SHOWSPLITTERBAR),
	m_crAllocation(RGB(255, 180, 180)),
	m_crOverlap(RGB(255, 0, 0)),
	m_dwMaxTaskID(0),
	m_mapTotalDays(FALSE),
	m_mapTotalTasks(FALSE),
	m_mapPercentLoad(TRUE), // average
	m_barChart(m_aAllocTo, m_mapPercentLoad),
	m_dtPeriod(DHD_BEGINTHISMONTH, DHD_ENDTHISMONTH, TRUE)
{
}

CWorkloadCtrl::~CWorkloadCtrl()
{
}

BEGIN_MESSAGE_MAP(CWorkloadCtrl, CTreeListCtrl)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_TASKTREE, OnBeginEditTreeLabel)
	ON_NOTIFY(HDN_ITEMCLICK, IDC_TASKHEADER, OnClickTreeHeader)
	ON_NOTIFY(TVN_GETDISPINFO, IDC_TASKTREE, OnTreeGetDispInfo)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_TOTALSLABELS, OnTotalsListsCustomDraw)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_ALLOCATIONTOTALS, OnTotalsListsCustomDraw)

	ON_WM_CREATE()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


int CWorkloadCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// prevent column reordering on list
	m_listHeader.ModifyStyle(HDS_DRAGDROP, 0);
	m_listHeader.EnableTracking(FALSE);

	// prevent translation of the list header
	CLocalizer::EnableTranslation(m_listHeader, FALSE);
	
	BOOL bVisible = (lpCreateStruct->style & WS_VISIBLE);
	CRect rect(0, 0, lpCreateStruct->cx, lpCreateStruct->cy);
	
	DWORD dwStyle = (WS_CHILD | (bVisible ? WS_VISIBLE : 0));
	
	// Totals are disabled so they can't grab the focus
	dwStyle |= LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SINGLESEL | LVS_NOSCROLL | WS_TABSTOP | WS_DISABLED;

	if (!m_lcTotalsLabels.Create(dwStyle, rect, this, IDC_TOTALSLABELS))
	{
		return -1;
	}

	m_lcTotalsLabels.SetBkColor(m_crBkgnd);
	m_lcTotalsLabels.SetTextBkColor(m_crBkgnd);
	ListView_SetExtendedListViewStyleEx(m_lcTotalsLabels, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
	
	if (!m_lcColumnTotals.Create(dwStyle, rect, this, IDC_ALLOCATIONTOTALS))
	{
		return -1;
	}
	
	m_lcColumnTotals.ModifyStyleEx(0, WS_EX_CLIENTEDGE, 0);
	ListView_SetExtendedListViewStyleEx(m_lcColumnTotals, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);

	if (m_themeHeader.AreControlsThemed())
		VERIFY(m_themeHeader.Open(*this, _T("HEADER")));

	// Bar Chart ------------------------------------------------------------------------
	// Initialise graph
	VERIFY(m_barChart.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_BARCHART));

	m_barChart.SetBkGnd(GetSysColor(COLOR_WINDOW));
	m_barChart.SetXLabelsAreTicks(true);
	m_barChart.SetXLabelAngle(45);
	m_barChart.SetYTicks(10);
	
	BuildTaskTreeColumns();
	BuildListColumns();
	PopulateTotalsLists();
	
	return 0;
}

BOOL CWorkloadCtrl::SetCurrentPeriod(const COleDateTimeRange& dtPeriod)
{
	m_dtPeriod = dtPeriod;
	m_dWorkDaysInPeriod = dtPeriod.GetWeekdayCount();

	if (GetSafeHwnd())
	{
		UpdateTotalsDateRangeLabel();
		RecalcAllocationTotals();
	}

	return TRUE;
}

void CWorkloadCtrl::UpdateTotalsDateRangeLabel()
{
	if (m_lcTotalsLabels.GetSafeHwnd())
	{
		CString sLabel;

		if (m_dtPeriod.IsValid())
			sLabel.Format(CEnString(IDS_TOTALSDATERANGE), m_dtPeriod.Format());

		m_lcTotalsLabels.SetItemText(ID_TOTALCOLUMNHEADER, 0, sLabel);
	}
}

void CWorkloadCtrl::AdjustSplitterToFitAttributeColumns()
{
	AdjustSplitterToFitColumns();
}

int CWorkloadCtrl::CalcSplitPosToFitListColumns(int nTotalWidth) const
{
	// Adjust for bar chart
	return CTreeListCtrl::CalcSplitPosToFitListColumns(MulDiv(nTotalWidth, 2, 3));
}

DWORD CWorkloadCtrl::GetSelectedTaskID() const
{
	return GetSelectedItemData();
}

BOOL CWorkloadCtrl::GetSelectedTask(WORKLOADITEM& wi) const
{
	DWORD dwTaskID = GetSelectedTaskID();
	const WORKLOADITEM* pWI = NULL;

	GET_WI_RET(dwTaskID, pWI, FALSE);
	
	wi = *pWI;
	return TRUE;
}

BOOL CWorkloadCtrl::SetSelectedTask(const WORKLOADITEM& wi)
{
	DWORD dwTaskID = GetSelectedTaskID();
	WORKLOADITEM* pWI = NULL;

	GET_WI_RET(dwTaskID, pWI, FALSE);

	*pWI = wi;

	RecalcAllocationTotals();

	return TRUE;
}

BOOL CWorkloadCtrl::SelectTask(DWORD dwTaskID)
{
	HTREEITEM hti = GetTreeItem(dwTaskID);

	return SelectItem(hti);
}

BOOL CWorkloadCtrl::SelectTask(IUI_APPCOMMAND nCmd, const IUISELECTTASK& select)
{
	HTREEITEM htiStart = NULL;
	BOOL bForwards = TRUE;

	switch (nCmd)
	{
	case IUI_SELECTFIRSTTASK:
		htiStart = m_tree.TCH().GetFirstItem();
		break;

	case IUI_SELECTNEXTTASK:
		htiStart = m_tree.TCH().GetNextItem(GetSelectedItem());
		break;
		
	case IUI_SELECTNEXTTASKINCLCURRENT:
		htiStart = GetSelectedItem();
		break;

	case IUI_SELECTPREVTASK:
		htiStart = m_tree.TCH().GetPrevItem(GetSelectedItem());

		if (htiStart == NULL) // we were on the first task
			htiStart = m_tree.TCH().GetLastItem();
		
		bForwards = FALSE;
		break;

	case IUI_SELECTLASTTASK:
		htiStart = m_tree.TCH().GetLastItem();
		bForwards = FALSE;
		break;

	default:
		return FALSE;
	}

	return SelectTask(htiStart, select, bForwards);
}

BOOL CWorkloadCtrl::SelectTask(HTREEITEM hti, const IUISELECTTASK& select, BOOL bForwards)
{
	if (!hti)
		return FALSE;

	CString sTitle = m_tree.GetItemText(hti);

	if (Misc::Find(select.szWords, sTitle, select.bCaseSensitive, select.bWholeWord) != -1)
	{
		if (SelectItem(hti))
			return TRUE;

		ASSERT(0);
	}

	if (bForwards)
		return SelectTask(m_tree.TCH().GetNextItem(hti), select, TRUE);

	// else
	return SelectTask(m_tree.TCH().GetPrevItem(hti), select, FALSE);
}

BOOL CWorkloadCtrl::CanMoveSelectedTask(const IUITASKMOVE& move) const
{
	if (m_bReadOnly)
		return FALSE;

	TLCITEMMOVE itemMove = { 0 };

	itemMove.htiSel = GetTreeItem(move.dwSelectedTaskID);
	itemMove.htiDestParent = GetTreeItem(move.dwParentID);
	itemMove.htiDestAfterSibling = GetTreeItem(move.dwAfterSiblingID);

	return CTreeListCtrl::CanMoveItem(itemMove);
}

BOOL CWorkloadCtrl::MoveSelectedTask(const IUITASKMOVE& move)
{
	if (m_bReadOnly)
		return FALSE;

	TLCITEMMOVE itemMove = { 0 };

	itemMove.htiSel = GetTreeItem(move.dwSelectedTaskID);
	itemMove.htiDestParent = GetTreeItem(move.dwParentID);
	itemMove.htiDestAfterSibling = GetTreeItem(move.dwAfterSiblingID);

	return CTreeListCtrl::MoveItem(itemMove);
}

BOOL CWorkloadCtrl::EditWantsResort(const ITASKLISTBASE* pTasks, IUI_UPDATETYPE nUpdate) const
{
	switch (nUpdate)
	{
	case IUI_ALL:
		// Note: Tasks should arrive 'unsorted' so we only need to
		// resort if an attribute is set
		return m_sort.IsSorting();

	case IUI_NEW:
		// Don't sort new tasks because it's confusing
		return FALSE;

	case IUI_EDIT:
		if (m_sort.IsSorting())
		{
			if (!m_sort.bMultiSort)
				return pTasks->IsAttributeAvailable(MapColumnToAttribute(m_sort.single.nBy));

			// else
			for (int nCol = 0; nCol < 3; nCol++)
			{
				if (pTasks->IsAttributeAvailable(MapColumnToAttribute(m_sort.multi.cols[nCol].nBy)))
					return TRUE;
			}
		}
		break;

	case IUI_DELETE:
		break;

	default:
		ASSERT(0);
	}

	return FALSE;
}

void CWorkloadCtrl::UpdateTasks(const ITaskList* pTaskList, IUI_UPDATETYPE nUpdate)
{
	// we must have been initialized already
	ASSERT(m_list.GetSafeHwnd() && m_tree.GetSafeHwnd());

	// always cancel any ongoing operation
	CancelOperation();

	const ITASKLISTBASE* pTasks = GetITLInterface<ITASKLISTBASE>(pTaskList, IID_TASKLISTBASE);

	if (pTasks == NULL)
	{
		ASSERT(0);
		return;
	}

	// Two stage update to avoid redrawing when in
	// partially updated state
	CDWordArray aExpanded;
	GetExpandedState(aExpanded);

	DWORD dwSelID = GetSelectedTaskID();

	// Stage 1: Update the data structures
	switch (nUpdate)
	{
	case IUI_ALL:
		LockWindowUpdate();
		EnableResync(FALSE);

		RebuildTree(pTasks);
		break;
		
	case IUI_NEW:
	case IUI_EDIT:
		CWnd::SetRedraw(FALSE);
			
		// update the task(s)
		if (UpdateTask(pTasks, pTasks->GetFirstTask(), nUpdate, TRUE))
		{
			if (pTasks->IsAttributeAvailable(TDCA_DUEDATE) || pTasks->IsAttributeAvailable(TDCA_STARTDATE))
				m_data.RecalculateOverlaps();
		}
		break;
		
	case IUI_DELETE:
		CWnd::SetRedraw(FALSE);
		RemoveDeletedTasks(pTasks);
		break;
		
	default:
		ASSERT(0);
		return;
	}

	InitItemHeights();
	UpdateListColumns();
	FixupListSortColumn();
	RecalcAllocationTotals();
	RecalcDataDateRange();

	// Stage 2: Restore rendering
	switch (nUpdate)
	{
	case IUI_ALL:
		{
			// Odd bug: The very last tree item will sometimes not scroll into view. 
			// Expanding and collapsing an item is enough to resolve the issue. 
			// First time only though.
			if (aExpanded.GetSize() == 0)
				PreFixVScrollSyncBug();

			SetExpandedState(aExpanded);
			SelectTask(dwSelID);

			UnlockWindowUpdate();
			EnableResync(TRUE, m_tree);
			UpdateColumnWidths(UCWA_ANY);
		}
		break;

	case IUI_NEW:
	case IUI_EDIT:
	case IUI_DELETE:
		CWnd::SetRedraw(TRUE);
		InvalidateAll();
		break;

	default:
		ASSERT(0);
		return;
	}
		
	if (EditWantsResort(pTasks, nUpdate))
	{
		ASSERT(m_sort.IsSorting());

		CHoldRedraw hr(m_tree);

		if (m_sort.bMultiSort)
			CTreeListCtrl::Sort(MultiSortProc, (DWORD)this);
		else
			CTreeListCtrl::Sort(SortProc, (DWORD)this);
	}
}

void CWorkloadCtrl::PreFixVScrollSyncBug()
{
	// Odd bug: The very last tree item will not scroll into view. 
	// Expanding and collapsing an item is enough to resolve the issue.
	HTREEITEM hti = TCH().FindFirstParent();

	if (hti)
	{
		TCH().ExpandItem(hti, TRUE);
		TCH().ExpandItem(hti, FALSE);
	}
}

void CWorkloadCtrl::RecalcAllocationTotals()
{
	m_data.CalculateTotals(m_dtPeriod, m_mapTotalDays, m_mapTotalTasks);

	// Individual loading
	m_mapPercentLoad.RemoveAll();

	if (m_dWorkDaysInPeriod > 0.0)
	{
		for (int nAllocTo = 0; nAllocTo < m_aAllocTo.GetSize(); nAllocTo++)
		{
			const CString& sAllocTo = m_aAllocTo[nAllocTo];
			double dTotalDays = m_mapTotalDays.Get(sAllocTo);

			m_mapPercentLoad.Set(sAllocTo, (dTotalDays * 100 / m_dWorkDaysInPeriod));
		}
	}

	m_lcColumnTotals.Invalidate();
	m_barChart.RebuildChart();
}

int CWorkloadCtrl::GetTaskAllocTo(const ITASKLISTBASE* pTasks, HTASKITEM hTask, CStringArray& aAllocTo)
{
	aAllocTo.RemoveAll();
	int nAllocTo = pTasks->GetTaskAllocatedToCount(hTask);
	
	while (nAllocTo--)
		aAllocTo.InsertAt(0, pTasks->GetTaskAllocatedTo(hTask, nAllocTo));
	
	return aAllocTo.GetSize();
}

double CWorkloadCtrl::GetTaskTimeEstimate(const ITASKLISTBASE* pTasks, HTASKITEM hTask)
{
	TDC_UNITS nUnits = TDCU_NULL;
	double dTimeEst = pTasks->GetTaskTimeEstimate(hTask, nUnits, true);

	TH_UNITS nTHUnits = THU_NULL;

	switch (nUnits)
	{
	case TDCU_MINS:		nTHUnits = THU_MINS;	break;
	case TDCU_HOURS:	nTHUnits = THU_HOURS;	break;
	case TDCU_WEEKS:	nTHUnits = THU_WEEKS;	break;
	case TDCU_MONTHS:	nTHUnits = THU_MONTHS;	break;
	case TDCU_YEARS:	nTHUnits = THU_YEARS;	break;

	case TDCU_DAYS:
	case TDCU_WEEKDAYS:
		return dTimeEst;

	default:
		ASSERT(0);
		return 0.0;
	}

	return CTimeHelper().GetTime(dTimeEst, nTHUnits, THU_WEEKDAYS);
}

BOOL CWorkloadCtrl::WantEditUpdate(TDC_ATTRIBUTE nAttrib)
{
	switch (nAttrib)
	{
	case TDCA_ALLOCTO:
	case TDCA_COLOR:
	case TDCA_DONEDATE:
	case TDCA_DUEDATE:
	case TDCA_ICON:
	case TDCA_ID:
	case TDCA_NONE:
	case TDCA_PERCENT:
	case TDCA_STARTDATE:
	case TDCA_SUBTASKDONE:
	case TDCA_TASKNAME:
	case TDCA_TIMEEST:
		return TRUE;
	}
	
	// all else 
	return (nAttrib == IUI_ALL);
}

BOOL CWorkloadCtrl::WantSortUpdate(TDC_ATTRIBUTE nAttrib)
{
	switch (nAttrib)
	{
	case TDCA_ALLOCTO:
	case TDCA_DUEDATE:
	case TDCA_ID:
	case TDCA_PERCENT:
	case TDCA_STARTDATE:
	case TDCA_TASKNAME:
		return (MapAttributeToColumn(nAttrib) != WLCC_NONE);

	case TDCA_NONE:
		return TRUE;
	}
	
	// all else 
	return FALSE;
}

TDC_ATTRIBUTE CWorkloadCtrl::MapColumnToAttribute(WLC_COLUMNID nCol)
{
	switch (nCol)
	{
	case WLCC_TITLE:		return TDCA_TASKNAME;
	case WLCC_DUEDATE:		return TDCA_DUEDATE;
	case WLCC_STARTDATE:	return TDCA_STARTDATE;
	case WLCC_PERCENT:		return TDCA_PERCENT;
	case WLCC_TASKID:		return TDCA_ID;
	case WLCC_ALLOCTO:		return TDCA_ALLOCTO;
	case WLCC_DURATION:		return TDCA_TIMEEST;
	}
	
	// all else 
	return TDCA_NONE;
}

WLC_COLUMNID CWorkloadCtrl::MapAttributeToColumn(TDC_ATTRIBUTE nAttrib)
{
	switch (nAttrib)
	{
	case TDCA_TASKNAME:		return WLCC_TITLE;		
	case TDCA_DUEDATE:		return WLCC_DUEDATE;		
	case TDCA_STARTDATE:	return WLCC_STARTDATE;	
	case TDCA_PERCENT:		return WLCC_PERCENT;		
	case TDCA_ID:			return WLCC_TASKID;		
	case TDCA_ALLOCTO:		return WLCC_ALLOCTO;		
	case TDCA_TIMEEST:		return WLCC_DURATION;		
	}
	
	// all else 
	return WLCC_NONE;
}

BOOL CWorkloadCtrl::UpdateTask(const ITASKLISTBASE* pTasks, HTASKITEM hTask, IUI_UPDATETYPE nUpdate, BOOL bAndSiblings)
{
	if (hTask == NULL)
		return FALSE;

	ASSERT((nUpdate == IUI_EDIT) || (nUpdate == IUI_NEW));

	// handle task if not NULL (== root)
	DWORD dwTaskID = pTasks->GetTaskID(hTask);
	m_dwMaxTaskID = max(m_dwMaxTaskID, dwTaskID);

	// This can be a new task
	if (!m_data.HasItem(dwTaskID))
	{
		ASSERT(nUpdate == IUI_NEW);

		// Parent must exist or be NULL
		DWORD dwParentID = pTasks->GetTaskParentID(hTask);
		HTREEITEM htiParent = NULL;

		if (dwParentID)
		{
			if (!m_data.HasItem(dwParentID))
			{
				ASSERT(0);
				return FALSE;
			}

			htiParent = GetTreeItem(dwParentID);

			if (!htiParent)
			{
				ASSERT(0);
				return FALSE;
			}
		}

		// Before anything else we increment the position of 
		// any tasks having the same position of this new task 
		// or greater within the parent
		int nPos = pTasks->GetTaskPosition(hTask);
		IncrementItemPositions(htiParent, nPos);

		BuildTreeItem(pTasks, hTask, htiParent, FALSE, FALSE);
		RefreshTreeItemMap();

		return TRUE;
	}
	
	WORKLOADITEM* pWI = NULL;
	GET_WI_RET(dwTaskID, pWI, FALSE);

	// update taskID to refID 
	if (pWI->dwOrgRefID)
	{
		dwTaskID = pWI->dwOrgRefID;
		pWI->dwOrgRefID = 0;
	}

	// take a snapshot we can check changes against
	// Note: No need to check meta-data because we're 
	// the only one who can change it
	WORKLOADITEM giOrg = *pWI;

	// can't use a switch here because we also need to check for IUI_ALL
	time64_t tDate = 0;
	BOOL bAllocationChange = FALSE;
	
	if (pTasks->IsAttributeAvailable(TDCA_TASKNAME))
		pWI->sTitle = pTasks->GetTaskTitle(hTask);
	
 	if (pTasks->IsAttributeAvailable(TDCA_ALLOCTO))
	{
 		GetTaskAllocTo(pTasks, hTask, pWI->aAllocTo);
		Misc::AddUniqueItems(pWI->aAllocTo, m_aAllocTo);

		bAllocationChange = TRUE;
	}
	
	if (pTasks->IsAttributeAvailable(TDCA_ICON))
		pWI->bHasIcon = !Misc::IsEmpty(pTasks->GetTaskIcon(hTask));

	if (pTasks->IsAttributeAvailable(TDCA_PERCENT))
		pWI->nPercent = pTasks->GetTaskPercentDone(hTask, TRUE);
		
	if (pTasks->IsAttributeAvailable(TDCA_STARTDATE))
	{
		if (pTasks->GetTaskStartDate64(hTask, pWI->bParent, tDate))
			pWI->dtRange.m_dtStart = CDateHelper::GetDate(tDate);
		else
			CDateHelper::ClearDate(pWI->dtRange.m_dtStart);

		bAllocationChange = TRUE;
	}
	
	if (pTasks->IsAttributeAvailable(TDCA_DUEDATE))
	{
		if (pTasks->GetTaskDueDate64(hTask, pWI->bParent, tDate))
			pWI->dtRange.m_dtEnd = CDateHelper::GetDate(tDate);
		else
			CDateHelper::ClearDate(pWI->dtRange.m_dtEnd);

		bAllocationChange = TRUE;
	}

	if (pTasks->IsAttributeAvailable(TDCA_TIMEEST))
	{
		pWI->dTimeEst = GetTaskTimeEstimate(pTasks, hTask);

		bAllocationChange = TRUE;
	}
	
	if (pTasks->IsAttributeAvailable(TDCA_DONEDATE))
		pWI->bDone = pTasks->IsTaskDone(hTask);

	if (pTasks->IsAttributeAvailable(TDCA_SUBTASKDONE))
	{
		LPCWSTR szSubTaskDone = pTasks->GetTaskSubtaskCompletion(hTask);
		pWI->bSomeSubtaskDone = (!Misc::IsEmpty(szSubTaskDone) && (szSubTaskDone[0] != '0'));
	}

	// always update lock states
	pWI->bLocked = pTasks->IsTaskLocked(hTask, true);

	// always update colour because it can change for so many reasons
	pWI->color = pTasks->GetTaskTextColor(hTask);

	// likewise 'Good as Done'
	pWI->bGoodAsDone = pTasks->IsTaskGoodAsDone(hTask);

	// detect update
	BOOL bChange = !(*pWI == giOrg);
		
	// children
	if (UpdateTask(pTasks, pTasks->GetFirstTask(hTask), nUpdate, TRUE))
		bChange = TRUE;

	// handle siblings WITHOUT RECURSION
	if (bAndSiblings)
	{
		HTASKITEM hSibling = pTasks->GetNextTask(hTask);
		
		while (hSibling)
		{
			// FALSE == not siblings
			if (UpdateTask(pTasks, hSibling, nUpdate, FALSE))
				bChange = TRUE;
			
			hSibling = pTasks->GetNextTask(hSibling);
		}
	}

	if (bAllocationChange &&
		HasOption(WLCF_CALCMISSINGALLOCATIONS) &&
		pWI->mapAllocatedDays.IsAutoCalculated())
	{
		pWI->AutoCalculateAllocations(HasOption(WLCF_PREFERTIMEESTFORCALCS));
	}
	
	return bChange;
}

void CWorkloadCtrl::BuildTaskIDMap(const ITASKLISTBASE* pTasks, HTASKITEM hTask, 
								   CDWordSet& mapIDs, BOOL bAndSiblings)
{
	if (hTask == NULL)
		return;

	mapIDs.Add(pTasks->GetTaskID(hTask));

	// children
	BuildTaskIDMap(pTasks, pTasks->GetFirstTask(hTask), mapIDs, TRUE);

	// handle siblings WITHOUT RECURSION
	if (bAndSiblings)
	{
		HTASKITEM hSibling = pTasks->GetNextTask(hTask);
		
		while (hSibling)
		{
			// FALSE == not siblings
			BuildTaskIDMap(pTasks, hSibling, mapIDs, FALSE);
			hSibling = pTasks->GetNextTask(hSibling);
		}
	}
}

void CWorkloadCtrl::RemoveDeletedTasks(const ITASKLISTBASE* pTasks)
{
	CDWordSet mapIDs;
	BuildTaskIDMap(pTasks, pTasks->GetFirstTask(), mapIDs, TRUE);

	if (RemoveDeletedTasks(NULL, pTasks, mapIDs))
	{
		m_data.RecalculateOverlaps();
		RefreshTreeItemMap();
	}
}

BOOL CWorkloadCtrl::RemoveDeletedTasks(HTREEITEM hti, const ITASKLISTBASE* pTasks, const CDWordSet& mapIDs)
{
	// traverse the tree looking for items that do not 
	// exist in pTasks and delete them
	if (hti && !mapIDs.Has(GetTaskID(hti)))
	{
		DeleteItem(hti);
		return TRUE;
	}

	// check its children
	BOOL bSomeRemoved = FALSE;
	HTREEITEM htiChild = m_tree.GetChildItem(hti);
	
	while (htiChild)
	{
		// get next sibling before we (might) delete this one
		HTREEITEM htiNext = m_tree.GetNextItem(htiChild, TVGN_NEXT);
		
		bSomeRemoved |= RemoveDeletedTasks(htiChild, pTasks, mapIDs);
		htiChild = htiNext;
	}

	return bSomeRemoved;
}

WORKLOADITEM* CWorkloadCtrl::GetWorkloadItem(DWORD dwTaskID, BOOL bCopyRefID) const
{
	WORKLOADITEM* pWI = m_data.GetItem(dwTaskID);
	ASSERT(pWI);
	
	if (pWI)
	{
		// For references we use the 'real' task but with the 
		// original reference reference ID copied over
		DWORD dwRefID = pWI->dwRefID;
		
		if (dwRefID && (dwRefID != dwTaskID) && m_data.Lookup(dwRefID, pWI))
		{
			// copy over the reference id so that the caller can still detect it
			if (bCopyRefID)
			{
				ASSERT((pWI->dwRefID == 0) || (pWI->dwRefID == dwRefID));
				pWI->dwOrgRefID = dwRefID;
			}
		}
		else
		{
			pWI->dwOrgRefID = 0;
		}
	}
	
	return pWI;
}

void CWorkloadCtrl::RebuildTree(const ITASKLISTBASE* pTasks)
{
	m_tree.DeleteAllItems();
	m_list.DeleteAllItems();

	m_aAllocTo.RemoveAll();
	m_data.RemoveAll();

	m_dwMaxTaskID = 0;

	BuildTreeItem(pTasks, pTasks->GetFirstTask(), NULL, TRUE);

	m_data.RecalculateOverlaps();

	RefreshTreeItemMap();
	ExpandList();
	RefreshItemBoldState();
}

void CWorkloadCtrl::RecalcDataDateRange()
{
	COleDateTimeRange dtRange;

	if (!m_data.CalcDateRange(dtRange))
		dtRange.Add(COleDateTime::GetCurrentTime());

	m_dtDataRange.Set(CDateHelper::GetStartOfMonth(dtRange.GetStart()), 
					  CDateHelper::GetEndOfMonth(dtRange.GetEnd()));
}

void CWorkloadCtrl::PopulateTotalsLists()
{
	// Build Task totals once only
	if (m_lcTotalsLabels.GetItemCount() == 0)
	{
		for (int nTotal = 0; nTotal < NUM_TOTALS; nTotal++)
		{
			int nItem = m_lcTotalsLabels.InsertItem(nTotal, CEnString(WORKLOADTOTALS[nTotal].nTextResID));
			m_lcTotalsLabels.SetItemData(nItem, WORKLOADTOTALS[nTotal].nTotal);

			nItem = m_lcColumnTotals.InsertItem(nTotal, _T(""));
			m_lcColumnTotals.SetItemData(nItem, WORKLOADTOTALS[nTotal].nTotal);
		}

		UpdateTotalsDateRangeLabel();
	}
}

void CWorkloadCtrl::BuildTreeItem(const ITASKLISTBASE* pTasks, HTASKITEM hTask, 
										HTREEITEM htiParent, BOOL bAndSiblings, BOOL bInsertAtEnd)
{
	if (hTask == NULL)
		return;

	DWORD dwTaskID = pTasks->GetTaskID(hTask);
	ASSERT(!m_data.HasItem(dwTaskID));

	m_dwMaxTaskID = max(m_dwMaxTaskID, dwTaskID);

	// map the data
	WORKLOADITEM* pWI = new WORKLOADITEM(dwTaskID);
	m_data[dwTaskID] = pWI;
	
	pWI->dwRefID = pTasks->GetTaskReferenceID(hTask);

	// Except for position
	pWI->nPosition = pTasks->GetTaskPosition(hTask);

	// Only save data for non-references
	if (pWI->dwRefID == 0)
	{
		pWI->sTitle = pTasks->GetTaskTitle(hTask);
		pWI->color = pTasks->GetTaskTextColor(hTask);
		pWI->bDone = pTasks->IsTaskDone(hTask);
		pWI->bGoodAsDone = pTasks->IsTaskGoodAsDone(hTask);
		pWI->bParent = pTasks->IsTaskParent(hTask);
		pWI->nPercent = pTasks->GetTaskPercentDone(hTask, TRUE);
		pWI->bLocked = pTasks->IsTaskLocked(hTask, true);
		pWI->bHasIcon = !Misc::IsEmpty(pTasks->GetTaskIcon(hTask));

		GetTaskAllocTo(pTasks, hTask, pWI->aAllocTo);
		Misc::AddUniqueItems(pWI->aAllocTo, m_aAllocTo);
		
		LPCWSTR szSubTaskDone = pTasks->GetTaskSubtaskCompletion(hTask);
		pWI->bSomeSubtaskDone = (!Misc::IsEmpty(szSubTaskDone) && (szSubTaskDone[0] != '0'));

		time64_t tDate = 0;

		if (pTasks->GetTaskStartDate64(hTask, pWI->bParent, tDate))
			pWI->dtRange.m_dtStart = CDateHelper::GetDate(tDate);

		if (pTasks->GetTaskDueDate64(hTask, pWI->bParent, tDate))
			pWI->dtRange.m_dtEnd = CDateHelper::GetDate(tDate);

		pWI->dTimeEst = GetTaskTimeEstimate(pTasks, hTask);

		// This wants to be last so that time estimate and date range are up to date
		if (!pWI->mapAllocatedDays.Decode(pTasks->GetTaskMetaData(hTask, WORKLOAD_TYPEID)))
			pWI->AutoCalculateAllocations(HasOption(WLCF_PREFERTIMEESTFORCALCS));
	}
	
	// add item to tree
	HTREEITEM htiAfter = TVI_LAST; // default

	if (!bInsertAtEnd)
	{
		// Find the sibling task whose position is one less
		HTREEITEM htiSibling = m_tree.GetChildItem(htiParent);

		while (htiSibling)
		{
			DWORD dwSiblingID = m_tree.GetItemData(htiSibling);
			const WORKLOADITEM* pWISibling = GetWorkloadItem(dwSiblingID);
			ASSERT(pWISibling);

			if (pWISibling && (pWISibling->nPosition == (pWI->nPosition - 1)))
			{
				htiAfter = htiSibling;
				break;
			}

			htiSibling = m_tree.GetNextItem(htiSibling, TVGN_NEXT);
		}
	}

	HTREEITEM hti = m_tree.TCH().InsertItem(LPSTR_TEXTCALLBACK, 
											I_IMAGECALLBACK, 
											I_IMAGECALLBACK, 
											dwTaskID, // lParam
											htiParent, 
											htiAfter,
											FALSE,
											FALSE);
	
	// add first child which will add all the rest
	BuildTreeItem(pTasks, pTasks->GetFirstTask(hTask), hti, TRUE);
	
	// handle siblings WITHOUT RECURSION
	if (bAndSiblings)
	{
		HTASKITEM hSibling = pTasks->GetNextTask(hTask);
		
		while (hSibling)
		{
			// FALSE == not siblings
			BuildTreeItem(pTasks, hSibling, htiParent, FALSE);
			
			hSibling = pTasks->GetNextTask(hSibling);
		}
	}
}

void CWorkloadCtrl::IncrementItemPositions(HTREEITEM htiParent, int nFromPos)
{
	HTREEITEM htiChild = m_tree.GetChildItem(htiParent);

	while (htiChild)
	{
		DWORD dwTaskID = GetTaskID(htiChild);
		WORKLOADITEM* pWI = NULL;

		GET_WI(dwTaskID, pWI);

		if (pWI->nPosition >= nFromPos)
			pWI->nPosition++;
		
		htiChild = m_tree.GetNextItem(htiChild, TVGN_NEXT);
	}
}

void CWorkloadCtrl::SetOption(DWORD dwOption, BOOL bSet)
{
	if (dwOption)
	{
		DWORD dwPrev = m_dwOptions;

		if (bSet)
			m_dwOptions |= dwOption;
		else
			m_dwOptions &= ~dwOption;

		// specific handling
		if (m_dwOptions != dwPrev)
		{
			switch (dwOption)
			{
			case WLCF_STRIKETHRUDONETASKS:
				m_tree.Fonts().Clear();
				CWnd::Invalidate(FALSE);
				break;

			case WLCF_SHOWSPLITTERBAR:
				CTreeListCtrl::SetSplitBarWidth(bSet ? 10 : 0);
				break;

			case WLCF_DISPLAYISODATES:
				CWnd::Invalidate(FALSE);
				break;

			case WLCF_SHOWTREECHECKBOXES:
				m_tree.ShowCheckboxes(bSet);
				break;

			case WLCF_CALCMISSINGALLOCATIONS:
				RefreshMissingAllocations();
				break;

			case WLCF_PREFERTIMEESTFORCALCS:
				if (HasOption(WLCF_CALCMISSINGALLOCATIONS))
					RefreshMissingAllocations();
				else
					m_tree.Invalidate();
				break;
			}

			if (IsSyncing())
				RedrawList();
		}
	}
}

void CWorkloadCtrl::RefreshMissingAllocations()
{
	BOOL bAutoCalc = HasOption(WLCF_CALCMISSINGALLOCATIONS);
	BOOL bPreferTimeEst = HasOption(WLCF_PREFERTIMEESTFORCALCS);

	POSITION pos = m_data.GetStartPosition();

	while (pos)
	{
		WORKLOADITEM* pWI = m_data.GetNextItem(pos);

		if (pWI && pWI->mapAllocatedDays.IsAutoCalculated())
		{
			if (bAutoCalc)
				pWI->AutoCalculateAllocations(bPreferTimeEst);
			else
				pWI->ClearAllocations();
		}
	}
}

int CWorkloadCtrl::GetRequiredListColumnCount() const
{
	int nNumCols = m_aAllocTo.GetSize();

	nNumCols++; // first hidden column 
	nNumCols++; // spacer before last 'total' column
	nNumCols++; // last 'total' column

	return nNumCols;
}

void CWorkloadCtrl::BuildTaskTreeColumns()
{
	// add columns
	m_treeHeader.InsertItem(0, 0, _T("Task"), (HDF_LEFT | HDF_STRING), 0, WLCC_TITLE);
	m_treeHeader.EnableItemDragging(0, FALSE);

	for (int nCol = 0; nCol < NUM_TREECOLUMNS; nCol++)
	{
		m_treeHeader.InsertItem(nCol + 1, 
								0, 
								CEnString(WORKLOADTREECOLUMNS[nCol].nIDColName), 
								(WORKLOADTREECOLUMNS[nCol].nColAlign | HDF_STRING),
								0,
								WORKLOADTREECOLUMNS[nCol].nColID);
	}

	// Build Task totals once only
	m_lcTotalsLabels.InsertColumn(0, _T(""), LVCFMT_RIGHT, GetSplitPos());

	// Align right
	LV_COLUMN lvc = { LVCF_FMT, LVCFMT_RIGHT, 0 };
	m_lcTotalsLabels.SetColumn(0, &lvc);
}

void CWorkloadCtrl::OnResize(int cx, int cy)
{
	if (cx && cy)
	{
		CRect rTreeList(0, 0, MulDiv(cx, 2, 3), cy);

		rTreeList.bottom = cy;
		rTreeList.bottom -= (GetItemHeight(m_lcTotalsLabels) * NUM_TOTALS); // rows
		rTreeList.bottom -= LV_COLPADDING; // spacing
		rTreeList.bottom -= 2; // totals border

		CRect rChart(rTreeList.right + LV_COLPADDING, 0, cx, cy);
		m_barChart.MoveWindow(rChart);
				
		CTreeListCtrl::OnResize(rTreeList.Width(), rTreeList.Height());
	}
}

void CWorkloadCtrl::ResyncTotalsPositions()
{
	CRect rClient;
	CWnd::GetClientRect(rClient);

	CRect rTreeTotals = CDialogHelper::GetChildRect(&m_tree);
	CRect rColumnTotals = CDialogHelper::GetChildRect(&m_list);

	rTreeTotals.top = rColumnTotals.top = (max(rTreeTotals.bottom, rColumnTotals.bottom) + LV_COLPADDING);
	rTreeTotals.bottom = rColumnTotals.bottom = rClient.bottom;

	// Adjust for splitter border
	rColumnTotals.left--;

	// Adjust for border drawn by CTreeListSyncer
	rColumnTotals.right++;

	m_lcColumnTotals.MoveWindow(rColumnTotals);
	m_lcTotalsLabels.MoveWindow(rTreeTotals);
	m_lcTotalsLabels.SetColumnWidth(0, rTreeTotals.Width());
}

LRESULT CWorkloadCtrl::OnTreeCustomDraw(NMTVCUSTOMDRAW* pTVCD)
{
	HTREEITEM hti = (HTREEITEM)pTVCD->nmcd.dwItemSpec;
	
	switch (pTVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
								
	case CDDS_ITEMPREPAINT:
		{
			DWORD dwTaskID = pTVCD->nmcd.lItemlParam;
			WORKLOADITEM* pWI = NULL;

			GET_WI_RET(dwTaskID, pWI, 0L);
				
 			CDC* pDC = CDC::FromHandle(pTVCD->nmcd.hdc);
			CRect rItem(pTVCD->nmcd.rc);

			COLORREF crBack = DrawTreeItemBackground(pDC, hti, *pWI, rItem, rItem, FALSE);
				
			// hide text because we will draw it later
			pTVCD->clrTextBk = pTVCD->clrText = crBack;
		
		}
		return (CDRF_NOTIFYPOSTPAINT | CDRF_NEWFONT); // always
								
	case CDDS_ITEMPOSTPAINT:
		{
			// check row is visible
			CRect rItem;
			GetTreeItemRect(hti, WLCC_TITLE, rItem);

			CRect rClient;
			m_tree.GetClientRect(rClient);
			
			if ((rItem.bottom > 0) && (rItem.top < rClient.bottom))
			{
				DWORD dwTaskID = pTVCD->nmcd.lItemlParam;
				WORKLOADITEM* pWI = NULL;

				GET_WI_RET(dwTaskID, pWI, 0L);
				
				CDC* pDC = CDC::FromHandle(pTVCD->nmcd.hdc);

				GM_ITEMSTATE nState = GetItemState(hti);
				BOOL bSelected = (nState != GMIS_NONE);

				// draw horz gridline before selection
				DrawItemDivider(pDC, pTVCD->nmcd.rc, FALSE, bSelected);

				// Draw icon
				if (pWI->bHasIcon || pWI->bParent)
				{
					int iImageIndex = -1;
					HIMAGELIST hilTask = GetTaskIcon(pWI->dwTaskID, iImageIndex);

					if (hilTask && (iImageIndex != -1))
					{
						CRect rItem;
						m_tree.GetItemRect(hti, rItem, TRUE);

						CRect rIcon(rItem);
						rIcon.left -= (m_tree.IMAGE_SIZE + 2);
						rIcon.bottom = (rIcon.top + m_tree.IMAGE_SIZE);
						GraphicsMisc::CentreRect(rIcon, rItem, FALSE, TRUE);

						ImageList_Draw(hilTask, iImageIndex, *pDC, rIcon.left, rIcon.top, ILD_TRANSPARENT);
					}
				}
				
				// draw background
				COLORREF crBack = DrawTreeItemBackground(pDC, hti, *pWI, rItem, rClient, bSelected);
				
				// draw Workload item attribute columns
				DrawTreeItem(pDC, hti, *pWI, bSelected, crBack);
			}			
	
		}
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_DODEFAULT;
}

HIMAGELIST CWorkloadCtrl::GetTaskIcon(DWORD dwTaskID, int& iImageIndex) const
{
	if (m_tree.GetImageList(TVSIL_NORMAL) == NULL)
		return NULL;

	return (HIMAGELIST)CWnd::GetParent()->SendMessage(WM_WLC_GETTASKICON, dwTaskID, (LPARAM)&iImageIndex);
}

COLORREF CWorkloadCtrl::DrawTreeItemBackground(CDC* pDC, HTREEITEM hti, const WORKLOADITEM& wi, const CRect& rItem, const CRect& rClient, BOOL bSelected)
{
	BOOL bAlternate = (HasAltLineColor() && !IsTreeItemLineOdd(hti));
	COLORREF crBack = GetTreeTextBkColor(wi, bSelected, bAlternate);

	if (!bSelected)
	{
		// redraw item background else tooltips cause overwriting
		CRect rBack(rItem);
		rBack.bottom--;
		rBack.right = rClient.right;

		pDC->FillSolidRect(rBack, crBack);
	}
	else
	{
		DWORD dwFlags = (GMIB_THEMECLASSIC | GMIB_EXTENDRIGHT | GMIB_CLIPRIGHT);
		GraphicsMisc::DrawExplorerItemBkgnd(pDC, m_tree, GetItemState(hti), rItem, dwFlags);
	}

	return crBack;
}

GM_ITEMSTATE CWorkloadCtrl::GetItemState(int nItem) const
{
	if (!m_bSavingToImage)
		return CTreeListCtrl::GetItemState(nItem);

	// else
	return GMIS_NONE;
}

GM_ITEMSTATE CWorkloadCtrl::GetItemState(HTREEITEM hti) const
{
	if (!m_bSavingToImage)
		return CTreeListCtrl::GetItemState(hti);

	// else
	return GMIS_NONE;
}

LRESULT CWorkloadCtrl::OnListCustomDraw(NMLVCUSTOMDRAW* pLVCD)
{
	ASSERT(pLVCD->nmcd.hdr.idFrom == IDC_ALLOCATIONCOLUMNS);

	return OnAllocationsListCustomDraw(pLVCD);
}

void CWorkloadCtrl::OnTotalsListsCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	switch (pNMHDR->idFrom)
	{
	case IDC_ALLOCATIONCOLUMNS:
		ASSERT(0);
		break;

	case IDC_TOTALSLABELS:
		*pResult = OnTotalsLabelsListCustomDraw((NMLVCUSTOMDRAW*)pNMHDR);
		break;

	case IDC_ALLOCATIONTOTALS:
		*pResult = OnAllocationsTotalsListCustomDraw((NMLVCUSTOMDRAW*)pNMHDR);
		break;
	}
}

LRESULT CWorkloadCtrl::OnTotalsLabelsListCustomDraw(NMLVCUSTOMDRAW* pLVCD)
{
	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		{
			CRect rClient;
			m_lcTotalsLabels.GetClientRect(rClient);

			CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);
			pDC->FillSolidRect(rClient, (m_bSavingToImage ? GetSysColor(COLOR_WINDOW) : m_crBkgnd));
		}
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			pLVCD->clrTextBk = (m_bSavingToImage ? GetSysColor(COLOR_WINDOW) : m_crBkgnd);

			CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);

			if (pLVCD->nmcd.lItemlParam == ID_TOTALCOLUMNHEADER)
				pDC->SelectObject(m_tree.Fonts().GetFont(GMFS_BOLD));
			else
				pDC->SelectObject(m_tree.Fonts().GetFont());
		}
		return CDRF_NEWFONT;
	}

	// else
	return CDRF_DODEFAULT;
}

LRESULT CWorkloadCtrl::OnAllocationsTotalsListCustomDraw(NMLVCUSTOMDRAW* pLVCD)
{
	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
								
	case CDDS_ITEMPREPAINT:
		{
			HWND hwndList = pLVCD->nmcd.hdr.hwndFrom;
			int nItem = (int)pLVCD->nmcd.dwItemSpec;
			CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);
			
			CRect rItem, rClient;
			m_lcColumnTotals.GetItemRect(nItem, rItem, LVIR_BOUNDS);
			m_lcColumnTotals.GetClientRect(rClient);

			// draw item bkgnd and gridlines full width of list
			CRect rFullWidth(rItem);
			rFullWidth.right = rClient.right + 2;

			switch (nItem)
			{
			case ID_TOTALCOLUMNHEADER:
				if (m_themeHeader.AreControlsThemed())
					m_themeHeader.DrawBackground(pDC, HP_HEADERITEM, HIS_NORMAL, rFullWidth);
				else
					pDC->FillSolidRect(rFullWidth, ::GetSysColor(COLOR_3DFACE));
				break;
				
			case ID_TOTALDAYSPERPERSON:
			case ID_TOTALTASKSPERPERSON:
			case ID_PERCENTLOADPERPERSON:
				{
					pDC->FillSolidRect(rFullWidth, GetRowColor(nItem + 1));
					
					if (m_bSavingToImage || ((nItem + 1) != ID_LASTTOTAL))
						DrawItemDivider(pDC, rFullWidth, FALSE, FALSE);
				}
				break;
				
			default:
				ASSERT(0);
			}
			
			// Draw content
			switch (nItem)
			{
			case ID_TOTALCOLUMNHEADER:
				DrawTotalsHeader(pDC);
				break;

			case ID_TOTALDAYSPERPERSON:
				DrawTotalsListItem(pDC, nItem, m_mapTotalDays, 2);
				break;
				
			case ID_TOTALTASKSPERPERSON:
				DrawTotalsListItem(pDC, nItem, m_mapTotalTasks, 0);
				break;
				
			case ID_PERCENTLOADPERPERSON:
				DrawTotalsListItem(pDC, nItem, m_mapPercentLoad, 1);
				break;

			default:
				ASSERT(0);
			}
		}
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_DODEFAULT;
}

LRESULT CWorkloadCtrl::OnAllocationsListCustomDraw(NMLVCUSTOMDRAW* pLVCD)
{
	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
								
	case CDDS_ITEMPREPAINT:
		{
			HWND hwndList = pLVCD->nmcd.hdr.hwndFrom;
			int nItem = (int)pLVCD->nmcd.dwItemSpec;
			
			DWORD dwTaskID = GetTaskID(nItem);

			WORKLOADITEM* pWI = NULL;
			GET_WI_RET(dwTaskID, pWI, 0L);

			CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);
			
			// draw item bkgnd and gridlines full width of list
			CRect rItem;
			m_list.GetItemRect(nItem, rItem, LVIR_BOUNDS);

			COLORREF crBack = GetRowColor(nItem);
			pLVCD->clrTextBk = pLVCD->clrText = crBack;
			
			GraphicsMisc::FillItemRect(pDC, rItem, crBack, m_list);

			DrawItemDivider(pDC, rItem, FALSE, FALSE);
			
			// draw background
			GM_ITEMSTATE nState = GetItemState(nItem);
			BOOL bSelected = (nState != GMIS_NONE);
			DWORD dwFlags = (GMIB_THEMECLASSIC | GMIB_CLIPLEFT);

			GraphicsMisc::DrawExplorerItemBkgnd(pDC, m_list, nState, rItem, dwFlags);

			// draw row
			COLORREF crText = GetTreeTextColor(*pWI, bSelected, FALSE);
			pDC->SetTextColor(crText);
			
			DrawAllocationListItem(pDC, nItem, *pWI, bSelected);
		}
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_DODEFAULT;
}

LRESULT CWorkloadCtrl::OnHeaderCustomDraw(NMCUSTOMDRAW* pNMCD)
{
	if (pNMCD->hdr.hwndFrom == m_listHeader)
	{
		switch (pNMCD->dwDrawStage)
		{
		case CDDS_PREPAINT:
			// only need handle drawing for coloumn sorting or double row height
			if (m_sort.IsSingleSortingBy(WLCC_ALLOCTO) || (m_listHeader.GetRowCount() > 1))
			{
				return CDRF_NOTIFYITEMDRAW;
			}
			break;
							
		case CDDS_ITEMPREPAINT:
			// only need handle drawing for double row height
			if (m_listHeader.GetRowCount() > 1)
			{
				CDC* pDC = CDC::FromHandle(pNMCD->hdc);
				int nCol = (int)pNMCD->dwItemSpec;

				DrawListHeaderItem(pDC, nCol);
				return CDRF_SKIPDEFAULT;
			}
			// else alloc to selection
			return CDRF_NOTIFYPOSTPAINT;

		case CDDS_ITEMPOSTPAINT:
			{
				int nCol = (int)pNMCD->dwItemSpec;

				if (nCol == m_nSortByAllocToCol)
				{
					CDC* pDC = CDC::FromHandle(pNMCD->hdc);
					m_listHeader.DrawItemSortArrow(pDC, nCol, m_sort.single.bAscending);
				}
			}
			break;
		}
	}
	else if (pNMCD->hdr.hwndFrom == m_treeHeader)
	{
		switch (pNMCD->dwDrawStage)
		{
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;
			
		case CDDS_ITEMPREPAINT:
			{
				// don't draw columns having min width
				CRect rItem(pNMCD->rc);
				
				if (rItem.Width() <= m_tree.MIN_COL_WIDTH)
					return CDRF_DODEFAULT;
			}
			return CDRF_NOTIFYPOSTPAINT;
			
		case CDDS_ITEMPOSTPAINT:
			{
				// draw sort direction
				int nCol = (int)pNMCD->dwItemSpec;
				WLC_COLUMNID nColID = GetTreeColumnID(nCol);
				CDC* pDC = CDC::FromHandle(pNMCD->hdc);
				
				if (m_sort.IsSingleSortingBy(nColID))
				{
					m_treeHeader.DrawItemSortArrow(pDC, nCol, m_sort.single.bAscending);
				}
			}
			break;
		}
	}
	
	return CDRF_DODEFAULT;
}

// Called by parent
void CWorkloadCtrl::Sort(WLC_COLUMNID nBy, BOOL bAscending)
{
	ASSERT(bAscending != -1);

	FixupListSortColumn();

	Sort(nBy, FALSE, bAscending, FALSE);
}

void CWorkloadCtrl::SetSortByAllocTo(LPCTSTR szAllocTo)
{
	m_sSortByAllocTo = szAllocTo;
}

void CWorkloadCtrl::FixupListSortColumn(LPCTSTR szAllocTo)
{
	if (!Misc::IsEmpty(szAllocTo))
	{
		m_sSortByAllocTo = szAllocTo;
	}
	else if (m_sSortByAllocTo.IsEmpty() && m_aAllocTo.GetSize())
	{
		m_sSortByAllocTo = m_aAllocTo[0];
	}

	if (m_sSortByAllocTo == ALLOCTO_TOTALID)
	{
		m_nSortByAllocToCol = (m_listHeader.GetItemCount() - 1);
	}
	else
	{
		m_nSortByAllocToCol = Misc::Find(m_sSortByAllocTo, m_aAllocTo);

		if (m_nSortByAllocToCol >= 0)
			m_nSortByAllocToCol++; // first column is hidden
	}
}

void CWorkloadCtrl::Sort(WLC_COLUMNID nBy, BOOL bAllowToggle, BOOL bAscending, BOOL bNotifyParent)
{
	ASSERT((nBy != WLCC_ALLOCTO) || !m_sSortByAllocTo.IsEmpty());

	m_sort.Sort(nBy, bAllowToggle, bAscending);

	// do the sort
	CHoldRedraw hr(m_tree);
	CTreeListCtrl::Sort(SortProc, (DWORD)this);

	// update sort arrow
	if (nBy == WLCC_ALLOCTO)
		m_listHeader.Invalidate(FALSE);
	else
		m_treeHeader.Invalidate(FALSE);

	if (bNotifyParent)
		CWnd::GetParent()->PostMessage(WM_WLCN_SORTCHANGE, m_sort.single.bAscending, m_sort.single.nBy);
}

void CWorkloadCtrl::Sort(const WORKLOADSORTCOLUMNS& multi)
{
	FixupListSortColumn();

	m_sort.Sort(multi);

	// do the sort
	CHoldRedraw hr(m_tree);
	CTreeListCtrl::Sort(MultiSortProc, (DWORD)this);

	// hide sort arrow
	m_treeHeader.Invalidate(FALSE);
}

void CWorkloadCtrl::OnBeginEditTreeLabel(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	*pResult = TRUE; // cancel our edit always
	
	if (m_bReadOnly)
		return;

	CPoint point(GetMessagePos());
	int nCol = -1;
	HTREEITEM hti = TreeHitTestItem(point, TRUE, nCol);

	if (!hti || (nCol == -1))
		return;

	if (m_treeHeader.GetItemData(nCol) != WLCC_TITLE)
		return;

	// notify app to edit
	CWnd::GetParent()->SendMessage(WM_WLC_EDITTASKTITLE, 0, GetTaskID(hti));
}

void CWorkloadCtrl::OnClickTreeHeader(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	HD_NOTIFY *pHDN = (HD_NOTIFY *) pNMHDR;
	
	if (pHDN->iButton == 0) // left button
	{
		WLC_COLUMNID nColID = GetTreeColumnID(pHDN->iItem);
		Sort(nColID, TRUE, -1, TRUE);
	}
}

void CWorkloadCtrl::OnTreeGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	TV_DISPINFO* pDispInfo = (TV_DISPINFO*)pNMHDR;
	DWORD dwTaskID = pDispInfo->item.lParam;
	
	const WORKLOADITEM* pWI = NULL;
	GET_WI(dwTaskID, pWI);
	
	if (pDispInfo->item.mask & TVIF_TEXT)
	{
		pDispInfo->item.pszText = (LPTSTR)(LPCTSTR)pWI->sTitle;
	}
	
	if (pDispInfo->item.mask & (TVIF_SELECTEDIMAGE | TVIF_IMAGE))
	{
		// checkbox image
		pDispInfo->item.mask |= TVIF_STATE;
		pDispInfo->item.stateMask = TVIS_STATEIMAGEMASK;
		
		if (pWI->bDone)
		{
			pDispInfo->item.state = TCHC_CHECKED;
		}
		else if (pWI->bSomeSubtaskDone)
		{
			pDispInfo->item.state = TCHC_MIXED;
		}
		else 
		{
			pDispInfo->item.state = TCHC_UNCHECKED;
		}
	}
}

UINT CWorkloadCtrl::OnDragOverItem(UINT nCursor)
{
	// We currently DON'T support 'linking'
	if (nCursor == DD_DROPEFFECT_LINK)
		nCursor = DD_DROPEFFECT_NONE;
	
	return nCursor;
}

BOOL CWorkloadCtrl::OnDragDropItem(const TLCITEMMOVE& move)
{
	// Notify parent of move
	IUITASKMOVE taskMove = { 0 };
			
	taskMove.dwSelectedTaskID = GetTaskID(move.htiSel);
	taskMove.dwParentID = GetTaskID(move.htiDestParent);
	taskMove.dwAfterSiblingID = GetTaskID(move.htiDestAfterSibling);
	taskMove.bCopy = (move.bCopy != FALSE);
			
	// If copying a task, app will send us a full update 
	// so we do not need to perform the move ourselves
	if (CWnd::GetParent()->SendMessage(WM_WLC_MOVETASK, 0, (LPARAM)&taskMove) && !taskMove.bCopy)
	{
		VERIFY(MoveItem(move));
	}

	return TRUE; // prevent base class handling
}

LRESULT CWorkloadCtrl::ScWindowProc(HWND hRealWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (!IsResyncEnabled())
		return CTreeListCtrl::ScWindowProc(hRealWnd, msg, wp, lp);

	if (hRealWnd == m_list)
	{
		switch (msg)
		{
		case WM_HSCROLL:
			CTreeListCtrl::ScWindowProc(hRealWnd, msg, wp, lp);

			// Keep totals synced
			m_lcColumnTotals.Invalidate(FALSE);
			m_lcColumnTotals.UpdateWindow();
			return 0L;

		case WM_MOUSEWHEEL:
			CTreeListCtrl::ScWindowProc(hRealWnd, msg, wp, lp);

			if (HasHScrollBar(hRealWnd) && !HasVScrollBar(hRealWnd))
			{
				// Keep totals synced
				m_lcColumnTotals.Invalidate(FALSE);
				m_lcColumnTotals.UpdateWindow();
			}
			return 0L;

		case WM_SETCURSOR:
			if (!m_bReadOnly)
			{
				int nCol, nHit = ListHitTestItem(GetMessagePos(), TRUE, nCol);

				if (nHit != -1)
				{
					BOOL bCanEdit = (GetListColumnType(nCol) == WLCT_VALUE);

					if (bCanEdit)
					{
						const WORKLOADITEM* pWI = NULL;
						GET_WI_RET(GetTaskID(nHit), pWI, FALSE);

						bCanEdit = !pWI->bParent;
					}

					if (!bCanEdit)
						return GraphicsMisc::SetAppCursor(_T("NoDrag"), _T("Resources\\Cursors"));
				}
			}
			break;
		}
	}
	else if (hRealWnd == m_tree)
	{
		switch (msg)
		{
		case WM_SETCURSOR:
			if (!m_bReadOnly)
			{
				CPoint ptCursor(GetMessagePos());
				DWORD dwTaskID = TreeHitTestTask(ptCursor, TRUE);

				if (dwTaskID && m_data.ItemIsLocked(dwTaskID))
					return GraphicsMisc::SetAppCursor(_T("Locked"), _T("Resources\\Cursors"));
			}
			break;
		}
	}

	return CTreeListCtrl::ScWindowProc(hRealWnd, msg, wp, lp);
}

BOOL CWorkloadCtrl::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint pt)
{
	// We have to handle mouse wheel over the totals because we have disabled it
	if (HasHScrollBar(m_list))
	{
		CWnd::ScreenToClient(&pt);

		if (ChildWindowFromPoint(pt) == &m_lcColumnTotals)
		{
			BOOL bScrollRight = (zDelta < 0);
			int nScroll = 3;

			while (nScroll--)
				m_list.SendMessage(WM_HSCROLL, (bScrollRight ? SB_LINERIGHT : SB_LINELEFT), 0L);

			return TRUE;
		}
	}

	return FALSE;
}

void CWorkloadCtrl::OnListHeaderClick(NMHEADER* pHDN)
{
	if (pHDN->iButton == 0) // left button
	{
		CString sSortByAllocTo;

		switch (GetListColumnType(pHDN->iItem))
		{
		case WLCT_TOTAL:
			sSortByAllocTo = ALLOCTO_TOTALID;
			break;

		case WLCT_VALUE:
			sSortByAllocTo = m_listHeader.GetItemText(pHDN->iItem);
			break;

		default:
			return;
		}

		BOOL bAscending = -1, bAllowToggle = TRUE;

		if (m_sort.IsSingleSortingBy(WLCC_ALLOCTO))
		{
			bAllowToggle = (m_sSortByAllocTo == sSortByAllocTo);
			bAscending = m_sort.single.bAscending;
		}

		m_sSortByAllocTo = sSortByAllocTo;
		m_nSortByAllocToCol = pHDN->iItem;

		Sort(WLCC_ALLOCTO, bAllowToggle, bAscending, TRUE);
	}
}

BOOL CWorkloadCtrl::OnItemCheckChange(HTREEITEM hti)
{
	if (!m_bReadOnly)
	{
		DWORD dwTaskID = GetTaskID(hti);
		const WORKLOADITEM* pWI = m_data.GetItem(dwTaskID);
		ASSERT(pWI);

		if (pWI)
			CWnd::GetParent()->SendMessage(WM_WLCN_COMPLETIONCHANGE, (WPARAM)m_tree.GetSafeHwnd(), !pWI->bDone);
	}

	return TRUE; // always
}

BOOL CWorkloadCtrl::OnListLButtonDblClk(UINT nFlags, CPoint point)
{
	if (CTreeListCtrl::OnListLButtonDblClk(nFlags, point))
		return TRUE;

	// else
	int nCol = -1, nHit = ListHitTestItem(point, FALSE, nCol);

	if (m_bReadOnly || (GetListColumnType(nCol) != WLCT_VALUE))
		return FALSE;

	CString sAllocTo(m_listHeader.GetItemText(nCol));

	return CWnd::GetParent()->SendMessage(WM_WLC_EDITTASKALLOCATIONS, (WPARAM)(LPCTSTR)sAllocTo, GetTaskID(nHit));
}

void CWorkloadCtrl::SetOverlapColor(COLORREF crOverlap)
{
	SetColor(m_crOverlap, crOverlap);
}

void CWorkloadCtrl::SetAllocationColor(COLORREF crAllocation)
{
	SetColor(m_crAllocation, crAllocation);
}

void CWorkloadCtrl::EnableOverload(BOOL bEnable, double dOverloadValue, COLORREF crOverload)
{
	m_barChart.EnableOverload(bEnable, dOverloadValue, crOverload);
	m_lcColumnTotals.Invalidate();		
}

void CWorkloadCtrl::EnableUnderload(BOOL bEnable, double dUnderloadValue, COLORREF crUnderload)
{
	m_barChart.EnableUnderload(bEnable, dUnderloadValue, crUnderload);
	m_lcColumnTotals.Invalidate();
}

BOOL CWorkloadCtrl::SetBackgroundColor(COLORREF crBkgnd)
{
	if (!CTreeListCtrl::SetBackgroundColor(crBkgnd))
		return FALSE;
	
	if (m_lcTotalsLabels.GetSafeHwnd())
	{
		m_lcTotalsLabels.SetBkColor(m_crBkgnd);
		m_lcTotalsLabels.SetTextBkColor(m_crBkgnd);
	}

	return TRUE;
}

int CWorkloadCtrl::GetLargestVisibleDuration(HTREEITEM hti) const
{
	int nLongest = 0;

	if (hti)
	{
		DWORD dwTaskID = GetTaskID(hti);

		const WORKLOADITEM* pWI = NULL;
		GET_WI_RET(dwTaskID, pWI, 0);

		if (pWI->HasValidDates())
			nLongest = pWI->dtRange.GetWeekdayCount();
	}

	// children
	if (!hti || TCH().IsItemExpanded(hti))
	{
		HTREEITEM htiChild = m_tree.GetChildItem(hti);

		while (htiChild)
		{
			int nLongestChild = GetLargestVisibleDuration(htiChild);
			nLongest = max(nLongest, nLongestChild);

			htiChild = m_tree.GetNextItem(htiChild, TVGN_NEXT);
		}
	}

	return nLongest;
}

double CWorkloadCtrl::GetLargestVisibleTimeEstimate(HTREEITEM hti) const
{
	double dLargest = 0.0;

	if (hti)
	{
		DWORD dwTaskID = GetTaskID(hti);

		const WORKLOADITEM* pWI = NULL;
		GET_WI_RET(dwTaskID, pWI, 0);

		dLargest = pWI->dTimeEst;
	}

	// children
	if (!hti || TCH().IsItemExpanded(hti))
	{
		HTREEITEM htiChild = m_tree.GetChildItem(hti);

		while (htiChild)
		{
			double dLargestChild = GetLargestVisibleDuration(htiChild);
			dLargest = max(dLargest, dLargestChild);

			htiChild = m_tree.GetNextItem(htiChild, TVGN_NEXT);
		}
	}

	return dLargest;
}

CString CWorkloadCtrl::GetTreeItemColumnText(const WORKLOADITEM& wi, WLC_COLUMNID nCol) const
{
	CString sItem;

	switch (nCol)
	{
		case WLCC_TITLE:
			sItem = wi.sTitle;
			break;

		case WLCC_TASKID:
			sItem.Format(_T("%ld"), wi.dwTaskID);
			break;
			
		case WLCC_STARTDATE:
			if (wi.HasStartDate())
				sItem = FormatDate(wi.dtRange.m_dtStart);
			break;

		case WLCC_DUEDATE:
			if (wi.HasDueDate())
				sItem = FormatDate(wi.dtRange.m_dtEnd);
			break;

		case WLCC_TIMEEST:
			sItem = FormatTimeSpan(wi.dTimeEst, 1);
			break;

		case WLCC_DURATION:
			sItem = FormatTimeSpan(wi.dtRange.GetWeekdayCount(), 0);
			break;

		case WLCC_PERCENT:
			sItem.Format(_T("%d%%"), wi.nPercent);
			break;
	}

	return sItem;
}

CString CWorkloadCtrl::FormatDate(const COleDateTime& date, DWORD dwFlags) const
{
	dwFlags &= ~DHFD_ISO;
	dwFlags |= (HasOption(WLCF_DISPLAYISODATES) ? DHFD_ISO : 0);

	return CDateHelper::FormatDate(date, dwFlags);
}

CString CWorkloadCtrl::FormatTimeSpan(double dDays, int nDecimals)
{
	if (dDays == 0.0)
		return _T("");

	CString sValue = Misc::Format(dDays, nDecimals);

	if (nDecimals > 0)
		sValue.TrimRight(_T(".0"));

	return CEnString(IDS_TIMESPAN_FORMAT, sValue);
}

void CWorkloadCtrl::DrawTreeItem(CDC* pDC, HTREEITEM hti, const WORKLOADITEM& wi, BOOL bSelected, COLORREF crBack)
{
	int nNumCol = m_treeHeader.GetItemCount();

	for (int nCol = 0; nCol < nNumCol; nCol++)
		DrawTreeItemText(pDC, hti, nCol, wi, bSelected, crBack);
}

void CWorkloadCtrl::DrawTreeItemText(CDC* pDC, HTREEITEM hti, int nCol, const WORKLOADITEM& wi, BOOL bSelected, COLORREF crBack)
{
	CRect rItem;
	GetTreeItemRect(hti, nCol, rItem);

	if (rItem.Width() == 0)
		return;

	DrawItemDivider(pDC, rItem, TRUE, bSelected);

	WLC_COLUMNID nColID = GetTreeColumnID(nCol);
	BOOL bTitleCol = (nColID == WLCC_TITLE);

	// draw item background colour
	if (!bSelected && (crBack != CLR_NONE))
	{
		CRect rBack(rItem);

		if (bTitleCol)
			GetTreeItemRect(hti, WLCC_TITLE, rBack, TRUE); // label only
		
		// don't overwrite gridlines
		if (m_crGridLine != CLR_NONE)
			rBack.DeflateRect(0, 0, 1, 1);
		
		pDC->FillSolidRect(rBack, crBack);
	}
	
	if (rItem.Width() <= m_tree.MIN_COL_WIDTH)
		return;

	// draw text
	CString sItem = GetTreeItemColumnText(wi, nColID);

	if (!sItem.IsEmpty())
	{
		if (bTitleCol)
			rItem.DeflateRect(2, 2, 1, 0);
		else
			rItem.DeflateRect(LV_COLPADDING, 2, LV_COLPADDING, 0);

		// text color and alignment
		BOOL bLighter = FALSE; 
		UINT nFlags = (DT_LEFT | DT_VCENTER | DT_NOPREFIX | GraphicsMisc::GetRTLDrawTextFlags(m_tree));

		switch (nColID)
		{
		case WLCC_TITLE:
			nFlags |= DT_END_ELLIPSIS;
			break;

		case WLCC_TASKID:
		case WLCC_DURATION:
		case WLCC_TIMEEST:
			nFlags |= DT_RIGHT;
			break;
			
		case WLCC_STARTDATE:
		case WLCC_DUEDATE:
			{
				// Right-align if the column width can show the entire date
				// else keep left align to ensure day and month remain visible
				if (rItem.Width() >= pDC->GetTextExtent(sItem).cx)
					nFlags |= DT_RIGHT;
			}
			break;
			
		case WLCC_PERCENT:
			nFlags |= DT_CENTER;
			break;
		}

		COLORREF crText = GetTreeTextColor(wi, bSelected, bLighter);
		COLORREF crOldColor = pDC->SetTextColor(crText);
		HGDIOBJ hFontOld = pDC->SelectObject(GetTreeItemFont(hti, wi, nColID));
		
		pDC->SetBkMode(TRANSPARENT);
		pDC->DrawText(sItem, rItem, nFlags);
		pDC->SetTextColor(crOldColor);
		pDC->SelectObject(hFontOld);
	}

	// special case: drawing shortcut icon for reference tasks
	if (bTitleCol && wi.dwOrgRefID)
	{
		GetTreeItemRect(hti, nCol, rItem, TRUE);
		CPoint ptIcon(rItem.left, rItem.bottom - 32);

		ShellIcons::DrawIcon(pDC, ShellIcons::SI_SHORTCUT, ptIcon, true);
	}
}

HFONT CWorkloadCtrl::GetTreeItemFont(HTREEITEM hti, const WORKLOADITEM& wi, WLC_COLUMNID nCol)
{
	BOOL bStrikThru = (HasOption(WLCF_STRIKETHRUDONETASKS) && wi.bDone);
	BOOL bBold = ((nCol == WLCC_TITLE) && (m_tree.GetParentItem(hti) == NULL));
	
	return m_tree.Fonts().GetHFont(bBold, FALSE, FALSE, bStrikThru);
}

BOOL CWorkloadCtrl::GetTreeItemRect(HTREEITEM hti, int nCol, CRect& rItem, BOOL bText) const
{
	if (!CTreeListCtrl::GetTreeItemRect(hti, nCol, rItem, bText))
		return FALSE;

	ASSERT(GetTreeColumnID(nCol) != WLCC_NONE);

	if (m_bSavingToImage)
		rItem.OffsetRect(-1, 0);

	return TRUE;
}

CString CWorkloadCtrl::GetListItemColumnText(const WORKLOADITEM& wi, int nCol, int nDecimals, BOOL bSelected, COLORREF& crBack) const
{
	crBack = CLR_NONE;

	switch (GetListColumnType(nCol))
	{
	case WLCT_TOTAL:
		return wi.mapAllocatedDays.FormatTotalDays(nDecimals);
		
	case WLCT_VALUE:
		{
			int nAllocTo = (nCol - 1);
			ASSERT(nAllocTo < m_aAllocTo.GetSize());
			
			CString sAllocTo = m_aAllocTo[nAllocTo];
			CString sDays = wi.mapAllocatedDays.FormatDays(sAllocTo, nDecimals);

			if (!sDays.IsEmpty())
			{
				if ((m_crOverlap != CLR_NONE) && wi.mapAllocatedDays.IsOverlapping(sAllocTo))
				{
					crBack = m_crOverlap;
				}
				else if (!bSelected && (m_crAllocation != CLR_NONE))
				{
					crBack = m_crAllocation;
				}
			}

			return sDays;
		}
		break;
	}

	return _T("");
}

CString CWorkloadCtrl::GetListItemColumnTotal(const CMapAllocationTotals& mapTotals, int nCol, int nDecimals) const
{
	switch (GetListColumnType(nCol))
	{
	case WLCT_TOTAL:
		return mapTotals.FormatTotal(nDecimals);

	case WLCT_VALUE:
		{
			int nAllocTo = (nCol - 1);
			ASSERT(nAllocTo < m_aAllocTo.GetSize());

			CString sAllocTo = m_aAllocTo[nAllocTo];
			return mapTotals.Format(sAllocTo, nDecimals);
		}
		break;
	}

	return _T("");
}

void CWorkloadCtrl::DrawAllocationListItem(CDC* pDC, int nItem, const WORKLOADITEM& wi, BOOL bSelected)
{
	ASSERT(nItem != -1);
	int nNumCol = GetRequiredListColumnCount();

	for (int nCol = 1; nCol < nNumCol; nCol++)
	{
		CRect rColumn;
		m_list.GetSubItemRect(nItem, nCol, LVIR_BOUNDS, rColumn);
		
		DrawItemDivider(pDC, rColumn, TRUE, bSelected);
		rColumn.right--;
		rColumn.bottom--;

		COLORREF crBack = CLR_NONE;
		CString sDays = GetListItemColumnText(wi, nCol, 2, bSelected, crBack);
		
		if (!sDays.IsEmpty())
		{
			if (crBack != CLR_NONE)
			{
				if (bSelected)
				{
					GraphicsMisc::DrawRect(pDC, rColumn, CLR_NONE, crBack);
				}
				else
				{
					pDC->FillSolidRect(rColumn, crBack);
					pDC->SetTextColor(GraphicsMisc::GetBestTextColor(crBack));
				}
			}
			else
			{
				pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
			}

			rColumn.DeflateRect(LV_COLPADDING, 1, LV_COLPADDING, 0);
			pDC->DrawText(sDays, (LPRECT)(LPCRECT)rColumn, DT_CENTER);
		}
	}
}

void CWorkloadCtrl::DrawTotalsListItem(CDC* pDC, int nItem, const CMapAllocationTotals& mapTotals, int nDecimals)
{
	ASSERT(nItem != -1);
	int nNumCol = GetRequiredListColumnCount();
	
	for (int nCol = 1; nCol < nNumCol; nCol++)
	{
		CRect rColumn;
		m_lcColumnTotals.GetSubItemRect(nItem, nCol, LVIR_BOUNDS, rColumn);
			
		// Offset for allocation label horz scroll
		rColumn.OffsetRect(-m_list.GetScrollPos(SB_HORZ) - 1, 0);

		CString sValue = GetListItemColumnTotal(mapTotals, nCol, nDecimals);
			
		if (!sValue.IsEmpty())
		{
			if (nItem == ID_PERCENTLOADPERPERSON)
			{
				COLORREF crBack = CLR_NONE;

				switch (GetListColumnType(nCol))
				{
				case WLCT_VALUE:
					crBack = m_barChart.GetBkgndColor(mapTotals.Get(m_aAllocTo[nCol - 1]));
					break;
					
				case WLCT_TOTAL:
					crBack = m_barChart.GetBkgndColor(mapTotals.GetTotal());
					break;
				}

				if (crBack != CLR_NONE)
				{
					pDC->FillSolidRect(rColumn, crBack);
					pDC->SetTextColor(GraphicsMisc::GetBestTextColor(crBack));
				}
	
				sValue += '%';
			}
		
			CRect rText(rColumn);
			rText.DeflateRect(LV_COLPADDING, 1, LV_COLPADDING, 0);

			pDC->DrawText(sValue, (LPRECT)(LPCRECT)rText, DT_CENTER);
		}
			
		DrawItemDivider(pDC, rColumn, TRUE, FALSE);
	}
}

void CWorkloadCtrl::DrawTotalsHeader(CDC* pDC)
{
	int nNumCol = GetRequiredListColumnCount();
	
	for (int nCol = 1; nCol < nNumCol; nCol++)
	{
		CRect rColumn;
		m_lcColumnTotals.GetSubItemRect(ID_TOTALCOLUMNHEADER, nCol, LVIR_BOUNDS, rColumn);

		// Offset for allocation label horz scroll
		rColumn.OffsetRect(-m_list.GetScrollPos(SB_HORZ), 0);
		rColumn.right--;

		DrawListHeaderRect(pDC, rColumn, m_listHeader.GetItemText(nCol));
	}
}

COLORREF CWorkloadCtrl::GetTreeTextBkColor(const WORKLOADITEM& wi, BOOL bSelected, BOOL bAlternate) const
{
	COLORREF crTextBk = wi.GetTextBkColor(bSelected, HasOption(WLCF_TASKTEXTCOLORISBKGND));

	if (crTextBk == CLR_NONE)
	{
		if (!m_bSavingToImage && bAlternate && HasAltLineColor())
			crTextBk = m_crAltLine;
		else
			crTextBk = GetSysColor(COLOR_WINDOW);
	}
	
	return crTextBk;
}

COLORREF CWorkloadCtrl::GetTreeTextColor(const WORKLOADITEM& wi, BOOL bSelected, BOOL bLighter) const
{
	COLORREF crText = wi.GetTextColor(bSelected, HasOption(WLCF_TASKTEXTCOLORISBKGND));
	ASSERT(crText != CLR_NONE);

	if (!m_bSavingToImage)
	{
		if (bSelected)
		{
			crText = GraphicsMisc::GetExplorerItemTextColor(crText, GMIS_SELECTED, GMIB_THEMECLASSIC);
		}
		else if (bLighter)
		{
			crText = GraphicsMisc::Lighter(crText, 0.5);
		}
	}

	return crText;
}

void CWorkloadCtrl::DeleteItem(HTREEITEM hti)
{
	DWORD dwTaskID = GetTaskID(hti);
	ASSERT(dwTaskID);

	VERIFY(CTreeListCtrl::DeleteItem(hti));
	VERIFY(m_data.RemoveKey(dwTaskID));
}

WLC_COLUMNID CWorkloadCtrl::GetTreeColumnID(int nCol) const
{
	return (WLC_COLUMNID)m_treeHeader.GetItemData(nCol);
}

WLC_LISTCOLUMNTYPE CWorkloadCtrl::GetListColumnType(int nCol) const
{
	return (WLC_LISTCOLUMNTYPE)m_listHeader.GetItemData(nCol);
}

void CWorkloadCtrl::ResizeAttributeColumnsToFit(BOOL bForce)
{
	ResizeColumnsToFit(bForce);
}

void CWorkloadCtrl::RecalcListColumnsToFit()
{
	// Calc widest column first
	CClientDC dc(&m_list);
	CFont* pOldFont = GraphicsMisc::PrepareDCFont(&dc, m_list);

	int nMaxHeaderWidth = dc.GetTextExtent("100.0%").cx;
	int nCol = GetRequiredListColumnCount();

	while (nCol--)
	{
		CString sAllocTo = m_listHeader.GetItemText(nCol);

		int nHeaderWidth = dc.GetTextExtent(sAllocTo).cx;
		nMaxHeaderWidth = max(nMaxHeaderWidth, nHeaderWidth);

		int nTotalWidth = dc.GetTextExtent(m_mapTotalDays.Format(sAllocTo, 2)).cx;
		nMaxHeaderWidth = max(nMaxHeaderWidth, nTotalWidth);
	}
	nMaxHeaderWidth += (2 * HD_COLPADDING);

	// cleanup
	dc.SelectObject(pOldFont);

	// Resize all columns to that width (except first column)
	int nNumCols = GetRequiredListColumnCount();

	for (nCol = 1; nCol < nNumCols; nCol++)
	{
		if (GetListColumnType(nCol) == WLCT_EMPTY)
		{
			int nWidth = GraphicsMisc::ScaleByDPIFactor(10);

			m_listHeader.SetItemWidth(nCol, nWidth);
			m_lcColumnTotals.SetColumnWidth(nCol, nWidth);
		}
		else
		{
			m_listHeader.SetItemWidth(nCol, nMaxHeaderWidth);
			m_lcColumnTotals.SetColumnWidth(nCol, nMaxHeaderWidth);
		}
	}

	Resize();
	RemoveTotalsScrollbars();
}

void CWorkloadCtrl::RemoveTotalsScrollbars()
{
	if (m_lcColumnTotals.GetStyle() & (WS_VSCROLL | WS_HSCROLL))
		m_lcColumnTotals.ModifyStyle(WS_VSCROLL | WS_HSCROLL, 0, SWP_FRAMECHANGED);

	if (m_lcTotalsLabels.GetStyle() & (WS_VSCROLL | WS_HSCROLL))
		m_lcTotalsLabels.ModifyStyle(WS_VSCROLL | WS_HSCROLL, 0, SWP_FRAMECHANGED);
}

void CWorkloadCtrl::OnNotifySplitterChange(int nSplitPos)
{
	CTreeListCtrl::OnNotifySplitterChange(nSplitPos);

	ResyncTotalsPositions();
	UpdateWindow();
}

BOOL CWorkloadCtrl::HandleEraseBkgnd(CDC* pDC)
{
	CDialogHelper::ExcludeChild(&m_lcTotalsLabels, pDC);
	CDialogHelper::ExcludeChild(&m_lcColumnTotals, pDC);
	CDialogHelper::ExcludeChild(&m_barChart, pDC);
	
	return CTreeListCtrl::HandleEraseBkgnd(pDC);
}

int CWorkloadCtrl::CalcTreeColumnTextWidth(int nCol, CDC* pDC) const
{
	WLC_COLUMNID nColID = GetTreeColumnID(nCol);

	switch (nColID)
	{
	case WLCC_TITLE:
		break; // handled by base class

	case WLCC_TASKID:
		return pDC->GetTextExtent(Misc::Format(m_dwMaxTaskID)).cx;
		
	// rest of attributes are fixed width
	case WLCC_STARTDATE:
	case WLCC_DUEDATE: 
		{
			COleDateTime date(2015, 12, 31, 23, 59, 59);
			return GraphicsMisc::GetAverageMaxStringWidth(FormatDate(date), pDC);
		}
		break;

	case WLCC_DURATION:
		return pDC->GetTextExtent(FormatTimeSpan(GetLargestVisibleDuration(NULL), 0)).cx;

	case WLCC_TIMEEST:
		{
			int nWidthLargest = pDC->GetTextExtent(FormatTimeSpan(GetLargestVisibleTimeEstimate(NULL), 1)).cx;
			int nWidthSmallest = pDC->GetTextExtent(FormatTimeSpan(0.1, 1)).cx;

			return max(nWidthLargest, nWidthSmallest);
		}
		break;
		
	case WLCC_PERCENT: 
		return GraphicsMisc::GetAverageMaxStringWidth(_T("100%"), pDC);
		break;
	}

	ASSERT(0);
	return 0;
}

void CWorkloadCtrl::BuildListColumns()
{
	// once only
	if (m_listHeader.GetItemCount())
		return;

	// add empty column as placeholder so we can
	// easily replace the other columns without
	// losing all our items too
	LVCOLUMN lvc = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, 0 };
	
	m_list.InsertColumn(0, &lvc);
	m_listHeader.SetItemData(0, WLCT_EMPTY);
	m_lcColumnTotals.InsertColumn(0, &lvc);

	UpdateListColumns();
}

void CWorkloadCtrl::UpdateListColumns()
{
	Misc::SortArray(m_aAllocTo);

	int nNumCols = m_listHeader.GetItemCount();
	int nReqCols = GetRequiredListColumnCount();
	int nDiffCols = (nReqCols - nNumCols);

	if (nDiffCols > 0)
	{
		// add other columns
		LVCOLUMN lvc = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, 0 };

		lvc.cx = 100;
		lvc.fmt = LVCFMT_CENTER | HDF_STRING;
		lvc.pszText = _T("");
		lvc.cchTextMax = 50;
		
		for (int i = 0, nCol = nNumCols; i < nDiffCols; i++, nCol++)
		{
			m_list.InsertColumn(nCol, &lvc);
			m_lcColumnTotals.InsertColumn(nCol, &lvc);
		}
	}
	else if (nDiffCols < 0)
	{
		// don't delete first (hidden) column
		int i = nNumCols;

		while (i-- > nReqCols)
		{
			m_list.DeleteColumn(i);
			m_lcColumnTotals.DeleteColumn(i);
		}
	}
	ASSERT(m_listHeader.GetItemCount() == nReqCols);

	if (nDiffCols != 0)
		PostResize();

	// Update column header text and type
	for (int nCol = 1; nCol < nReqCols; nCol++)
	{
		if (nCol == (nReqCols - 2))
		{
			m_listHeader.SetItemData(nCol, WLCT_EMPTY);
			m_listHeader.SetItemText(nCol, _T(""));
		}
		else if (nCol == (nReqCols - 1))
		{
			m_listHeader.SetItemData(nCol, WLCT_TOTAL);
			m_listHeader.SetItemText(nCol, CEnString(IDS_COL_TOTAL));
		}
		else
		{
			m_listHeader.SetItemData(nCol, WLCT_VALUE);
			m_listHeader.SetItemText(nCol, m_aAllocTo[nCol - 1]);
		}
	}

	RecalcListColumnsToFit();
}

int CALLBACK CWorkloadCtrl::MultiSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CWorkloadCtrl* pThis = (CWorkloadCtrl*)lParamSort;
	const WORKLOADSORTCOLUMNS& sort = pThis->m_sort.multi;

	int nCompare = 0;

	for (int nCol = 0; ((nCol < 3) && (nCompare == 0)); nCol++)
	{
		if (sort.cols[nCol].nBy == TDCA_NONE)
			break;

		nCompare = pThis->CompareTasks(lParam1, lParam2, sort.cols[nCol]);
	}

	return nCompare;
}

int CALLBACK CWorkloadCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CWorkloadCtrl* pThis = (CWorkloadCtrl*)lParamSort;
	
	return pThis->CompareTasks(lParam1, lParam2, pThis->m_sort.single);
}

int CWorkloadCtrl::CompareTasks(DWORD dwTaskID1, DWORD dwTaskID2, const WORKLOADSORTCOLUMN& col) const
{
	int nCompare = 0;

	// Optimise for task ID
	if (col.nBy == WLCC_TASKID)
	{
		nCompare = (dwTaskID1 - dwTaskID2);
	}
	else
	{
		const WORKLOADITEM* pWI1 = GetWorkloadItem(dwTaskID1);
		const WORKLOADITEM* pWI2 = GetWorkloadItem(dwTaskID2);

		if (!pWI1 || !pWI2)
		{
			ASSERT(0);
			return 0;
		}

		switch (col.nBy)
		{
		case WLCC_TITLE:
			nCompare = Compare(pWI1->sTitle, pWI2->sTitle);
			break;

		case WLCC_STARTDATE:
			nCompare = CDateHelper::Compare(pWI1->dtRange.m_dtStart, pWI2->dtRange.m_dtStart, DHC_COMPARETIME);
			break;

		case WLCC_DUEDATE:
			nCompare = CDateHelper::Compare(pWI1->dtRange.m_dtEnd, pWI2->dtRange.m_dtEnd, DHC_COMPARETIME | DHC_NOTIMEISENDOFDAY);
			break;

		case WLCC_DURATION:
			nCompare = (pWI1->dtRange.GetWeekdayCount() - pWI2->dtRange.GetWeekdayCount());
			break;

		case WLCC_TIMEEST:
			{
				double dDiff = pWI1->dTimeEst - pWI2->dTimeEst;
				
				nCompare = ((dDiff > 0.0) ? 1 : ((dDiff < 0.0) ? -1 : 0));
			}
			break;

		case WLCC_PERCENT:
			nCompare = (pWI1->nPercent - pWI2->nPercent);
			break;

		case WLCC_NONE:
			nCompare = (pWI1->nPosition - pWI2->nPosition);
			break;

		case WLCC_ALLOCTO:
			{
				ASSERT(!m_sSortByAllocTo.IsEmpty());

				double dDays1 = pWI1->mapAllocatedDays.GetDays(m_sSortByAllocTo);
				double dDays2 = pWI2->mapAllocatedDays.GetDays(m_sSortByAllocTo);

				nCompare = (int)(dDays1 - dDays2);
			}
			break;

		default:
			ASSERT(0);
			break;
		}
	}

	return (col.bAscending ? nCompare : -nCompare);
}

bool CWorkloadCtrl::PrepareNewTask(ITaskList* pTaskList) const
{
	ITASKLISTBASE* pTasks = GetITLInterface<ITASKLISTBASE>(pTaskList, IID_TASKLISTBASE);

	if (pTasks == NULL)
	{
		ASSERT(0);
		return false;
	}

	// Set the start date to today and of duration 1 day
	HTASKITEM hNewTask = pTasks->GetFirstTask();
	ASSERT(hNewTask);

	COleDateTime dt = CDateHelper::GetDate(DHD_TODAY);
	time64_t tDate = 0;

	if (CDateHelper::GetTimeT64(dt, tDate))
	{
		pTasks->SetTaskStartDate64(hNewTask, tDate);
		pTasks->SetTaskDueDate64(hNewTask, tDate);
	}

	return true;
}

DWORD CWorkloadCtrl::HitTestTask(const CPoint& ptScreen) const
{
	HTREEITEM hti = HitTestItem(ptScreen);

	return GetItemData(hti);
}

DWORD CWorkloadCtrl::TreeHitTestTask(const CPoint& ptScreen, BOOL bScreen) const
{
	HTREEITEM hti = TreeHitTestItem(ptScreen, bScreen);
	
	return GetTaskID(hti);
}

DWORD CWorkloadCtrl::GetTaskID(HTREEITEM htiFrom) const
{
	return GetItemData(htiFrom);
}

DWORD CWorkloadCtrl::GetTaskID(int nItem) const
{
	return GetTaskID(GetTreeItem(nItem));
}

DWORD CWorkloadCtrl::GetNextTask(DWORD dwTaskID, IUI_APPCOMMAND nCmd) const
{
	HTREEITEM hti = FindTreeItem(m_tree, dwTaskID);
	
	if (!hti)
	{
		ASSERT(0);
		return 0L;
	}

	DWORD dwNextID(dwTaskID);

	switch (nCmd)
	{
	case IUI_GETNEXTTASK:
		{
			HTREEITEM htiNext = TCH().GetNextVisibleItem(hti);
			
			if (htiNext)
				dwNextID = GetTreeItemData(m_tree, htiNext);
		}
		break;
		
	case IUI_GETPREVTASK:
		{
			HTREEITEM htiPrev = TCH().GetPrevVisibleItem(hti);
			
			if (htiPrev)
				dwNextID = GetTreeItemData(m_tree, htiPrev);
		}
		break;
		
	case IUI_GETNEXTTOPLEVELTASK:
	case IUI_GETPREVTOPLEVELTASK:
		{
			HTREEITEM htiNext = TCH().GetNextTopLevelItem(hti, (nCmd == IUI_GETNEXTTOPLEVELTASK));
			
			if (htiNext)
				dwNextID = GetTreeItemData(m_tree, htiNext);
		}
		break;
		
	default:
		ASSERT(0);
	}
	
	return dwNextID;
}

BOOL CWorkloadCtrl::SaveToImage(CBitmap& bmImage)
{
	if (m_tree.GetCount() == 0)
		return FALSE;

	CClientDC dc(&m_tree);

	BOOL bTracked = m_treeHeader.IsItemTracked(0);

	// Resize tree width to suit title text width
	int nPrevTitleWidth = m_treeHeader.GetItemWidth(0);
	int nPrevTreeWidth = m_treeHeader.CalcTotalItemWidth();

	int nReqTitleWidth = CalcTreeColumnWidth(0, &dc);
	int nReqTreeWidth = (nReqTitleWidth + (nPrevTreeWidth - nPrevTitleWidth));

	m_treeHeader.SetItemWidth(0, nReqTitleWidth);
	Resize();

	CAutoFlag af(m_bSavingToImage, TRUE);
	CEnBitmap bmBase;

	BOOL bRes = CTreeListCtrl::SaveToImage(bmBase);

	if (bRes)
	{
		// Add totals and graph
		CEnBitmap bmLabels, bmTotals, bmChart;

		if (!CCopyListCtrlContents(m_lcTotalsLabels).DoCopy(bmLabels))
			return FALSE;

		// Manually resize the totals full width because it doesn't scroll
		int nReqWidth = m_listHeader.CalcTotalItemWidth();

		CRect rTotals = CDialogHelper::GetChildRect(&m_lcColumnTotals), rTemp(rTotals);
		rTemp.right = rTemp.left + nReqWidth;
		rTemp.bottom += 2; // else the bottom border is not always drawn

		m_lcColumnTotals.MoveWindow(rTemp);
		
		if (!CCopyListCtrlContents(m_lcColumnTotals).DoCopy(bmTotals))
			return FALSE;

		m_lcColumnTotals.MoveWindow(rTotals);

		if (!m_barChart.SaveToImage(bmChart))
			return FALSE;

		// Join them all the bits together
		CDC dcImage, dcParts;

		if (dcImage.CreateCompatibleDC(&dc) && dcParts.CreateCompatibleDC(&dc))
		{
			CSize sizeBase = bmBase.GetSize();
			CSize sizeLabels = bmLabels.GetSize();
			CSize sizeTotals = bmTotals.GetSize();
			CSize sizeChart = bmChart.GetSize();

			CSize sizeImage;

			sizeImage.cx = (sizeBase.cx + sizeChart.cx);
			sizeImage.cy = max((sizeBase.cy + sizeTotals.cy), sizeChart.cy);

			if (bmImage.CreateCompatibleBitmap(&dc, sizeImage.cx, sizeImage.cy))
			{
				CBitmap* pOldImage = dcImage.SelectObject(&bmImage);
				dcImage.FillSolidRect(0, 0, sizeImage.cx, sizeImage.cy, GetSysColor(COLOR_WINDOW));

				// Base-class
				CBitmap* pOldPart = dcParts.SelectObject(&bmBase);
				dcImage.BitBlt(0, 0, sizeBase.cx, sizeBase.cy, &dcParts, 0, 0, SRCCOPY);

				// Totals labels
				dcParts.SelectObject(bmLabels);
				dcImage.BitBlt(0, sizeBase.cy, sizeLabels.cx, sizeLabels.cy, &dcParts, 0, 0, SRCCOPY);

				// Column Totals
				dcParts.SelectObject(bmTotals);
				dcImage.BitBlt(sizeLabels.cx + 2, sizeBase.cy, sizeTotals.cx, sizeTotals.cy, &dcParts, 0, 0, SRCCOPY);

				// Draw vertical divider between labels and totals
				CRect rDivider(0, sizeBase.cy, (sizeLabels.cx + 1), (sizeBase.cy + sizeTotals.cy));
				DrawItemDivider(&dcImage, rDivider, TRUE, FALSE);

				// Draw vertical divider at end of totals
				rDivider.right = sizeBase.cx;
				DrawItemDivider(&dcImage, rDivider, TRUE, FALSE);

				// Bar chart
				dcParts.SelectObject(bmChart);
				dcImage.BitBlt(sizeBase.cx, 0, sizeChart.cx, sizeChart.cy, &dcParts, 0, 0, SRCCOPY);

				dcParts.SelectObject(pOldPart);
				dcImage.SelectObject(pOldImage);
			}
		}
	}
	
	// Restore title column width
	m_treeHeader.SetItemWidth(0, nPrevTitleWidth);
	m_treeHeader.SetItemTracked(0, bTracked);

	Resize();
	
	return bRes;
}

void CWorkloadCtrl::RefreshItemBoldState(HTREEITEM htiFrom, BOOL bAndChildren)
{
	if (htiFrom && (htiFrom != TVI_ROOT))
	{
		TCH().SetItemBold(htiFrom, (m_tree.GetParentItem(htiFrom) == NULL));
	}
	
	// children
	if (bAndChildren)
	{
		HTREEITEM htiChild = m_tree.GetChildItem(htiFrom);
		
		while (htiChild)
		{
			RefreshItemBoldState(htiChild);
			htiChild = m_tree.GetNextItem(htiChild, TVGN_NEXT);
		}
	}
}

BOOL CWorkloadCtrl::SetFont(HFONT hFont, BOOL bRedraw)
{
	if (!CTreeListCtrl::SetFont(hFont, bRedraw))
		return FALSE;

	CFont* pFont = CFont::FromHandle(hFont);

	m_lcColumnTotals.SetFont(pFont);
	m_lcTotalsLabels.SetFont(pFont);

	CString sFontName;
	int nFontSize = GraphicsMisc::GetFontNameAndPointSize(hFont, sFontName);

	m_barChart.SetFont(sFontName, nFontSize);

	return TRUE;
}

void CWorkloadCtrl::FilterToolTipMessage(MSG* pMsg)
{
	CTreeListCtrl::FilterToolTipMessage(pMsg);

	m_barChart.FilterToolTipMessage(pMsg);
}

void CWorkloadCtrl::SetReadOnly(BOOL bReadOnly)
{
	m_bReadOnly = bReadOnly;

	EnableDragAndDrop(!bReadOnly);
}
