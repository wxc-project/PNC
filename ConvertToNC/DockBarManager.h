#pragma once
#include "AcUiDialogPanel.h"
#include "PartListDlg.h"
#include "RevisionDlg.h"
#include "DockBarTemplate.h"

#ifdef __SUPPORT_DOCK_UI_
extern CAcUiDialogPanel *g_pPartListDockBar;	//¿ÉÍ£¿¿´°¿Ú
#endif
#ifndef __UBOM_ONLY_
CPartListDlg* GetPartListDlgPtr();
#endif
void DestoryPartListDockBar();
BOOL IsShowDisplayPartListDockBar();
void DisplayPartListDockBar();
void HidePartListDockBar();

#ifdef __UBOM_ONLY_
struct UBOM_DLG_ID { static const int DLG_ID = CRevisionDlg::IDD; };
struct UBOM_NAME_ID { static const int NAME_ID = 0; };
typedef CDlgDockBar<CRevisionDlg, UBOM_DLG_ID, UBOM_NAME_ID> CUBomDockBar;

extern CUBomDockBar g_ubomDocBar;
#endif