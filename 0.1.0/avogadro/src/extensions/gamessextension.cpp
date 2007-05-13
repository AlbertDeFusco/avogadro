/**********************************************************************
  GAMESS - GAMESS Input Deck Plugin for Avogadro

  Copyright (C) 2006 by Donald Ephraim Curtis
  Copyright (C) 2006 by Geoffrey R. Hutchison

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.sourceforge.net/>

  Some code is based on Open Babel
  For more information, see <http://openbabel.sourceforge.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "gamessextension.h"

#include <avogadro/primitive.h>
#include <avogadro/color.h>
#include <avogadro/glwidget.h>

#include <openbabel/obiter.h>

#include <QtGui>
#include <QFrame>
#include <QSpacerItem>

using namespace std;
using namespace OpenBabel;

  namespace Avogadro {
    GamessExtension::GamessExtension(QObject *parent) : QObject(parent), m_inputDialog(NULL), m_inputData(NULL)
    {
      QAction *action = new QAction(this);
      action->setText("GAMESS Input Generation");
      m_actions.append(action);
    }

    GamessExtension::~GamessExtension() 
    {
    }

    QList<QAction *> GamessExtension::actions() const
    {
      return m_actions;
    }

    QUndoCommand* GamessExtension::performAction(QAction *action, Molecule *molecule, GLWidget *widget, QTextEdit *messages)
    {

      qDebug() << "Perform Action";
      int i = m_actions.indexOf(action);
      switch(i)
      {
        case 0:
          if(!m_inputData)
          {
            m_inputData = new GamessInputData(molecule);
          }
          else
          {
            m_inputData->SetMolecule(molecule);
          }
          if(!m_inputDialog)
          {
            m_inputDialog = new GamessInputDialog(m_inputData);
            m_inputDialog->show();
          }
          else
          {
            m_inputDialog->setInputData(m_inputData);
            m_inputDialog->show();
          }
          break;
      }

      return 0;
    }
  }

#include "gamessextension.moc"

Q_EXPORT_PLUGIN2(gamessextension, Avogadro::GamessExtensionFactory)
