/**********************************************************************
  MainWindow - main window, menus, main actions

  Copyright (C) 2006 by Geoffrey R. Hutchison
  Some portions Copyright (C) 2006 by Donald E. Curtis

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

#include "config.h"

#include "mainwindow.h"
#include "aboutdialog.h"
#include "editcommands.h"

#include <avogadro/extension.h>
#include <avogadro/primitive.h>
#include <avogadro/toolgroup.h>
#include <avogadro/camera.h>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

#include <fstream>

#include <QClipboard>
#include <QColorDialog>
#include <QFileDialog>
#include <QGLFramebufferObject>
#include <QHeaderView>
#include <QMessageBox>
#include <QPluginLoader>
#include <QSettings>
#include <QStandardItem>
#include <QStackedLayout>
#include <QToolButton>
#include <QUndoStack>

using namespace std;
using namespace OpenBabel;

namespace Avogadro {

  class MainWindowPrivate 
  {
    public:
      MainWindowPrivate() : molecule(0), 
      undoStack(0), toolsFlow(0), toolSettingsStacked(0), messagesText(0),
      toolGroup(0) {}

      Molecule  *molecule;

      QString    fileName;
      QUndoStack *undoStack;

      FlowLayout *toolsFlow;
      QStackedLayout *toolSettingsStacked;

      QTextEdit *messagesText;

      QList<GLWidget *> glWidgets;
      GLWidget *glWidget;

      ToolGroup *toolGroup;
      QAction    *actionRecentFile[MainWindow::maxRecentFiles];
  };

  MainWindow::MainWindow() : QMainWindow(0), d(new MainWindowPrivate)
  {
    constructor();
    setFileName("");
  }

  MainWindow::MainWindow(const QString &fileName) : QMainWindow(0), d(new MainWindowPrivate)
  {
    constructor();
    loadFile(fileName);
  }

  void MainWindow::constructor()
  {
    ui.setupUi(this);

    readSettings();
    setAttribute(Qt::WA_DeleteOnClose);

    d->undoStack = new QUndoStack(this);
    d->toolsFlow = new FlowLayout(ui.toolsWidget);
    d->toolsFlow->setMargin(9);
    d->toolSettingsStacked = new QStackedLayout(ui.toolSettingsWidget);

    d->toolGroup = new ToolGroup(this);
    d->toolGroup->load();
    connect(d->toolGroup, SIGNAL(toolActivated(Tool *)), this, SLOT(setTool(Tool *)));

    d->toolSettingsStacked->addWidget(new QWidget);
    const QList<Tool *> tools = d->toolGroup->tools();
    int toolCount = tools.size();
    for(int i = 0; i < toolCount; i++)
    {
      QAction *action = tools.at(i)->activateAction();
      QToolButton *button = new QToolButton(ui.toolsWidget);
      button->setDefaultAction(action);
      d->toolsFlow->addWidget(button);
      QWidget *widget = tools.at(i)->settingsWidget();
      if(widget) {
        d->toolSettingsStacked->addWidget(widget);
        if(i == 0) {
          d->toolSettingsStacked->setCurrentIndex(1);
        }
      } 
    }


    d->glWidgets.append(ui.glWidget);
    d->glWidget = ui.glWidget;
    ui.glWidget->setToolGroup(d->toolGroup);
    ui.glWidget->setUndoStack(d->undoStack);

    QWidget *messagesWidget = new QWidget();
    QVBoxLayout *messagesVBox = new QVBoxLayout(messagesWidget);
    d->messagesText = new QTextEdit();
    d->messagesText->setReadOnly(true);
    messagesVBox->setMargin(3);
    messagesVBox->addWidget(d->messagesText);
    ui.bottomFlat->addTab(messagesWidget, tr("Messages"));

    ui.projectTree->header()->hide();

    loadExtensions();

    ui.enginesList->setGLWidget(ui.glWidget);
    ui.enginesList->setSettingsButton(ui.engineSettingsButton);

    connectUi();

    setMolecule(new Molecule(this));

    setFileName("");
    statusBar()->showMessage(tr("Ready."), 10000);

    qDebug() << "MainWindow Initialized" << endl;
  }

  void MainWindow::newFile()
  {
    MainWindow *other = new MainWindow;
    other->move(x() + 40, y() + 40);
    other->show();
  }

  void MainWindow::openFile()
  {
    QString fileName = QFileDialog::getOpenFileName(this);
    openFile(fileName);
  }

  void MainWindow::openFile(const QString &fileName)
  {
    if(!fileName.isEmpty()) {
      // check to see if we already have an open window
      MainWindow *existing = findMainWindow(fileName);
      if (existing) {
        existing->show();
        existing->raise();
        existing->activateWindow();
        return;
      }

      if(d->fileName.isEmpty() && !isWindowModified()) {
        loadFile(fileName);
      }
      else {
        MainWindow *other = new MainWindow();
        if(!other->loadFile(fileName)) {
          delete other;
          return;
        }
        other->move(x() + 40, y() + 40);
        other->show();
      }
    }
  }

  void MainWindow::openRecentFile()
  {
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
      openFile(action->data().toString());
    }
  }

  void MainWindow::closeFile()
  {
    if (maybeSave()) {
      d->undoStack->clear();
      setFileName("");
      setMolecule(new Molecule(this));
    }
  }


  void MainWindow::closeEvent(QCloseEvent *event)
  {
    if (maybeSave()) {
      writeSettings();
      event->accept();
    } else {
      event->ignore();
    }
  }

  bool MainWindow::save()
  {
    if (d->fileName.isEmpty()) {
      return saveAs();
    } else {
      return saveFile(d->fileName);
    }
  }

  bool MainWindow::saveAs()
  {
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
      return false;

//     if (QFile::exists(fileName)) {
//       QMessageBox::StandardButton ret;
//       ret = QMessageBox::warning(this, tr("Avogadro"),
//           tr("File %1 already exists.\n"
//             "Do you want to overwrite it?")
//           .arg(QDir::convertSeparators(fileName)),
//           QMessageBox::Yes | QMessageBox::Cancel);
//       if (ret == QMessageBox::Cancel) {
//         return false;
//       }
//     }
    setFileName(fileName);
    return saveFile(fileName);
  }

  void MainWindow::undoStackClean(bool clean)
  {
    setWindowModified(!clean);
  }

  void MainWindow::exportGraphics()
  {
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
      return;

    // render it (with alpha channel)
    if (!ui.glWidget->grabFrameBuffer(true).save(fileName))
    {
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot save file %1.").arg(fileName));
      return;
    }
  }

  void MainWindow::revert()
  {
    if (!d->fileName.isEmpty()) {
      loadFile(d->fileName);
    }
  }

  void MainWindow::documentWasModified()
  {
    setWindowModified(true);
  }

  bool MainWindow::maybeSave()
  {
    if (isWindowModified()) {
      QMessageBox::StandardButton ret;
      ret = QMessageBox::warning(this, tr("Avogadro"),
          tr("The document has been modified.\n"
            "Do you want to save your changes?"),
          QMessageBox::Save | QMessageBox::Discard
          | QMessageBox::Cancel);
      if (ret == QMessageBox::Save)
        return save();
      else if (ret == QMessageBox::Cancel)
        return false;
    }
    return true;
  }

  void MainWindow::clearRecentFiles()
  {
    QSettings settings; // already set up properly via main.cpp
    QStringList files;
    settings.setValue("recentFileList", files);

    updateRecentFileActions();
  }

  void MainWindow::about()
  {
    AboutDialog * about = new AboutDialog(this);
    about->show();
  }

  void MainWindow::setView( int index )
  {
    d->glWidget = d->glWidgets.at(index);
    ui.enginesList->setGLWidget(d->glWidget);
    d->glWidget->makeCurrent();
  }

  void MainWindow::paste()
  {
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = NULL;

    if(clipboard->supportsSelection())
    {
      mimeData = clipboard->mimeData(QClipboard::Selection);
    } else {
      mimeData = clipboard->mimeData();
    }

    OBConversion conv;
    OBFormat *pasteFormat;
    QString text;
    Molecule newMol;

    if (mimeData->hasFormat("chemical/x-mdl-molfile")) {
      pasteFormat = conv.FindFormat("mdl");
      
      text = mimeData->data("chemical/x-mdl-molfile");
    }
    else if(mimeData->hasText()) {
      pasteFormat = conv.FindFormat("xyz");
      
      text = mimeData->text();
    }

    if (text.length() == 0)
      return;

    if(!pasteFormat || !conv.SetInFormat(pasteFormat)) {
      statusBar()->showMessage(tr("Paste failed (format unavailable)."), 5000);
      return;
    }

    if(conv.ReadString(&newMol, text.toStdString()) && newMol.NumAtoms() != 0)
      {
        PasteCommand *command = new PasteCommand(d->molecule, newMol);
        d->undoStack->push(command);
      } else {
      statusBar()->showMessage(tr("Unable to paste molecule."));
    }
  }

  // Helper function -- works for "cut" or "copy"
  // FIXME add parameter to set "Copy" or "Cut" in messages
  QMimeData* MainWindow::prepareClipboardData(QList<Primitive*> selectedItems)
  {
    QMimeData *mimeData = new QMimeData;
    mimeData->setImageData(ui.glWidget->grabFrameBuffer(true));

    Molecule *moleculeCopy = d->molecule;
    if (!selectedItems.isEmpty()) { // we only want to copy the selected items
      moleculeCopy = new Molecule;
      std::map<OBAtom*, OBAtom*> AtomMap; // key is from old, value from new
      // copy atoms and create a map of atom indexes
      foreach(Primitive *item, selectedItems) {
        OBAtom *selected = static_cast<Atom*>(item);
        moleculeCopy->AddAtom(*selected);
        AtomMap[selected] = moleculeCopy->GetAtom(moleculeCopy->NumAtoms());
      }

      // use the atom map to map bonds
      map<OBAtom*, OBAtom*>::iterator posBegin, posEnd;
      FOR_BONDS_OF_MOL(b, d->molecule) {
        posBegin = AtomMap.find(b->GetBeginAtom());
        posEnd = AtomMap.find(b->GetEndAtom());
        // make sure both bonds are in the map (i.e. selected)
        if (posBegin != AtomMap.end() && posEnd != AtomMap.end()) {
          moleculeCopy->AddBond((posBegin->second)->GetIdx(),
                               (posEnd->second)->GetIdx(),
                               b->GetBO(), b->GetFlags());
        }
      } // end looping over bonds
    } // should now have a copy of our selected fragment

    OBConversion conv;
    // MDL format is used for main copy -- atoms, bonds, chirality
    // supports either 2D or 3D, generic data
    // CML is another option, but not as well tested in Open Babel
    OBFormat *mdlFormat = conv.FindFormat("mdl");
    if(!mdlFormat || !conv.SetOutFormat(mdlFormat)) {
      statusBar()->showMessage(tr("Copy failed (mdl unavailable)."), 5000);
      return NULL; // nothing in it yet
    }

    // write an MDL file first (with bond orders, radicals, etc.)
    // (CML might be better in the future, but this works well now)
    string output = conv.WriteString(moleculeCopy);
    QByteArray copyData(output.c_str(), output.length());
    mimeData->setData("chemical/x-mdl-molfile", copyData);
    
    // Copy XYZ coordinates to the text selection buffer
    OBFormat *xyzFormat = conv.FindFormat("xyz");
    if(!xyzFormat || !conv.SetOutFormat(xyzFormat)) {
      statusBar()->showMessage(tr("Copy failed (xyz unavailable)."), 5000);
      return NULL;
    }
    output = conv.WriteString(moleculeCopy);
    copyData = output.c_str();
    mimeData->setText(QString(copyData));

    // need to free our temporary moleculeCopy
    if (!selectedItems.isEmpty())
      delete moleculeCopy;

    return mimeData;
  }

  void MainWindow::cut()
  {
    QMimeData *mimeData = prepareClipboardData(d->glWidget->selectedItems());

    if (mimeData) {
      CutCommand *command = new CutCommand(d->molecule, mimeData,
                                           d->glWidget->selectedItems());
      d->undoStack->push(command);
    }
  }

  void MainWindow::copy()
  {
    QMimeData *mimeData = prepareClipboardData(d->glWidget->selectedItems());

    if (mimeData) {
      CopyCommand *command = new CopyCommand(mimeData);
      d->undoStack->push(command);
    }
  }

  void MainWindow::clear()
  {
    // clear the molecule or a set of atoms
    // has the inteligence to figure out based on the number of selected items
    ClearCommand *command = new ClearCommand(d->molecule, 
                                             d->glWidget->selectedItems());
    d->undoStack->push(command);
  }

  void MainWindow::selectAll()
  {
    QList<Primitive*> atoms;
    FOR_ATOMS_OF_MOL(a, d->molecule) {
      Atom *atom = static_cast<Atom*>(&*a);
      atoms.append(atom);
    }
    d->glWidget->setSelection(atoms, true);
    d->glWidget->update();
  }

  void MainWindow::selectNone()
  {
    d->glWidget->clearSelection();
    d->glWidget->update();
  }

  void MainWindow::newView()
  {
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    GLWidget *glWidget = new GLWidget();
    glWidget->setObjectName(tr("GLWidget"));
    layout->addWidget(glWidget);
    layout->setMargin(0);
    layout->setSpacing(6);
    d->glWidgets.append(glWidget);
    glWidget->setMolecule(d->molecule);
    glWidget->setToolGroup(d->toolGroup);
    glWidget->setUndoStack(d->undoStack);

    int index = ui.centralTab->addTab(widget, QString(""));
    ui.centralTab->setTabText(index, tr("View ") + QString::number(index));
    d->glWidgets.at(ui.centralTab->currentIndex())->makeCurrent();
    ui.actionCloseView->setEnabled(true);
  }

  void MainWindow::closeView()
  {
    QWidget *widget = ui.centralTab->currentWidget();
    foreach(QObject *object, widget->children())
    {
      GLWidget *glWidget = qobject_cast<GLWidget *>(object);
      if(glWidget)
      {
      int index = ui.centralTab->currentIndex();
      ui.centralTab->removeTab(index);
      for(int count=ui.centralTab->count(); index < count; index++) {
        QString text = ui.centralTab->tabText(index);
        if(!text.compare(tr("View ") + QString::number(index+1)))
        {
          ui.centralTab->setTabText(index, tr("View ") + QString::number(index));
        }
      }
      d->glWidgets.removeAll(glWidget);
      delete glWidget;
      ui.actionCloseView->setEnabled(ui.centralTab->count() != 1);
      }
    }
  }

  void MainWindow::centerView()
  {
    d->glWidget->camera()->initializeViewPoint();
    d->glWidget->update();
  }


  void MainWindow::fullScreen()
  {
    if (!this->isFullScreen()) {
      ui.actionFullScreen->setText(tr("Normal Size"));
      ui.fileToolBar->hide();
      statusBar()->hide();
      this->showFullScreen();
    } else {
      this->showNormal();
      ui.actionFullScreen->setText(tr("Full Screen"));
      ui.fileToolBar->show();
      statusBar()->show();
    }
  }

  void MainWindow::setBackgroundColor()
  {
    QColor current = ui.glWidget->background();
    ui.glWidget->setBackground(QColorDialog::getRgba(current.rgba(), NULL, this));
  }

  void MainWindow::setTool(Tool *tool)
  {
    if(tool->settingsWidget())
    {
      d->toolSettingsStacked->setCurrentWidget(tool->settingsWidget());
    } else {
      d->toolSettingsStacked->setCurrentIndex(0);
    }
  }

  void MainWindow::connectUi()
  {
    connect(ui.actionNew, SIGNAL(triggered()), this, SLOT(newFile()));
    connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
#ifdef Q_WS_MAC
    connect(ui.actionClose, SIGNAL(triggered()), this, SLOT(close()));
#else
    connect(ui.actionClose, SIGNAL(triggered()), this, SLOT(closeFile()));
#endif
    connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));
    connect(ui.actionSaveAs, SIGNAL(triggered()), this, SLOT(saveAs()));
    connect(ui.actionRevert, SIGNAL(triggered()), this, SLOT(revert()));
    connect(ui.actionExportGraphics, SIGNAL(triggered()), this, SLOT(exportGraphics()));
    ui.actionExportGraphics->setEnabled(QGLFramebufferObject::hasOpenGLFramebufferObjects());
#ifdef Q_WS_MAC
    connect(ui.actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
#else
    connect(ui.actionQuit, SIGNAL(triggered()), this, SLOT(close()));
#endif

    for (int i = 0; i < maxRecentFiles; ++i) {
      d->actionRecentFile[i] = new QAction(this);
      d->actionRecentFile[i]->setVisible(false);
      ui.menuOpenRecent->addAction(d->actionRecentFile[i]);
      connect(d->actionRecentFile[i], SIGNAL(triggered()),
          this, SLOT(openRecentFile()));
    }
    ui.menuOpenRecent->addSeparator();
    ui.menuOpenRecent->addAction(ui.actionClearRecent);
    connect(ui.actionClearRecent, SIGNAL(triggered()), this, SLOT(clearRecentFiles()));

    QAction *undoAction = d->undoStack->createUndoAction(this);
    undoAction->setIcon(QIcon(QString::fromUtf8(":/icons/undo.png")));
    undoAction->setShortcuts(QKeySequence::Undo);
    QAction *redoAction = d->undoStack->createRedoAction(this);
    redoAction->setIcon(QIcon(QString::fromUtf8(":/icons/redo.png")));
    redoAction->setShortcuts(QKeySequence::Redo);
    if(ui.menuEdit->actions().count()) {
      QAction *firstAction = ui.menuEdit->actions().at(0);
      ui.menuEdit->insertAction(firstAction, redoAction);
      ui.menuEdit->insertAction(redoAction, undoAction);
    } else {
      ui.menuEdit->addAction(undoAction);
      ui.menuEdit->addAction(redoAction);
    }
    connect(d->undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(undoStackClean(bool)));

    connect(ui.actionCut, SIGNAL(triggered()), this, SLOT(cut()));
    connect(ui.actionCopy, SIGNAL(triggered()), this, SLOT(copy()));
    connect(ui.actionPaste, SIGNAL(triggered()), this, SLOT(paste()));
    connect(ui.actionClear, SIGNAL(triggered()), this, SLOT(clear()));
    connect(ui.actionSelect_All, SIGNAL(triggered()), this, SLOT(selectAll()));
    connect(ui.actionSelect_None, SIGNAL(triggered()), this, SLOT(selectNone()));

    ui.menuDocks->addAction(ui.projectDock->toggleViewAction());
    ui.menuDocks->addAction(ui.toolsDock->toggleViewAction());
    ui.menuDocks->addAction(ui.toolSettingsDock->toggleViewAction());
    ui.menuToolbars->addAction(ui.fileToolBar->toggleViewAction());

    connect(ui.actionNewView, SIGNAL(triggered()), this, SLOT(newView()));
    connect(ui.actionCloseView, SIGNAL(triggered()), this, SLOT(closeView()));
    connect(ui.actionCenter, SIGNAL(triggered()), this, SLOT(centerView()));
    connect(ui.actionFullScreen, SIGNAL(triggered()), this, SLOT(fullScreen()));
    connect(ui.actionSetBackgroundColor, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(about()));

    connect(ui.centralTab, SIGNAL(currentChanged(int)), this, SLOT(setView(int)));

  }

  bool MainWindow::loadFile(const QString &fileName)
  {
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      QApplication::restoreOverrideCursor();
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot read file %1:\n%2.")
          .arg(fileName)
          .arg(file.errorString()));
      return false;
    }
    file.close();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    statusBar()->showMessage(tr("Reading file."), 2000);
    OBConversion conv;
    OBFormat     *inFormat = conv.FormatFromExt((fileName.toAscii()).data());
    if (!inFormat || !conv.SetInFormat(inFormat)) {
      QApplication::restoreOverrideCursor();
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot read file format of file %1.")
          .arg(fileName));
      return false;
    }
    ifstream     ifs;
    ifs.open((fileName.toAscii()).data());
    if (!ifs) { // shouldn't happen, already checked file above
      QApplication::restoreOverrideCursor();
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot read file %1.")
          .arg(fileName));
      return false;
    }

    Molecule *molecule = new Molecule;
    if (conv.Read(molecule, &ifs) && molecule->NumAtoms() != 0)
    {
      setMolecule(molecule);

      QApplication::restoreOverrideCursor();

      QString status;
      QTextStream(&status) << "Atoms: " << d->molecule->NumAtoms() <<
        " Bonds: " << d->molecule->NumBonds();
      statusBar()->showMessage(status, 5000);
    }
    else {
      QApplication::restoreOverrideCursor();
      statusBar()->showMessage("Reading molecular file failed.", 5000);
      return false;
    }

    setFileName(fileName);

    return true;
  }

  void MainWindow::setMolecule(Molecule *molecule)
  {
    if(d->molecule) {
      disconnect(d->molecule, 0, this, 0);
      d->molecule->deleteLater();
    }

    d->molecule = molecule;
    connect(d->molecule, SIGNAL(primitiveAdded(Primitive *)), this, SLOT(documentWasModified()));
    connect(d->molecule, SIGNAL(primitiveUpdated(Primitive *)), this, SLOT(documentWasModified()));
    connect(d->molecule, SIGNAL(primitiveRemoved(Primitive *)), this, SLOT(documentWasModified()));

    d->undoStack->clear();
    foreach(GLWidget *widget, d->glWidgets) {
      widget->setMolecule(d->molecule);
    }
    ui.projectTree->setMolecule(d->molecule);

    setWindowModified(false);
  }

  Molecule *MainWindow::molecule() const
  {
    return d->molecule;
  }

  bool MainWindow::saveFile(const QString &fileName)
  {
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot write to the file %1:\n%2.")
          .arg(fileName)
          .arg(file.errorString()));
      return false;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    statusBar()->showMessage(tr("Saving file."), 2000);

    OBConversion conv;
    OBFormat     *outFormat = conv.FormatFromExt((fileName.toAscii()).data());
    if (!outFormat || !conv.SetOutFormat(outFormat)) {
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot write to file format of file %1.")
          .arg(fileName));
      return false;
    }
    ofstream     ofs;
    ofs.open((fileName.toAscii()).data());
    if (!ofs) { // shouldn't happen, already checked file above
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot write to the file %1.")
          .arg(fileName));
      return false;
    }

    OBMol *molecule = dynamic_cast<OBMol*>(d->molecule);
    if (conv.Write(molecule, &ofs))
      statusBar()->showMessage("Save succeeded.", 5000);
    else
      statusBar()->showMessage("Saving molecular file failed.", 5000);
    QApplication::restoreOverrideCursor();

    setWindowModified(false);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
  }

  void MainWindow::setFileName(const QString &fileName)
  {
    if (fileName.isEmpty()) {
      d->fileName.clear();
      setWindowTitle(tr("[*]Avogadro"));
    } else {
      d->fileName = QFileInfo(fileName).canonicalFilePath();
      setWindowTitle(tr("%1[*] - %2").arg(strippedName(d->fileName))
                     .arg(tr("Avogadro")));
      
      QSettings settings; // already set up properly via main.cpp
      QStringList files = settings.value("recentFileList").toStringList();
      files.removeAll(fileName);
      files.prepend(fileName);
      while (files.size() > maxRecentFiles)
        files.removeLast();
      
      settings.setValue("recentFileList", files);
    }

    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
      MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
      if (mainWin)
        mainWin->updateRecentFileActions();
    }
  }

  void MainWindow::updateRecentFileActions()
  {
    QSettings settings; // set up project and program properly in main.cpp
    QStringList files = settings.value("recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)maxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
      d->actionRecentFile[i]->setText(strippedName(files[i]));
      d->actionRecentFile[i]->setData(files[i]);
      d->actionRecentFile[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < maxRecentFiles; ++j)
      d->actionRecentFile[j]->setVisible(false);

    //     ui.actionSeparator->setVisible(numRecentFiles > 0);
  }

  QString MainWindow::strippedName(const QString &fullFileName)
  {
    return QFileInfo(fullFileName).fileName();
  }

  MainWindow *MainWindow::findMainWindow(const QString &fileName)
  {
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QWidget *widget, qApp->topLevelWidgets()) {
      MainWindow *window = qobject_cast<MainWindow *>(widget);
      if (window && window->d->fileName == canonicalFilePath)
        return window;
    }
    return 0;
  }

  void MainWindow::readSettings()
  {
    QSettings settings;
    //QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(640, 480)).toSize();
    resize(size);
    //move(pos);
  }

  void MainWindow::writeSettings()
  {
    QSettings settings;
    settings.setValue("pos", pos());
    settings.setValue("size", size());
  }

  void MainWindow::loadExtensions()
  {
    QString prefixPath = QString(INSTALL_PREFIX) + "/lib/avogadro/extensions";
    QStringList pluginPaths;
    pluginPaths << prefixPath;

#ifdef WIN32
    pluginPaths << "./extensions";
#endif

    if(getenv("AVOGADRO_EXTENSIONS") != NULL)
    {
      pluginPaths = QString(getenv("AVOGADRO_EXTENSIONS")).split(':');
    }

    foreach (QString path, pluginPaths)
    {
      QDir dir(path); 
      qDebug() << "SearchPath:" << dir.absolutePath() << endl;
      foreach (QString fileName, dir.entryList(QDir::Files)) {
        QPluginLoader loader(dir.absoluteFilePath(fileName));
        QObject *instance = loader.instance();
        qDebug() << "File: " << fileName;
        ExtensionFactory *factory = qobject_cast<ExtensionFactory *>(instance);
        if(factory) {
          Extension *extension = factory->createInstance(this);
          qDebug() << "Found Extension: " << extension->name() << " - " << extension->description();
          QList<QAction *>actions = extension->actions();
          foreach(QAction *action, actions)
          {
            ui.menuTools->addAction(action);
            connect(action, SIGNAL(triggered()), this, SLOT(actionTriggered()));
          }
        }
      }
    }
  }

  void MainWindow::actionTriggered()
  {
    QAction *action = qobject_cast<QAction *>(sender());
    if(action) {
      Extension *extension = dynamic_cast<Extension *>(action->parent());

      QUndoCommand *command = 0;
      command = extension->performAction(action, d->molecule,d->glWidget,
                                         d->messagesText);

      if(command) {
        d->undoStack->push(command);
      }
    }
  }

} // end namespace Avogadro

#include "mainwindow.moc"
