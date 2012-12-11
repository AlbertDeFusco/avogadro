/**********************************************************************
  FhiAimsInputDialog - Dialog for generating FhiAims input decks

  Copyright (C) 2012 Albert DeFusco

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

#include "fhiaimsinputdialog.h"

#include <avogadro/avospglib.h>
#include <avogadro/obeigenconv.h>
#include <avogadro/molecule.h>
#include <avogadro/atom.h>

#include <openbabel/mol.h>

#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QTextBrowser>
#include <QFileDialog>
#include <QMessageBox>
#include <QShowEvent>
#include <QSettings>
#include <QDebug>
#include <QProcess>
#include <QHash>
#include <QTableWidget>
#include <QSignalMapper>

#define ANG2BOHR 1.889725989
#define PI       3.141592654

using namespace OpenBabel;

namespace Avogadro
{

  FhiAimsInputDialog::FhiAimsInputDialog(QWidget *parent, Qt::WindowFlags f)
    : InputDialog(parent, f),
    //main
    m_title("Title"),

    //crystal structure tab
    m_crystalType(Display),

    // Rest
    m_dirty(false),
    m_warned(false)
  {
    ui.setupUi(this);
    // Connect the GUI elements to the correct slots
    // Basic Tab
    connect(ui.titleLine, SIGNAL(editingFinished()),
        this, SLOT(setTitle()));

    connect(ui.previewTextControl, SIGNAL(textChanged()),
        this, SLOT(previewEdited()));
    connect(ui.generateButton, SIGNAL(clicked()),
        this, SLOT(generateClicked()));
    connect(ui.resetButton, SIGNAL(clicked()),
        this, SLOT(resetClicked()));
    connect(ui.moreButton, SIGNAL(clicked()),
        this, SLOT(moreClicked()));
    connect(ui.enableFormButton, SIGNAL(clicked()),
        this, SLOT(enableFormClicked()));
    connect(ui.closeButton, SIGNAL(clicked()),
        this, SLOT(close()));

    //crystal structure tab
    connect(ui.crystalCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(setCrystalStructure(int)));
    connect(ui.previewTextGeometry, SIGNAL(textChanged()),
        this, SLOT(previewEdited()));

    QSettings settings;
    readSettings(settings);

    if(currentCell()==0)
      ui.crystalCombo->setEnabled(false);


    // Generate an initial preview of the input deck
    updatePreviewText();
  }


  FhiAimsInputDialog::~FhiAimsInputDialog()
  {
      QSettings settings;
      writeSettings(settings);
  }

  void FhiAimsInputDialog::setMolecule(Molecule *molecule)
  {
    // Disconnect the old molecule first...
    if (m_molecule) {
      disconnect(m_molecule, 0, this, 0);
    }

    m_molecule = molecule;
    // Update the preview text whenever atoms are changed
    // this is done by first updating the hash tables that
    // control pseudopotential input
    connect(m_molecule, SIGNAL(atomRemoved(Atom *)),
            this, SLOT(updatePreviewText()));
    connect(m_molecule, SIGNAL(atomAdded(Atom *)),
            this, SLOT(updatePreviewText()));
    connect(m_molecule, SIGNAL(atomUpdated(Atom *)),
            this, SLOT(updatePreviewText()));
    updatePreviewText();

  }



  void FhiAimsInputDialog::showEvent(QShowEvent *)
  {
    updatePreviewText();
  }

  void FhiAimsInputDialog::updatePreviewText()
  {
    if (!isVisible())
      return;
    // Generate the input deck and display it
    if (m_dirty && !m_warned) {
      m_warned = true;
      QMessageBox msgBox;

      msgBox.setWindowTitle(tr("FhiAims Input Deck Generator Warning"));
      msgBox.setText(tr("Would you like to update the preview text, "
            "losing all changes made in the FhiAims input "
            "deck preview pane?"));
      msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

      switch (msgBox.exec()) {
        case QMessageBox::Yes:
          // yes was clicked
          deckDirty(false);
          ui.previewTextControl->setText(generateInputDeck());
          ui.previewTextGeometry->setText(generateInputGeometry());
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
      ui.previewTextControl->setText(generateInputDeck());
      ui.previewTextGeometry->setText(generateInputGeometry());
      ui.previewTextControl->document()->setModified(false);
      ui.previewTextGeometry->document()->setModified(false);
    }
  }

  void FhiAimsInputDialog::resetClicked()
  {

    // Reset the form to defaults
    deckDirty(false);
    // Basic Tab
    //ui.titleLine->setText("");
    //setTitle();
    ui.crystalCombo->setCurrentIndex(0);
    ui.previewTextControl->document()->setModified(false);
    //ui.previewTextGeometry->document()->setModified(false);
  }

  void FhiAimsInputDialog::generateClicked()
  {
    saveInputFiles(ui.previewTextControl->toPlainText(),
                   ui.previewTextGeometry->toPlainText());
                          //tr("FhiAims Input Deck"), QString("in"));
  }

  QString FhiAimsInputDialog::saveInputFiles(QString inputDeck, QString inputGeometry)
  {
    // Fragment copied from InputDialog
    // Try to set default save path for dialog using the next sequence:
    //  1) directory of current file (if any);
    //  2) directory where previous deck was saved;
    //  3) $HOME
    QFileInfo defaultFile(m_molecule->fileName());
    QString defaultPath = defaultFile.canonicalPath();
    if(m_savePath == "") {
      if (defaultPath.isEmpty())
        defaultPath = QDir::homePath();
    } else {
      defaultPath = m_savePath;
    }

    QString inputPath = QFileDialog::getExistingDirectory(this,tr("Save Input Files"), defaultPath,QFileDialog::ShowDirsOnly);

    //QString defaultFileName = defaultPath + "/control.in"; // + defaultFile.baseName();
    //QString fileName = QFileDialog::getSaveFileName(this, tr("Save Input Files"),
      //defaultFileName, fileType + " (*." + ext + ")");


    if(inputPath == "")
      return inputPath;

    QString fileName = inputPath + "/control.in";
    QFile inputFile(fileName);
    if(!inputFile.open(QIODevice::WriteOnly | QIODevice::Text)) return QString();
// end of copied

// Fragment copied from InputDialog
    inputFile.write(inputDeck.toLocal8Bit()); // prevent troubles in Windows
    inputFile.close(); // flush buffer!
    m_savePath = QFileInfo(inputFile).absolutePath();

    fileName = inputPath + "/geometry.in";
    QFile geometryFile(fileName);
    if(!geometryFile.open(QIODevice::WriteOnly | QIODevice::Text)) return QString();
// end of copied

// Fragment copied from InputDialog
    geometryFile.write(inputGeometry.toLocal8Bit()); // prevent troubles in Windows
    geometryFile.close(); // flush buffer!
    m_savePath = QFileInfo(geometryFile).absolutePath();

    return inputPath;
  }

  void FhiAimsInputDialog::moreClicked()
  {
    // If the more button is clicked hide/show the preview text
    if (ui.previewTextControl->isVisible()) {
      ui.previewTextControl->hide();
      ui.previewTextGeometry->hide();
      ui.moreButton->setText(tr("Show Preview"));
    }
    else {
      ui.previewTextControl->show();
      ui.previewTextGeometry->show();
      ui.moreButton->setText(tr("Hide Preview"));
    }
  }

  void FhiAimsInputDialog::enableFormClicked()
  {
    updatePreviewText();
  }

  void FhiAimsInputDialog::previewEdited()
  {
    // Determine if the preview text has changed from the form generated
    if(ui.previewTextControl->toPlainText() != generateInputDeck())
      deckDirty(true);
    else
      deckDirty(false);
    /*if(ui.previewTextGeometry->toPlainText() != generateInputGeometry())
      deckDirty(true);
    else
      deckDirty(false);*/
  }

  void FhiAimsInputDialog::setTitle()
  {
    m_title = ui.titleLine->text();
    updatePreviewText();
  }

  //crystal structure tab slots
  void FhiAimsInputDialog::setCrystalStructure(int n)
  {
    m_crystalType = (FhiAimsInputDialog::crystalType) n;
    updatePreviewText();
  }

  //crystal structure tab output
  QString FhiAimsInputDialog::getCrystalStructure(crystalType t)
  {
    switch (t)
    {
      case Display:
        {
          QString buffer = getDisplayedCrystal();
          return buffer;
        }
      case Primitive:
        {
          QString buffer = generatePrimitiveLattice();
          return buffer;
        }
      default:
        {
          QString buffer = getDisplayedCrystal();
          return buffer;
        }
    }
  }

  QString FhiAimsInputDialog::getDisplayedCrystal()
  {
    //prepare memory
    QList<Eigen::Vector3d> fcoords;
    QList<unsigned int> atomicNums;
    Eigen::Matrix3d cellMatrix,fracMatrix;
    Spglib::prepareMolecule(m_molecule, currentCell(), &fcoords, &atomicNums, &cellMatrix);
    QString buffer,tmp;

    /*buffer = "ATOMIC_SPECIES\n";
    for (itr=atomTypes.begin();itr!=atomTypes.end();++itr)
    {
      QString symbol= OpenBabel::etab.GetSymbol( itr.key() );
      if(!atomPseudo.contains(symbol))
        initializePseudo(symbol);
      tmp.sprintf("%3s %9.4f %s\n",
          symbol.toStdString().c_str(),
          atomMass[itr.key()],
          atomPseudo[symbol].toStdString().c_str());
      buffer.append(tmp);
    }*/

    //Crystal means x_{frac} = L^-1*x_{cart},
    //which I think corresponds to other people's definition
    //of fractional coordinates.
    numAtoms = atomicNums.size();
    //buffer.append("ATOMIC_POSITIONS crystal\n");
    for(int iatom = 0;iatom<atomicNums.size();iatom++)
    {
      Eigen::Vector3d fracCoord;
      fracCoord=fcoords[iatom];
      tmp.sprintf("atom_frac %12.6f %12.6f %12.6f %3s\n",
          fracCoord.x(),
          fracCoord.y(),
          fracCoord.z(),
          OpenBabel::etab.GetSymbol( atomicNums.at(iatom) ));
      buffer.append(tmp);
    }
    //cubic is default
    //hexagonal is the only other option and needs to be tested
    //buffer.append("CELL_PARAMETERS cubic\n");
    buffer.append("#\n# unit cell\n#\n");
    for(int i=0;i<3;i++)
    {
      tmp.sprintf("lattice_vector %12.6f %12.6f %12.6f\n",
          cellMatrix(i,0), ///currentCell()->GetA(),
          cellMatrix(i,1), ///currentCell()->GetA(),
          cellMatrix(i,2)); ///currentCell()->GetA());
      buffer.append(tmp);
    }
    return buffer;
  }

  QString FhiAimsInputDialog::generatePrimitiveLattice()
  {
    QString buffer;

    //prepare memory to get primitive lattice and basis
    QList<Eigen::Vector3d> fcoords;
    QList<unsigned int> atomicNums;
    Eigen::Matrix3d cellMatrix;
    //It would be great to fill the unit cell first
    //uca->FillUnitCell(*obmol);
    Spglib::prepareMolecule(m_molecule, currentCell(), &fcoords, &atomicNums, &cellMatrix);

    const double tolerance=1e-5;
    const bool refine=true;
    unsigned int spg=Spglib::getPrimitive(&fcoords, &atomicNums, &cellMatrix,tolerance,refine);
    if(spg == 0) {
      qDebug() << "crap, no space group.";
      buffer = "---- ERROR: Spacegroup symmetry not found. ----\n";
      return buffer;
    }
    else
    {
      /*buffer = "ATOMIC_SPECIES\n";
      for (itr=atomTypes.begin();itr!=atomTypes.end();++itr)
      {
        QString tmp;
        QString symbol= OpenBabel::etab.GetSymbol( itr.key() );
        if(!atomPseudo.contains(symbol))
          initializePseudo(symbol);
        tmp.sprintf("%3s %9.4f %s\n",
            symbol.toStdString().c_str(),
            atomMass[itr.key()],
            atomPseudo[symbol].toStdString().c_str());
        buffer.append(tmp);
      }*/
      //Crystal means x_{frac} = L^-1*x_{cart},
      //which I think corresponds to other people's definition
      //of fractional coordinates.
      numAtoms = atomicNums.size();
      QString tmp;
      //buffer.append("ATOMIC_POSITIONS crystal\n");
      for(int iatom = 0;iatom<atomicNums.size();iatom++)
      {
        Eigen::Vector3d fracCoord;
        fracCoord=fcoords[iatom];
        tmp.sprintf("atom_frac %12.6f %12.6f %12.6f %3s\n",
            fracCoord.x(),
            fracCoord.y(),
            fracCoord.z(),
          OpenBabel::etab.GetSymbol( atomicNums.at(iatom) ));
        buffer.append(tmp);
      }
      //cubic is default
      //hexagonal is the only other option and needs to be tested
      //buffer.append("CELL_PARAMETERS cubic\n");
      buffer.append("#\n# unit cell\n#\n");
      for(int i=0;i<3;i++)
      {
        tmp.sprintf("lattice_vector %12.6f %12.6f %12.6f\n",
            cellMatrix(i,0), ///currentCell()->GetA(),
            cellMatrix(i,1), ///currentCell()->GetA(),
            cellMatrix(i,2)); ///currentCell()->GetA());
          buffer.append(tmp);
      }
      return buffer;
    }
  }

  QString FhiAimsInputDialog::generateInputDeck()
  {
    if (!m_molecule || m_molecule->numAtoms() == 0) {
      return QString("");
    }
    // Generate an input deck based on the settings of the dialog
    QString buffer;
    QTextStream mol(&buffer);

    // Title line
    mol << "# " << m_title << "\n";
    mol << "# fhiaims input generated by Avogadro\n#\n";

    // If we have a molecule, invent the box it will live in
    // TODO: check the box is big enough: get molecule size and add a bit in
    // each direction, or make it a ui input.


    //because &system needs to know the number and types of the atoms,
    //it must be called after the crystal sctructure selector, but
    //it might be better to print it second.

    return buffer;
  }

  QString FhiAimsInputDialog::generateInputGeometry()
  {
    if (!m_molecule || m_molecule->numAtoms() == 0) {
      return QString("");
    }
    // Generate an input deck based on the settings of the dialog
    QString buffer;
    QTextStream mol(&buffer);
    QString buffer2;

    // Title line
    mol << "# " << m_title << "\n";
    mol << "# fhiaims input coordinates generated by Avogadro\n#\n";

    // If we have a molecule, invent the box it will live in
    // TODO: check the box is big enough: get molecule size and add a bit in
    // each direction, or make it a ui input.


    //because &system needs to know the number and types of the atoms,
    //it must be called after the crystal sctructure selector, but
    //it might be better to print it second.


    OBUnitCell *uc = m_molecule->OBUnitCell();
    if(currentCell()!=0)
      buffer2 = getCrystalStructure(m_crystalType);
    else
    {
      ui.crystalCombo->setEnabled(false);
      QString tmp;
      //buffer.append("ATOMIC_POSITIONS crystal\n");
      foreach (Atom *atom, m_molecule->atoms()) {
        int atomicNumber = atom->atomicNumber();
        tmp.sprintf("atom  %13.8f  %13.8f  %13.8f  %s\n",
            atom->pos()->x(), atom->pos()->y(), atom->pos()->z() ,
            OpenBabel::etab.GetSymbol( atomicNumber ));
        buffer2.append(tmp);
      }
    }

    mol << buffer2;

    return buffer;
  }

  void FhiAimsInputDialog::deckDirty(bool dirty)
  {
    m_dirty = dirty;

    ui.titleLine->setEnabled(!dirty);

    ui.crystalCombo->setEnabled(!dirty);
    ui.enableFormButton->setEnabled(dirty);
  }

  void FhiAimsInputDialog::readSettings(QSettings& settings)
  {
    m_savePath = settings.value("fhiaims/savepath").toString();
  }

  void FhiAimsInputDialog::writeSettings(QSettings& settings) const
  {
    settings.setValue("fhiaims/savepath", m_savePath);
  }
}
