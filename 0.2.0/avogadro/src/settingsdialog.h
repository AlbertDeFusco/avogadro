/**********************************************************************
  SettingsDialog - Settings Dialog

  Copyright (C) 2007 Donald Ephraim Curtis

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.sourceforge.net/>

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

#ifndef __SETTINGSDIALOG_H
#define __SETTINGSDIALOG_H

#include "ui_settingsdialog.h"

class QAbstractButton;
namespace Avogadro {

  class MainWindow;
  class SettingsDialog : public QDialog
  {
    Q_OBJECT

    public:
      SettingsDialog(MainWindow *mainWindow);

    private:
      Ui::SettingsDialog ui;

      MainWindow *m_mainWindow;

    private Q_SLOTS:
      void buttonClicked(QAbstractButton *button);
      void saveValues();
      void loadValues();
  };

} // end namespace Avogadro

#endif
