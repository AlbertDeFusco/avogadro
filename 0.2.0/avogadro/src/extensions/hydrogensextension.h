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

#ifndef __HYDROGENSEXTENSION_H
#define __HYDROGENSEXTENSION_H


#include <openbabel/mol.h>

#include <avogadro/glwidget.h>
#include <avogadro/extension.h>

#include <QObject>
#include <QList>
#include <QString>
#include <QUndoCommand>

namespace Avogadro {

 class HydrogensExtension : public QObject, public Extension
  {
    Q_OBJECT

    public:
      //! Constructor
      HydrogensExtension(QObject *parent=0);
      //! Deconstructor
      virtual ~HydrogensExtension();

      //! \name Description methods
      //@{
      //! Plugin Name (ie Draw)
      virtual QString name() const { return QObject::tr("Hydrogens"); }
      //! Plugin Description (ie. Draws atoms and bonds)
      virtual QString description() const { return QObject::tr("Hydrogens Plugin"); };
      //! Perform Action
      virtual QList<QAction *> actions() const;
      virtual QUndoCommand* performAction(QAction *action, Molecule *molecule,
                                          GLWidget *widget, QTextEdit *messages=NULL);
      //@}

    private:
      QList<QAction *> m_actions;
  };

  class HydrogensExtensionFactory : public QObject, public ExtensionFactory
  {
    Q_OBJECT;
    Q_INTERFACES(Avogadro::ExtensionFactory);

    public:
    Extension *createInstance(QObject *parent = 0) { return new HydrogensExtension(parent); }
  };

  class HydrogensCommand : public QUndoCommand
  {
    public:
      enum Action {
        AddHydrogens = 0,
        RemoveHydrogens
      };

    public:
      HydrogensCommand(Molecule *molecule, enum Action action, GLWidget *widget);

      virtual void undo();
      virtual void redo();
      virtual bool mergeWith ( const QUndoCommand * command );
      virtual int id() const;

    private:
      Molecule *m_molecule;
      Molecule m_moleculeCopy;
      QList<Primitive*> m_SelectedList;
      enum Action m_action;
  };

} // end namespace Avogadro

#endif
