/*   -*- Mode: C++ -*-
 * meegotouchcp-connman - connectivity plugin for duicontrolpanel
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
#include "technologybutton.h"

TechnologyButton::TechnologyButton(const QString &technology,
				   NetworkListModel *model,
				   QGraphicsItem *parent) :
  MButton(parent),
  m_technology(technology),
  m_model(model)
{
  setViewType(MButton::switchType);

  if (m_model->availableTechnologies().contains(m_technology)) {
    setCheckable(true);
  } else {
    setCheckable(false);
  }
  if (m_model->enabledTechnologies().contains(m_technology)) {
    setChecked(true);
  } else {
    setChecked(false);
  }
  connect(this, SIGNAL(toggled(bool)), this, SLOT(toggledSlot(bool)));

  connect(model, SIGNAL(technologiesChanged(const QStringList&,
					    const QStringList&,
					    const QStringList&)),
	  this, SLOT(technologiesChanged(const QStringList&,
					 const QStringList&,
					 const QStringList&)));

}

TechnologyButton::~TechnologyButton()
{
}

void TechnologyButton::technologiesChanged(const QStringList &availableTechnologies,
					   const QStringList &enabledTechnologies,
					   const QStringList &connectedTechnologies)
{
  Q_UNUSED(connectedTechnologies);
  const bool available = availableTechnologies.contains(m_technology);
  const bool enabled = enabledTechnologies.contains(m_technology);
  setCheckable(available);
  setChecked(enabled);
}

void TechnologyButton::toggledSlot(bool checked)
{
  if (checked) {
    m_model->enableTechnology(m_technology);
  } else {
    m_model->disableTechnology(m_technology);
  }
}
