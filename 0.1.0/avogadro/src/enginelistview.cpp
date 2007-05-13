/**********************************************************************
  EngineListView - View for listing engines

  Copyright (C) 2007 by Geoffrey R. Hutchison

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

#include "enginelistview.h"

#include <avogadro/engine.h>
#include <avogadro/glwidget.h>

#include <QStandardItemModel>
#include <QAbstractButton>
#include <QStandardItem>
#include <QVBoxLayout>
#include <QDialog>

namespace Avogadro {

  class EngineListViewPrivate
  {
    public:
      EngineListViewPrivate() : widget(0) {};

      GLWidget *widget;
      QAbstractButton *button;
  };

  EngineListView::EngineListView( QWidget *parent ) : d(new EngineListViewPrivate)
  {
  }

  EngineListView::~EngineListView()
  {
    delete d;
  }

  GLWidget *EngineListView::glWidget() const
  {
    return d->widget;
  }

  void EngineListView::setGLWidget(GLWidget *widget)
  {
    d->widget = widget;

    QStandardItemModel *m = new QStandardItemModel(this);

    foreach(Engine *e, widget->engines())
      {
        QStandardItem *item = new QStandardItem(e->name());
        item->setCheckable(true);
        item->setToolTip(e->description());
        if(e->isEnabled()) {
          item->setCheckState(Qt::Checked);
        }
        item->setData(qVariantFromValue(e), EngineRole);
        item->setData(qVariantFromValue(static_cast<QWidget *>(0)), SettingsDialogRole);
        m->appendRow(item);
      }
    
    if(model())
    {
      delete model();
    }

    setModel(m);
    connect(m, SIGNAL(itemChanged(QStandardItem *)), 
        this, SLOT(updateEngine(QStandardItem *)));
    connect(this, SIGNAL(clicked(QModelIndex)), 
        this, SLOT(selectEngine(QModelIndex)));
  }

  void EngineListView::setSettingsButton( QAbstractButton *button )
  {
    d->button = button;
    connect(button, SIGNAL(clicked()), this, SLOT(showEngineSettings()));
  }

  QAbstractButton *EngineListView::settingsButton() const
  {
    return d->button;
  }

  void EngineListView::selectEngine( const QModelIndex &index )
  {
    if(d->button) {
      Engine *engine = index.data(EngineRole).value<Engine *>();
      if(engine) {
        d->button->setEnabled(engine->settingsWidget());
      }
    }
  }

  void EngineListView::updateEngine( QStandardItem *item )
  {
    Engine *engine = item->data(EngineRole).value<Engine *>();
    if(engine) {
      engine->setEnabled(item->checkState());
      d->widget->update();
    }
  }

  void EngineListView::showEngineSettings()
  {
    QModelIndexList selection = selectedIndexes();
    if(selection.count()) {
      QModelIndex index = selection.at(0);
      QDialog *dialog = qobject_cast<QDialog *>(model()->data(index, SettingsDialogRole).value<QWidget *>());
      if(dialog) {
        dialog->show();
      }
      else
      {
        Engine *engine = model()->data(index, EngineRole).value<Engine *>();
        if(engine && engine->settingsWidget()) {
          QDialog *newDialog = new QDialog();
          newDialog->setWindowTitle(engine->name() + tr(" Settings"));
          QVBoxLayout *vLayout = new QVBoxLayout();
          vLayout->addWidget(engine->settingsWidget());
          newDialog->setLayout(vLayout);
          model()->setData(index, qVariantFromValue(qobject_cast<QWidget *>(newDialog)), SettingsDialogRole);
          newDialog->show();
        }
      }
    }
  }


} // end namespace Avogadro

#include "enginelistview.moc"
