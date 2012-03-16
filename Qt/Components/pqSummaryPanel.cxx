/*=========================================================================

   Program: ParaView
   Module:    pqSummaryPanel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqSummaryPanel.h"

#include <QDebug>

// Qt includes
#include <QBoxLayout>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QStyleFactory>

// ParaView includes
#include "pqNamedWidgets.h"
#include "pqInterfaceTracker.h"
#include "pqApplicationCore.h"
#include "pqObjectPanelInterface.h"
#include "pqLoadedFormObjectPanel.h"
#include "pqAutoGeneratedObjectPanel.h"
#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptors.h"
#include "pqSummaryPanelInterface.h"
#include "pqInterfaceTracker.h"
#include "pqApplicationCore.h"
#include "pqUndoStack.h"
#include "pqPipelineSource.h"
#include "pqProxyModifiedStateUndoElement.h"
#include "pqServer.h"
#include "pqObjectBuilder.h"
#include "pqPipelineFilter.h"
#include "pqView.h"
#include "pqDisplayPolicy.h"
#include "pqApplyPropertiesManager.h"
#include "pqServerManagerModel.h"

// VTK includes
#include "vtkSMPropertyHelper.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTimerLog.h"

//-----------------------------------------------------------------------------
pqSummaryPanel::pqSummaryPanel(QWidget *p)
  : QWidget(p)
{
  this->CurrentPanel = 0;
  this->Representation = 0;
  this->Proxy = 0;
  this->View = 0;
  this->OutputPort = 0;
  this->ShowOnAccept = true;

  this->DisplayWidget = 0;
  this->Links = new pqPropertyLinks;

  QVBoxLayout* l = new QVBoxLayout;

  l->addWidget(this->createPropertiesPanel());
  l->addWidget(this->createButtonBox());
  l->addWidget(this->createRepresentationFrame());
  l->addWidget(this->createDisplayPanel());
  l->addStretch();

  this->PropertiesPanelFrame->hide();
  this->RepresentationFrame->hide();
  this->DisplayPanelFrame->hide();

  this->setLayout(l);

  this->connect(&pqActiveObjects::instance(), SIGNAL(representationChanged(pqDataRepresentation*)),
                this, SLOT(setRepresentation(pqDataRepresentation*)));
  this->connect(&pqActiveObjects::instance(), SIGNAL(portChanged(pqOutputPort*)),
                this, SLOT(setOutputPort(pqOutputPort*)));
  this->connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
                this, SLOT(setView(pqView*)));

  pqApplyPropertiesManager *applyPropertiesManager =
    qobject_cast<pqApplyPropertiesManager *>(
      pqApplicationCore::instance()->manager("APPLY_PROPERTIES"));

  if(applyPropertiesManager)
    {
    this->connect(this->AcceptButton, SIGNAL(clicked()),
                  applyPropertiesManager, SLOT(applyProperties()));
    this->connect(applyPropertiesManager, SIGNAL(apply()),
                  this, SLOT(accept()));
    this->connect(applyPropertiesManager, SIGNAL(applyStateChanged(bool)),
                  this->AcceptButton, SLOT(setEnabled(bool)));
    this->connect(applyPropertiesManager, SIGNAL(resetStateChanged(bool)),
                  this->ResetButton, SLOT(setEnabled(bool)));
    this->connect(applyPropertiesManager, SIGNAL(deleteStateChanged(bool)),
                  this->DeleteButton, SLOT(setEnabled(bool)));
    }

  this->connect(pqApplicationCore::instance()->getServerManagerModel(),
                SIGNAL(sourceRemoved(pqPipelineSource*)),
                this,
                SLOT(removeProxy(pqPipelineSource*)));
  this->connect(pqApplicationCore::instance()->getServerManagerModel(),
                SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*, int)),
                this,
                SLOT(handleConnectionChanged(pqPipelineSource*, pqPipelineSource*)));
  this->connect(pqApplicationCore::instance()->getServerManagerModel(),
                SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*, int)),
                this,
                SLOT(handleConnectionChanged(pqPipelineSource*, pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
pqSummaryPanel::~pqSummaryPanel()
  {
  // delete all queued panels
  foreach(pqObjectPanel* panel, this->PanelStore)
    {
    panel->deleteLater();
    }

  this->setRepresentation(0);
  this->setProxy(0);

  delete this->RepresentationSignalAdaptor;
  delete this->Links;
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::setProxy(pqProxy *p)
{
  if(this->Proxy)
    {
    // remove old property links
    if(vtkSMProperty *prop = this->Proxy->getProxy()->GetProperty("Representation"))
      {
      this->Links->removePropertyLink(this->RepresentationSignalAdaptor,
                                      "currentText",
                                      SIGNAL(currentIndexChanged(int)),
                                      this->Proxy->getProxy(),
                                      prop);
      }
    }

  this->Proxy = p;

  // do nothing if this proxy is already current
  if(this->CurrentPanel && this->CurrentPanel->referenceProxy() == p)
    {
    return;
    }

  if (this->CurrentPanel)
    {
    this->CurrentPanel->deselect();
    this->CurrentPanel->hide();
    this->CurrentPanel->setObjectName("");
    // We don't delete the panel, it's managed by this->PanelStore.
    // Any deletion of any panel must ensure that call pointers
    // to the panel ie. this->CurrentPanel and this->PanelStore
    // are updated so that we don't have any dangling pointers.
    }

  this->CurrentPanel = NULL;
  bool reusedPanel = false;

  if(!p)
    {
    this->DeleteButton->setEnabled(false);
    this->PropertiesPanelFrame->hide();
    this->setRepresentation(0);
    return;
    }

  if(this->CurrentPanel == NULL)
    {
    this->CurrentPanel = this->createSummaryPropertiesPanel(p);
    }

  // the current auto panel always has the name "Editor"
  this->CurrentPanel->setObjectName("Editor");

  if(!reusedPanel)
    {
    QObject::connect(this,
                     SIGNAL(viewChanged(pqView*)),
                     this->CurrentPanel,
                     SLOT(setView(pqView*)));

    QObject::connect(this->CurrentPanel,
                     SIGNAL(modified()),
                     this,
                     SLOT(updateAcceptState()));

    QObject::connect(this->CurrentPanel->referenceProxy(),
                     SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
                     this,
                     SLOT(updateAcceptState()));
    }

  this->PropertiesLayout->addWidget(this->CurrentPanel);
  this->CurrentPanel->setView(this->View);
  this->CurrentPanel->select();
  this->CurrentPanel->show();
  this->updateDeleteButtonState();

  this->PanelStore[p] = this->CurrentPanel;
  this->PropertiesPanelFrame->show();

  this->updateAcceptState();
}

//-----------------------------------------------------------------------------
pqProxy* pqSummaryPanel::proxy() const
{
  return this->Proxy;
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::setRepresentation(pqDataRepresentation *repr)
{
  this->Representation = repr;

  if(!this->Representation)
    {
    this->RepresentationSelector->setRepresentation(0);
    this->RepresentationFrame->hide();
    this->DisplayPanelFrame->hide();
    return;
    }

  vtkSMProxy *p = repr->getProxy();
  vtkSMProperty *representationProperty = p->GetProperty("Representation");

  if(representationProperty)
    {
    this->RepresentationSelector->setRepresentation(repr);

    this->RepresentationFrame->show();
    }
  else
    {
    this->RepresentationFrame->hide();
    }

  this->representationChanged(repr);
}

//-----------------------------------------------------------------------------
pqRepresentation* pqSummaryPanel::representation() const
{
  return this->Representation;
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::setView(pqView* v)
{
  this->View = v;
  emit this->viewChanged(v);
}

//-----------------------------------------------------------------------------
pqView* pqSummaryPanel::view() const
{
  return this->View;
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::setOutputPort(pqOutputPort *port)
{
  if (this->OutputPort == port)
    {
    return;
    }

  if (this->OutputPort)
    {
    QObject::disconnect(this->OutputPort, 0, this, 0);
    }

  this->OutputPort = port;

  this->setProxy(port ? port->getSource() : 0);
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::reset()
{
  // reset all panels that are dirty
  foreach(pqObjectPanel *panel, this->PanelStore)
    {
    if (panel->referenceProxy()->modifiedState() != pqProxy::UNMODIFIED)
      {
      panel->reset();
      }
    }

  if(this->CurrentPanel)
    {
    this->CurrentPanel->reset();
    }
}

//-----------------------------------------------------------------------------
QWidget* pqSummaryPanel::createPropertiesPanel()
{
  pqCollapsedGroup *frame = new pqCollapsedGroup(this);
  frame->setTitle("Properties");

  QGridLayout *frameLayout = new QGridLayout;
  frameLayout->setMargin(0);

  QScrollArea *scrollArea = new QScrollArea(frame);
  scrollArea->setFrameStyle(QFrame::NoFrame);
  scrollArea->setWidgetResizable(true);

  QWidget *scrollAreaWidget = new QWidget(scrollArea);

  this->PropertiesLayout = new QGridLayout;
  this->PropertiesLayout->setMargin(0);

  scrollAreaWidget->setLayout(this->PropertiesLayout);
  scrollArea->setWidget(scrollAreaWidget);
  frameLayout->addWidget(scrollArea);
  frame->setLayout(frameLayout);

  this->PropertiesPanelFrame = frame;

  return frame;
}

//-----------------------------------------------------------------------------
QWidget* pqSummaryPanel::createButtonBox()
{
  QFrame *frame = new QFrame(this);

  QBoxLayout* l = new QHBoxLayout();
  this->AcceptButton = new QPushButton(this);
  this->AcceptButton->setObjectName("Accept");
  this->AcceptButton->setText(tr("&Apply"));
  this->AcceptButton->setIcon(QIcon(QPixmap(":/pqWidgets/Icons/pqUpdate16.png")));
#ifdef Q_WS_MAC
  this->AcceptButton->setShortcut(QKeySequence(Qt::AltModifier + Qt::Key_A));
#endif

  this->ResetButton = new QPushButton(this);
  this->ResetButton->setObjectName("Reset");
  this->ResetButton->setText(tr("&Reset"));
  this->ResetButton->setIcon(QIcon(QPixmap(":/pqWidgets/Icons/pqCancel16.png")));
#ifdef Q_WS_MAC
  this->ResetButton->setShortcut(QKeySequence(Qt::AltModifier + Qt::Key_R));
#endif
  this->DeleteButton = new QPushButton(this);
  this->DeleteButton->setObjectName("Delete");
  this->DeleteButton->setText(tr("Delete"));
  this->DeleteButton->setIcon(QIcon(QPixmap(":/QtWidgets/Icons/pqDelete16.png")));

  this->connect(this->ResetButton, SIGNAL(clicked()), SLOT(reset()));
  this->connect(this->DeleteButton, SIGNAL(clicked()), SLOT(deleteProxy()));

  l->addWidget(this->AcceptButton);
  l->addWidget(this->ResetButton);
  l->addWidget(this->DeleteButton);

  this->AcceptButton->setEnabled(false);
  this->ResetButton->setEnabled(false);
  this->DeleteButton->setEnabled(false);

  // if XP Style is being used
  // swap it out for cleanlooks which looks almost the same
  // so we can have a green accept button
  // make all the buttons the same
  QString styleName = this->AcceptButton->style()->metaObject()->className();
  if(styleName == "QWindowsXPStyle")
     {
     QStyle* st = QStyleFactory::create("cleanlooks");
     st->setParent(this);
     this->AcceptButton->setStyle(st);
     this->ResetButton->setStyle(st);
     this->DeleteButton->setStyle(st);
     QPalette buttonPalette = this->AcceptButton->palette();
     buttonPalette.setColor(QPalette::Button, QColor(244,246,244));
     this->AcceptButton->setPalette(buttonPalette);
     this->ResetButton->setPalette(buttonPalette);
     this->DeleteButton->setPalette(buttonPalette);
     }
  // Change the accept button palette so it is green when it is active.
  QPalette acceptPalette = this->AcceptButton->palette();
  acceptPalette.setColor(QPalette::Active, QPalette::Button, QColor(161, 213, 135));
  this->AcceptButton->setPalette(acceptPalette);
  this->AcceptButton->setDefault(true);

  frame->setLayout(l);

  return frame;
}

//-----------------------------------------------------------------------------
QWidget* pqSummaryPanel::createRepresentationFrame()
{
  QFrame *frame = new QFrame(this);

  QHBoxLayout *l = new QHBoxLayout;

  this->RepresentationSelector = new pqDisplayRepresentationWidget(frame);
  this->RepresentationSignalAdaptor = 0;

  connect(this->RepresentationSelector,
          SIGNAL(currentTextChanged(const QString&)),
          this,
          SLOT(representionComboBoxChanged(const QString&)),
          Qt::QueuedConnection);

  l->addWidget(new QLabel("Representation:", frame));
  l->addWidget(this->RepresentationSelector);

  frame->setLayout(l);

  this->RepresentationFrame = frame;

  return frame;
}

//-----------------------------------------------------------------------------
QWidget* pqSummaryPanel::createDisplayPanel()
{
  pqCollapsedGroup *frame = new pqCollapsedGroup(this);
  frame->setTitle("Display");

  this->DisplayLayout = new QGridLayout;
  this->DisplayLayout->setMargin(0);
  frame->setLayout(this->DisplayLayout);

  this->DisplayPanelFrame = frame;

  return frame;
}

//-----------------------------------------------------------------------------
pqObjectPanel* pqSummaryPanel::createSummaryPropertiesPanel(pqProxy *p)
{
  if(!p)
    {
    return 0;
    }

  pqInterfaceTracker *interfaceTracker = pqApplicationCore::instance()->interfaceTracker();

  // attempt to create a custom properties panel from a summary
  // panel interface
  pqObjectPanel *customPropertiesPanel = 0;

  foreach(pqSummaryPanelInterface *interface, interfaceTracker->interfaces<pqSummaryPanelInterface *>())
    {
    customPropertiesPanel = interface->createPropertiesPanel(p);

    if(customPropertiesPanel)
      {
      // stop if we successfully created a properties panel
      break;
      }
    }

  if(customPropertiesPanel)
    {
    return customPropertiesPanel;
    }
  else
    {
    // use an auto-generated panel
    return new pqAutoGeneratedObjectPanel(p, true);
    }
}

//-----------------------------------------------------------------------------
QWidget* pqSummaryPanel::createSummaryDisplayPanel(pqDataRepresentation *repr)
{
  if(!repr)
    {
    return 0;
    }

  QWidget *widget = new QWidget(this);
  QVBoxLayout *l = new QVBoxLayout;
  l->setMargin(0);

  pqInterfaceTracker *interfaceTracker = pqApplicationCore::instance()->interfaceTracker();

  // attempt to create a custom display panel from a summary
  // panel interface
  QWidget *customDisplayPanel = 0;

  foreach(pqSummaryPanelInterface *interface, interfaceTracker->interfaces<pqSummaryPanelInterface *>())
    {
    customDisplayPanel = interface->createDisplayPanel(repr);

    if(customDisplayPanel)
      {
      // stop if we successfully created a display panel
      break;
      }
    }

  if(customDisplayPanel)
    {
    l->addWidget(customDisplayPanel);
    }

  widget->setLayout(l);

  return widget;
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::accept()
{
  QSet<pqProxy*> proxiesToShow;

  // accept all panels that are dirty.
  foreach(pqObjectPanel* panel, this->PanelStore)
    {
    pqProxy* refProxy = panel->referenceProxy();
    if(!refProxy)
      {
      continue;
      }

    int modifiedState = refProxy->modifiedState();
    if (this->ShowOnAccept && modifiedState == pqProxy::UNINITIALIZED)
      {
      proxiesToShow.insert(refProxy);
      }
    if (modifiedState != pqProxy::UNMODIFIED)
      {
      panel->accept();
      }
    }

  if (this->CurrentPanel)
    {
    pqProxy* refProxy = this->CurrentPanel->referenceProxy();
    int modifiedState = refProxy->modifiedState();
    if (this->ShowOnAccept && modifiedState == pqProxy::UNINITIALIZED)
      {
      proxiesToShow.insert(refProxy);
      }
    this->CurrentPanel->accept();
    }

  foreach (pqProxy* proxyToShow, proxiesToShow)
    {
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(proxyToShow);
    if (source)
      {
      this->show(source);
      pqProxyModifiedStateUndoElement* elem = pqProxyModifiedStateUndoElement::New();
      elem->SetSession(source->getServer()->session());
      elem->MadeUnmodified(source);
      ADD_UNDO_ELEM(elem);
      elem->Delete();
      }
    }
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::canAccept(bool status)
{
  this->AcceptButton->setEnabled(status);
  this->ResetButton->setEnabled(status);
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::updateDeleteButtonState()
{
  pqPipelineSource *source = 0;
  if(this->CurrentPanel)
    {
    source = dynamic_cast<pqPipelineSource *>(this->CurrentPanel->referenceProxy());
    }

  this->DeleteButton->setEnabled(source && source->getNumberOfConsumers() == 0);
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::removeProxy(pqPipelineSource* p)
{
  this->disconnect(p, SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
                   this, SLOT(updateAcceptState()));

  if (this->CurrentPanel && this->CurrentPanel->referenceProxy() == p)
    {
    this->CurrentPanel = NULL;
    }

  QMap<pqProxy*, QPointer<pqObjectPanel> >::iterator iter = this->PanelStore.find(p);
  if (iter != this->PanelStore.end())
    {
    this->disconnect(iter.value(), SIGNAL(modified()),
                     this, SLOT(updateAcceptState()));

    if(iter.value())
      {
      delete iter.value();
      }

    this->PanelStore.erase(iter);
    }

}

//-----------------------------------------------------------------------------
void pqSummaryPanel::deleteProxy()
{
  if (this->CurrentPanel && this->CurrentPanel->referenceProxy())
    {
    pqPipelineSource* source =
      qobject_cast<pqPipelineSource*>(this->CurrentPanel->referenceProxy());

    pqApplicationCore* core = pqApplicationCore::instance();
    BEGIN_UNDO_SET(QString("Delete %1").arg(source->getSMName()));
    core->getObjectBuilder()->destroy(source);
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::handleConnectionChanged(pqPipelineSource* in, pqPipelineSource* out)
{
  Q_UNUSED(out);

  if(this->CurrentPanel && this->CurrentPanel->referenceProxy() == in)
    {
    this->updateDeleteButtonState();
    }
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::updateAcceptState()
{
  // watch for modified state changes
  bool acceptable = false;
  foreach(const pqObjectPanel *panel, this->PanelStore)
    {
    if(panel->referenceProxy() &&
       panel->referenceProxy()->modifiedState() != pqProxy::UNMODIFIED)
      {
      acceptable = true;
      }
    }
  this->canAccept(acceptable);
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::show(pqPipelineSource* source)
{
  pqDisplayPolicy* displayPolicy = pqApplicationCore::instance()->getDisplayPolicy();
  if (!displayPolicy)
    {
    qCritical() << "No display policy defined. Cannot create pending displays.";
    return;
    }

  // Create representations for all output ports.
  for (int i = 0; i < source->getNumberOfOutputPorts(); i++)
    {
    pqDataRepresentation* repr =
      displayPolicy->createPreferredRepresentation(source->getOutputPort(i),
                                                   this->view(),
                                                   false);
    if (!repr || !repr->getView())
      {
      continue;
      }

    pqView* currentView = repr->getView();
    pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
    if (filter)
      {
      filter->hideInputIfRequired(currentView);
      }
    currentView->render(); // these renders are collapsed.
    }
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::representationChanged(pqDataRepresentation *repr)
{
  if(this->DisplayWidget)
    {
    this->DisplayLayout->removeWidget(this->DisplayWidget);
    this->DisplayWidget->deleteLater();
    this->DisplayWidget = 0;
    this->DisplayPanelFrame->hide();
    }

  this->DisplayWidget = this->createSummaryDisplayPanel(repr);

  if(this->DisplayWidget)
    {
    this->DisplayLayout->addWidget(this->DisplayWidget);
    this->DisplayPanelFrame->show();
    }
}

//-----------------------------------------------------------------------------
void pqSummaryPanel::representionComboBoxChanged(const QString &text)
{
  Q_UNUSED(text);

  representationChanged(this->Representation);
}