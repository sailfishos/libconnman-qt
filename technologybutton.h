/*   -*- Mode: C++ -*-
 * meegotouchcp-connman - connectivity plugin for duicontrolpanel
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
#ifndef  TECHNOLOGYBUTTON_H
#define  TECHNOLOGYBUTTON_H

#include "networklistmodel.h"
#include <MButton>

class TechnologyButton : public MButton
{
  Q_OBJECT;

public:
  TechnologyButton(const QString &technology,
		   NetworkListModel *model,
		   QGraphicsItem *parent = 0);
  virtual ~TechnologyButton();

private slots:
  void technologiesChanged(const QStringList &availableTechnologies,
			   const QStringList &enabledTechnologies,
			   const QStringList &connectedTechnologies);
  void toggledSlot(bool checked);

private:
  TechnologyButton();
  QString m_technology;
  NetworkListModel *m_model;
};


#endif //TECHNOLOGYBUTTON_H
