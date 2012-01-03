/**********************************************************************
  LammpsInputDialog - Dialog for generating LAMMPS input decks

  Copyright (C) 2008-2009 Marcus D. Hanwell

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

#ifndef LAMMPSINPUTDIALOG_H
#define LAMMPSINPUTDIALOG_H

#include "inputdialog.h"
#include "ui_lammpsinputdialog.h"

namespace Avogadro
{
  //class Molecule;
  class LammpsInputDialog : public InputDialog
  {
  Q_OBJECT

  public:
    explicit LammpsInputDialog(QWidget *parent = 0, Qt::WindowFlags f = 0 );
    ~LammpsInputDialog();

    //void setMolecule(Molecule *molecule);
    void readSettings(QSettings&);
    void writeSettings(QSettings&) const;
      

    enum unitType{lj, real, metal, si, cgs, u_electron};
    enum dimensionType{d2, d3};
    enum boundaryType{p, s, f, m, fs, fm};
    enum atomStyle{ angle,
	    atomic,
	    bond,
	    charge,
	    dipole,
	    a_electron,
	    ellipsoid,
	    full,
	    line,
	    meso,
	    molecular,
	    peri,
	    sphere,
	    tri,
	    wavepacket};
    enum waterPotential{ NONE, SPC, SPCE};

    //enum calculationType{SP, OPT, FREQ};
    //enum theoryType{RHF, MP2, B3LYP, CCSD, CCSDT};
    //enum basisType{STO3G, B321G, B631Gd, B631Gdp, B631plusGd, B6311Gd, ccpVDZ, ccpVTZ, AUGccpVDZ, AUGccpVTZ};
    //enum coordType{CARTESIAN, ZMATRIX, ZMATRIX_COMPACT};

  protected:
    /**
     * Reimplemented to update the dialog when it is shown
     */
    void showEvent(QShowEvent *event);

  private:
    Ui::LammpsInputDialog ui;
//    Molecule* m_molecule;

    // Internal data structure for the simulation
    //bool m_2009;
    //calculationType m_calculationType;
    //theoryType m_theoryType;
    //basisType m_basisType;
    //int m_multiplicity;
    //int m_charge;
    //int m_num_elec;

    //QString m_title;
    QString m_readData;
    unitType m_unitType;
    dimensionType m_dimensionType;
    boundaryType m_xBoundaryType;
    boundaryType m_yBoundaryType;
    boundaryType m_zBoundaryType;
    atomStyle m_atomStyle;
    waterPotential m_waterPotential;

    QString m_output;
    coordType m_coordType;
    bool m_dirty;
    bool m_warned;
    bool readData;

    // Generate an input deck as a string
    QString generateInputDeck();
    QString getUnitType(unitType t);
    QString getAtomStyle(atomStyle t);
    QString getDimensionType(dimensionType t);
    QString getXBoundaryType(boundaryType t);
    QString getYBoundaryType(boundaryType t);
    QString getZBoundaryType(boundaryType t);
    QString getWaterPotential(waterPotential t);
    // Translate enums to strings
    //QString getCalculationType(calculationType t);
    //QString getWavefunction(void);
    //QString getTheoryType(theoryType t);
    //QString getBasisType(basisType t);

    // Enable/disable form elements
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
    void setReadData();
    //void setVersion(int);
    //void setCalculation(int);
    //void setTheory(int);
    //void setBasis(int);
    //void setMultiplicity(int);
    //void setCharge(int);
    //void setCoords(int);

    void setUnits(int);
    void setAtomStyle(int);
    void setDimensionType(int);
    void setXBoundaryType(int);
    void setYBoundaryType(int);
    void setZBoundaryType(int);

    void setWaterPotential(int);

  };
}

#endif
