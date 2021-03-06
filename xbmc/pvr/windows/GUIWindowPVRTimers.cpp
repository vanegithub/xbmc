/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIWindowPVRTimers.h"

#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "GUIWindowPVR.h"

CGUIWindowPVRTimers::CGUIWindowPVRTimers(CGUIWindowPVR *parent) :
  CGUIWindowPVRCommon(parent, PVR_WINDOW_TIMERS, CONTROL_BTNTIMERS, CONTROL_LIST_TIMERS)
{
}

void CGUIWindowPVRTimers::GetContextButtons(int itemNumber, CContextButtons &buttons) const
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  /* Check for a empty file item list, means only a
     file item with the name "Add timer..." is present */
  if (pItem->m_strPath == "pvr://timers/add.timer")
  {
    buttons.Add(CONTEXT_BUTTON_ADD, 19056);             /* new timer */
    if (m_parent->m_vecItems->Size() > 1)
    {
      buttons.Add(CONTEXT_BUTTON_SORTBY_NAME, 103);     /* sort by name */
      buttons.Add(CONTEXT_BUTTON_SORTBY_DATE, 104);     /* sort by date */
    }
  }
  else
  {
    buttons.Add(CONTEXT_BUTTON_EDIT, 19057);            /* edit timer */
    buttons.Add(CONTEXT_BUTTON_ADD, 19056);             /* new timer */
    buttons.Add(CONTEXT_BUTTON_ACTIVATE, 19058);        /* activate/deactivate */
    buttons.Add(CONTEXT_BUTTON_RENAME, 118);            /* rename timer */
    buttons.Add(CONTEXT_BUTTON_DELETE, 117);            /* delete timer */
    buttons.Add(CONTEXT_BUTTON_SORTBY_NAME, 103);       /* sort by name */
    buttons.Add(CONTEXT_BUTTON_SORTBY_DATE, 104);       /* sort by date */
    if (CPVRManager::Get()->HasMenuHooks(pItem->GetPVRTimerInfoTag()->ClientID()))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);    /* PVR client specific action */
  }
}

bool CGUIWindowPVRTimers::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  return OnContextButtonActivate(pItem.get(), button) ||
      OnContextButtonAdd(pItem.get(), button) ||
      OnContextButtonDelete(pItem.get(), button) ||
      OnContextButtonEdit(pItem.get(), button) ||
      OnContextButtonRename(pItem.get(), button) ||
      CGUIWindowPVRCommon::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRTimers::UpdateData(void)
{
  if (m_bIsFocusing)
    return;

  CLog::Log(LOGDEBUG, "CGUIWindowPVRTimers - %s - update window '%s'. set view to %d", __FUNCTION__, GetName(), m_iControlList);
  m_bIsFocusing = true;
  m_bUpdateRequired = false;
  m_parent->m_vecItems->Clear();
  m_parent->m_viewControl.SetCurrentView(m_iControlList);
  m_parent->m_vecItems->m_strPath = "pvr://timers/";
  m_parent->Update(m_parent->m_vecItems->m_strPath);
  m_parent->m_vecItems->Sort(m_iSortMethod, m_iSortOrder);
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);
  m_parent->m_viewControl.SetSelectedItem(m_iSelected);

  m_parent->SetLabel(CONTROL_LABELHEADER, g_localizeStrings.Get(19025));
  m_parent->SetLabel(CONTROL_LABELGROUP, "");
  m_bIsFocusing = false;
}

bool CGUIWindowPVRTimers::OnClickButton(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedButton(message))
  {
    bReturn = true;
    CPVRManager::GetTimers()->Update();
    UpdateData();
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnClickList(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedList(message))
  {
    bReturn = true;
    int iAction = message.GetParam1();
    int iItem = m_parent->m_viewControl.GetSelectedItem();

    /* get the fileitem pointer */
    if (iItem < 0 || iItem >= m_parent->m_vecItems->Size())
      return bReturn;
    CFileItemPtr pItem = m_parent->m_vecItems->Get(iItem);

    /* process actions */
    if (iAction == ACTION_SHOW_INFO || iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      ActionShowTimer(pItem.get());
    else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      m_parent->OnPopupMenu(iItem);
    else if (iAction == ACTION_DELETE_ITEM)
      ActionDeleteTimer(pItem.get());
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonActivate(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_ACTIVATE)
  {
    bReturn = true;
    if (!item->HasPVRTimerInfoTag())
      return bReturn;

    CPVRTimerInfoTag *timer = item->GetPVRTimerInfoTag();
    int iLabelId;
    if (timer->Active())
    {
      timer->SetActive(false);
      iLabelId = 13106;
    }
    else
    {
      timer->SetActive(true);
      iLabelId = 305;
    }

    CGUIDialogOK::ShowAndGetInput(19033, 19040, 0, iLabelId);
    CPVRManager::GetTimers()->UpdateTimer(*item);
    UpdateData();
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonAdd(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_ADD)
  {
    bReturn = true;
    CPVRTimerInfoTag *newtimer = CPVRManager::GetTimers()->InstantTimer(NULL, false);
    CFileItem *item = new CFileItem(*newtimer);

    if (ShowTimerSettings(item))
    {
      CPVRManager::GetTimers()->AddTimer(*item);
      UpdateData();
    }
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_DELETE)
  {
    bReturn = true;
    if (!item->HasPVRTimerInfoTag())
      return bReturn;

    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return bReturn;
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 19040);
    pDialog->SetLine(1, "");
    pDialog->SetLine(2, item->GetPVRTimerInfoTag()->Title());
    pDialog->DoModal();

    if (!pDialog->IsConfirmed())
      return bReturn;

    if (CPVRManager::GetTimers()->DeleteTimer(*item))
      UpdateData();
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonEdit(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_EDIT)
  {
    bReturn = true;
    if (!item->HasPVRTimerInfoTag())
      return bReturn;

    if (ShowTimerSettings(item))
    {
      CPVRManager::GetTimers()->UpdateTimer(*item);
      UpdateData();
    }
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonRename(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_RENAME)
  {
    bReturn = true;
    if (!item->HasPVRTimerInfoTag())
      return bReturn;
    CPVRTimerInfoTag *timer = item->GetPVRTimerInfoTag();

    CStdString strNewName = timer->Title();
    if (CGUIDialogKeyboard::ShowAndGetInput(strNewName, g_localizeStrings.Get(19042), false))
    {
      if (CPVRManager::GetTimers()->RenameTimer(*item, strNewName))
        UpdateData();
    }
  }

  return bReturn;
}
