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

#ifndef FHIAIMSINPUTDIALOG_H
#define FHIAIMSINPUTDIALOG_H

#include <QProcess>
#include <QProgressDialog>
#include <QTextBrowser>
#include <QHash>
#include <QSignalMapper>
#include <QFileDialog>

#include "inputdialog.h"
#include "ui_fhiaimsinputdialog.h"

namespace Avogadro
{
  class Molecule;
  class FhiAimsInputDialog : public InputDialog
  {
    Q_OBJECT

  public:
    explicit FhiAimsInputDialog(QWidget *parent = 0, Qt::WindowFlags f = 0 );
    ~FhiAimsInputDialog();

    void setMolecule(Molecule *molecule);

    //crystal structure tab
    enum crystalType{Display, Primitive};

    //control tab
    enum xcFunctional{
      pwlda,
      pzlda,
      vwn,
      vwngauss,
      am05,
      blyp,
      pbe,
      pbeint,
      pbesol,
      rpbe,
      revpbe,
      b3lyp,
      hse03,
      hse06,
      pbe0,
      pbeso10,
      hf,
      mp2,
      screx,
      cohsex,
      pbe_vdw,
      revpbe_vdw
    };
    enum spinPolarization{snone,collinear};
    enum relativisticType{rnone};


    /**
     * Save the settings for this extension.
     * @param settings Settings variable to write settings to.
     */
    virtual void writeSettings(QSettings &settings) const;

    /**
     * Read the settings for this extension.
     * @param settings Settings variable to read settings from.
     */
    virtual void readSettings(QSettings &settings);

    inline OpenBabel::OBUnitCell* currentCell() const {
      return (m_molecule) ? m_molecule->OBUnitCell() : 0 ;}

  protected:
    /**
     * Reimplemented to update the dialog when it is shown
     */
    void showEvent(QShowEvent *event);

  private:
    Ui::FhiAimsInputDialog ui;

    // Internal data structure for the calculation
    // Basic Tab
    QString m_title;

    //crystal sctructure tab
    crystalType m_crystalType;

    //control tab
    xcFunctional m_xcFunctional;
    spinPolarization m_spinPolarization;
    relativisticType m_relativisticType;
    double m_charge;
    double m_initMoment;


    // Generate an input deck as a string
    QString generateInputDeck();
    QString generateInputGeometry();

    //crystal sctructure (geometry.in)
    QString getCrystalStructure(crystalType t);
    QString generatePrimitiveLattice();
    QString getDisplayedCrystal();
    QString saveInputFiles(QString inputDeck, QString inputGeometry);

    //input keywords (control.in)
    int numAtoms;
    QHash<int, int> atomTypes;
    QHash<int, int>::iterator itr;
    QHash<int, double> atomMass;
    QString generateSpeciesTag();

    QString getXCFunctional(xcFunctional t);
    QString getSpinPolarization(spinPolarization t);
    QString getRelativisticType(relativisticType t);

    // Enable/disable form elements
    bool m_dirty;
    bool m_warned;

    void deckDirty(bool);


  public Q_SLOTS:
    void updatePreviewText();

  private Q_SLOTS:
    //! Button Slots
    void resetClicked();
    void generateClicked();
    void enableFormClicked();
    void moreClicked();
    void previewEdited();

    void setTitle();

    //crystal structure tab
    void setCrystalStructure(int);

    //control tab
    void setXCFunctional(int);
    void setSpinPolarization(int);
    void setRelativisticType(int);
    void setCharge(double);
    void setMoment(double);



  };
}

#endif
