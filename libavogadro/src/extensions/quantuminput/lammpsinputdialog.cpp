/**********************************************************************
  LammpsInputDialog - Dialog for generating LAMMPS input files

  Albert DeFusco 2011

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  Avogadro is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Avogadro is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 **********************************************************************/

#include "lammpsinputdialog.h"

#include <avogadro/molecule.h>
#include <avogadro/atom.h>

#include <openbabel/mol.h>

#include <QString>
#include <QTextStream>
//#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

using namespace OpenBabel;

namespace Avogadro
{
  LammpsInputDialog::LammpsInputDialog(QWidget *parent, Qt::WindowFlags f)
    : InputDialog(parent, f), 
	m_unitType(real),
    m_output(),  m_dirty(false), m_warned(false)
  {
    ui.setupUi(this);
    // Connect the GUI elements to the correct slots
    connect(ui.titleLine, SIGNAL(editingFinished()),
        this, SLOT(setTitle()));

    //now for something useful
    connect(ui.unitsCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(setUnits(int)));

    connect(ui.previewText, SIGNAL(cursorPositionChanged()),
        this, SLOT(previewEdited()));
    connect(ui.generateButton, SIGNAL(clicked()),
        this, SLOT(generateClicked()));
    connect(ui.resetButton, SIGNAL(clicked()),
        this, SLOT(resetClicked()));
    
    connect(ui.moreButton, SIGNAL(clicked()),
        this, SLOT(moreClicked()));
    connect(ui.enableFormButton, SIGNAL(clicked()),
        this, SLOT(enableFormClicked()));

    QSettings settings;
    readSettings(settings);
    
    // Generate an initial preview of the input deck
    updatePreviewText();
  }

  LammpsInputDialog::~LammpsInputDialog()
  {
      QSettings settings;
      writeSettings(settings);
  }


  void LammpsInputDialog::showEvent(QShowEvent *)
  {
    updatePreviewText();
  }

  void LammpsInputDialog::updatePreviewText()
  {
    if (!isVisible())
      return;
    // Generate the input deck and display it
    if (m_dirty && !m_warned) {
      m_warned = true;
      QMessageBox msgBox;

      msgBox.setWindowTitle(tr("Lammps Input Deck Generator Warning"));
      msgBox.setText(tr("Would you like to update the preview text, losing all changes made in the Lammps input deck preview pane?"));
      msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

      switch (msgBox.exec()) {
        case QMessageBox::Yes:
          // yes was clicked
          deckDirty(false);
          ui.previewText->setText(generateInputDeck());
          ui.previewText->document()->setModified(false);
          m_warned = false;
          break;
        case QMessageBox::No:
          // no was clicked
          m_warned = false;
          break;
        default:
          // should never be reached
          break;
      }
    }
    else if (!m_dirty) {
      ui.previewText->setText(generateInputDeck());
      ui.previewText->document()->setModified(false);
    }
  }

  void LammpsInputDialog::resetClicked()
  {
    // Reset the form to defaults
    deckDirty(false);
    //ui.calculationCombo->setCurrentIndex(1);
    //ui.theoryCombo->setCurrentIndex(3);
    //ui.basisCombo->setCurrentIndex(2);
    //ui.multiplicitySpin->setValue(0);
    //ui.chargeSpin->setValue(0);
    ui.unitsCombo->setCurrentIndex(1);
    ui.previewText->setText(generateInputDeck());
    ui.previewText->document()->setModified(false);
  }

  void LammpsInputDialog::generateClicked()
  {
    saveInputFile(ui.previewText->toPlainText(), tr("Lammps Input"), QString("lmp"));
  }

  void LammpsInputDialog::moreClicked()
  {
    // If the more button is clicked hide/show the preview text
    if (ui.previewText->isVisible()) {
      ui.previewText->hide();
      ui.moreButton->setText(tr("Show Preview"));
    }
    else {
      ui.previewText->show();
      ui.moreButton->setText(tr("Hide Preview"));
    }
  }

  void LammpsInputDialog::enableFormClicked()
  {
    updatePreviewText();
  }

  void LammpsInputDialog::previewEdited()
  {
    // Determine if the preview text has changed from the form generated
    if(ui.previewText->document()->isModified())
      deckDirty(true);
  }

  void LammpsInputDialog::setTitle()
  {
    m_title = ui.titleLine->text();
    updatePreviewText();
  }

  void LammpsInputDialog::setUnits(int n)
  {
    m_unitType = (LammpsInputDialog::unitType) n;
    ui.unitsCombo->setEnabled(true);
    updatePreviewText();
  }


  QString LammpsInputDialog::generateInputDeck()
  {
    // Generate an input deck based on the settings of the dialog
    QString buffer;
    QTextStream mol(&buffer);

    // Begin the job specification, including title
    mol << "#LAMMPS Input file:  " << m_title << "\n\n";
    mol << "units          " << getUnitType(m_unitType) << "\n\n";

    // Default output parameters
    //mol << "gprint,basis" << '\n';
    //mol << "gprint,orbital" << '\n';

    mol << '\n';


    return buffer;
  }

  QString LammpsInputDialog::getUnitType(unitType t)
  {
    // Translate the enum to text for the output generation
    switch (t)
    {
      case lj:
        return "lj";
      case real:
        return "real";
      case metal:
        return "metal";
      case si:
        return "si";
      case cgs:
        return "cgs";
      case electron:
        return "electron";
      default:
        return "lj";
    }
  }

  void LammpsInputDialog::deckDirty(bool dirty)
  {
    m_dirty = dirty;
    ui.titleLine->setEnabled(!dirty);
    //ui.calculationCombo->setEnabled(!dirty);
    //ui.theoryCombo->setEnabled(!dirty);
    //ui.basisCombo->setEnabled(!dirty);
    //ui.multiplicitySpin->setEnabled(!dirty);
    //ui.chargeSpin->setEnabled(!dirty);
    ui.enableFormButton->setEnabled(dirty);
  }

  void LammpsInputDialog::readSettings(QSettings& settings)
  {
    m_savePath = settings.value("lammps/savepath").toString();
  }
  
  void LammpsInputDialog::writeSettings(QSettings& settings) const
  {
    settings.setValue("lammps/savepath", m_savePath);
  }

}

