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

#ifndef OFFLINEBUTTON_H
#define OFFLINEBUTTON_H

#include "networklistmodel.h"
#include <MButton>

class OfflineButton : public MButton
{
  Q_OBJECT;

public:
  OfflineButton(NetworkListModel *networkListModel,
		QGraphicsItem *parent = 0);
  virtual ~OfflineButton();
			  
private slots:
  void toggledSlot(bool checked);
  void offlineModeChanged(const bool offlineMode);

private:
  OfflineButton();
  NetworkListModel *m_networkListModel;
};

#endif //OFFLINEBUTTON_H
