/**********************************************************************
  Hydrogens - Hydrogens Plugin for Avogadro

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

#include "hydrogensextension.h"
#include <avogadro/primitive.h>
#include <avogadro/color.h>
#include <avogadro/glwidget.h>

#include <openbabel/obiter.h>

#include <QtGui>

using namespace std;
using namespace OpenBabel;

  namespace Avogadro {
    HydrogensExtension::HydrogensExtension(QObject *parent) : QObject(parent)
    {
      QAction *action = new QAction(this);
      action->setText("Add Hydrogens");
      m_actions.append(action);

      action = new QAction(this);
      action->setText("Remove Hydrogens");
      m_actions.append(action);
    }

    HydrogensExtension::~HydrogensExtension() 
    {
    }

    QList<QAction *> HydrogensExtension::actions() const
    {
      return m_actions;
    }

    QUndoCommand* HydrogensExtension::performAction(QAction *action, Molecule *molecule, GLWidget *widget, QTextEdit *messages)
    {

      QUndoCommand *undo = 0;
      int i = m_actions.indexOf(action);
      if( 0 <= i <= 1) {
        undo = new HydrogensCommand(molecule, (enum HydrogensCommand::Action) i);
      }

      return undo;
    }

    HydrogensCommand::HydrogensCommand(Molecule *molecule, enum Action action)
    {
      m_moleculeCopy = *molecule;
      m_molecule = molecule;
      m_action = action;
      switch(action) {
        case AddHydrogens:
          setText(QObject::tr("Add Hydrogens"));
          break;
        case RemoveHydrogens:
          setText(QObject::tr("Remove Hydrogens"));
          break;
      }
    }

    void HydrogensCommand::redo()
    {
      switch(m_action) {
        case AddHydrogens:
          m_molecule->AddHydrogens(false,true);
          break;
        case RemoveHydrogens:
          m_molecule->DeleteHydrogens();
          break;
      }
      m_molecule->update();
    }

    void HydrogensCommand::undo()
    {
      *m_molecule = m_moleculeCopy;
      m_molecule->update();
    }

    bool HydrogensCommand::mergeWith ( const QUndoCommand * command )
    {
      // we recieved another call of the same action
      return true;
    }

    int HydrogensCommand::id() const
    {
      return 4709537 + (int) m_action;
    }

  } // end namespace Avogadro

#include "hydrogensextension.moc"
Q_EXPORT_PLUGIN2(hydrogensextension, Avogadro::HydrogensExtensionFactory)
