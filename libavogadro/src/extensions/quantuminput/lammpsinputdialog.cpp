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
	m_dimensionType(d3),
	m_xBoundaryType(p),
	m_yBoundaryType(p),
	m_zBoundaryType(p),

	m_atomStyle(full),
    m_output(),  m_dirty(false), m_warned(false)
  {
    ui.setupUi(this);
    // Connect the GUI elements to the correct slots
    connect(ui.titleLine, SIGNAL(editingFinished()),
        this, SLOT(setTitle()));

    //now for something useful
    connect(ui.unitsCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(setUnits(int)));
    connect(ui.atomStyleCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(setAtomStyle(int)));
    connect(ui.dimensionCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(setDimensionType(int)));
    connect(ui.xBoundaryCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(setXBoundaryType(int)));
    connect(ui.yBoundaryCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(setYBoundaryType(int)));
    connect(ui.zBoundaryCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(setZBoundaryType(int)));

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
    ui.atomStyleCombo->setCurrentIndex(7);
    ui.dimensionCombo->setCurrentIndex(1);
    ui.xBoundaryCombo->setCurrentIndex(0);
    ui.yBoundaryCombo->setCurrentIndex(0);
    ui.zBoundaryCombo->setCurrentIndex(0);

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
  void LammpsInputDialog::setAtomStyle(int n)
  {
    m_atomStyle = (LammpsInputDialog::atomStyle) n;
    ui.atomStyleCombo->setEnabled(true);
    updatePreviewText();
  }
  void LammpsInputDialog::setDimensionType(int n)
  {
    m_dimensionType = (LammpsInputDialog::dimensionType) n;
    ui.dimensionCombo->setEnabled(true);
    if(n==0)
    {
      setZBoundaryType(0);
      ui.zBoundaryCombo->setCurrentIndex(0);
      ui.zBoundaryCombo->setEnabled(false);
    }
    if(n==1)
    {
      ui.zBoundaryCombo->setEnabled(true);
    }
    updatePreviewText();
  }
  void LammpsInputDialog::setXBoundaryType(int n)
  {
    m_xBoundaryType = (LammpsInputDialog::boundaryType) n;
    ui.xBoundaryCombo->setEnabled(true);
    updatePreviewText();
  }
  void LammpsInputDialog::setYBoundaryType(int n)
  {
    m_yBoundaryType = (LammpsInputDialog::boundaryType) n;
    ui.yBoundaryCombo->setEnabled(true);
    updatePreviewText();
  }
  void LammpsInputDialog::setZBoundaryType(int n)
  {
    m_zBoundaryType = (LammpsInputDialog::boundaryType) n;
    //should be careful here
    //z boundary must be p for 2d!!!
    ui.zBoundaryCombo->setEnabled(true);
    updatePreviewText();
  }


  QString LammpsInputDialog::generateInputDeck()
  {
    // Generate an input deck based on the settings of the dialog
    QString buffer;
    QTextStream mol(&buffer);

    mol << "#LAMMPS Input file generated by Avogadro\n";
    mol << "# " << m_title << "\n\n";

    mol << "# Intialization\n";
    mol << "units          " << getUnitType(m_unitType) << "\n";
    mol << "dimension      " << getDimensionType(m_dimensionType) << "\n";
    mol << "boundary       "
      << getXBoundaryType(m_xBoundaryType) << " "
      << getYBoundaryType(m_yBoundaryType) << " "
      << getZBoundaryType(m_zBoundaryType) << "\n";
    mol << "atom_style     " << getAtomStyle(m_atomStyle) << "\n";
    mol << "pair_style     xxxxx\n";
    mol << "bond_style     xxxxx\n";
    mol << "angle_style    xxxxx\n";
    mol << "\n";

    mol << "# Atom Definition\n";
    mol << "read_data      xxxxx\n";
    mol << "\n";

    mol << "# Settings\n";
    mol << "pair_coeff     xxxxx\n";
    mol << "bond_coeff     xxxxx\n";
    mol << "angle_coeff    xxxxx\n";
    mol << "fix            xxxxx\n";
    mol << "velocity       xxxxx\n";
    mol << "\n";

    mol << "# Run the simulation\n";
    mol << "run            xxxxx\n";
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
      case u_electron:
        return "electron";
      default:
        return "lj";
    }
  }

  QString LammpsInputDialog::getAtomStyle(atomStyle t)
  {
    switch (t)
    {
      case angle:
	return "angle";
      case atomic:
	return "atomic";
      case bond:
	return "bond";
      case charge:
	return "charge";
      case dipole:
	return "dipole";
      case a_electron:
	return "electron";
      case ellipsoid:
	return "ellipsoid";
      case full:
	return "full";
      case line:
	return "line";
      case meso:
	return "meso";
      case molecular:
	return "molecular";
      case peri:
	return "peri";
      case sphere:
	return "sphere";
      case tri:
	return "tri";
      case wavepacket:
	return "wavepacket"; 
      default:
	return "full";
    }
  }
  QString LammpsInputDialog::getDimensionType(dimensionType t)
  {
    switch(t)
    {
      case d2:
	return "2d";
      case d3:
	return "3d";
      default:
	return "3d";
    }
  }
  QString LammpsInputDialog::getXBoundaryType(boundaryType t)
  {
    switch(t)
    {
      case p:
	return "p";
      case f:
	return "f";
      case s:
	return "s";
      case m:
	return "m";
      case fs:
	return "fs";
      case fm:
	return "fm";
      default:
	return "p";
    }
  }
  QString LammpsInputDialog::getYBoundaryType(boundaryType t)
  {
    switch(t)
    {
      case p:
	return "p";
      case f:
	return "f";
      case s:
	return "s";
      case m:
	return "m";
      case fs:
	return "fs";
      case fm:
	return "fm";
      default:
	return "p";
    }
  }
  QString LammpsInputDialog::getZBoundaryType(boundaryType t)
  {
    switch(t)
    {
      case p:
	return "p";
      case f:
	return "f";
      case s:
	return "s";
      case m:
	return "m";
      case fs:
	return "fs";
      case fm:
	return "fm";
      default:
	return "p";
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

