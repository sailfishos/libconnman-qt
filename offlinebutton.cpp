/*  -*- Mode: C++ -*-
 *
 * MCP - meego control panel, a settings application for meego handsets
 * Copyright © 2010, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St 
 * - Fifth Floor, Boston, MA 02110-1301 USA
 *
 */


#include "offlinebutton.h"


OfflineButton::OfflineButton(NetworkListModel *networkListModel,
			     QGraphicsItem *parent) :
  MButton(parent), m_networkListModel(networkListModel)
{
  setViewType(MButton::switchType);
  if (m_networkListModel->offlineMode()) {
    setChecked(true);
  } else {
    setChecked(false);
  }
  connect(this, SIGNAL(toggled(bool)), this, SLOT(toggledSlot(bool)));
  connect(m_networkListModel, SIGNAL(offlineModeChanged(bool)), 
	  this, SLOT(offlineModeChanged(bool)));
	  
}

OfflineButton::~OfflineButton()
{
}

void OfflineButton::toggledSlot(bool checked)
{
  m_networkListModel->setOfflineMode(checked);
}

void OfflineButton::offlineModeChanged(const bool offlineMode)
{
  setChecked(offlineMode);
}
