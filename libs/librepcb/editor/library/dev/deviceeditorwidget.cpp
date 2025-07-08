/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "deviceeditorwidget.h"

#include "../../graphics/graphicslayerlist.h"
#include "../../graphics/graphicsscene.h"
#include "../../library/cmd/cmdlibraryelementedit.h"
#include "../../undocommandgroup.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmddeviceedit.h"
#include "../cmd/cmddevicepadsignalmapitemedit.h"
#include "../cmp/componentchooserdialog.h"
#include "../pkg/footprintgraphicsitem.h"
#include "../pkg/packagechooserdialog.h"
#include "../sym/symbolgraphicsitem.h"
#include "ui_deviceeditorwidget.h"

#include <librepcb/core/application.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/libraryelementcheckmessages.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DeviceEditorWidget::DeviceEditorWidget(const Context& context,
                                       const FilePath& fp, QWidget* parent)
  : EditorWidgetBase(context, fp, parent), mUi(new Ui::DeviceEditorWidget) {
  mUi->setupUi(this);
  mUi->lstMessages->setHandler(this);
  mUi->lstMessages->setReadOnly(mContext.readOnly);
  mUi->edtName->setReadOnly(mContext.readOnly);
  mUi->edtDescription->setReadOnly(mContext.readOnly);
  mUi->edtKeywords->setReadOnly(mContext.readOnly);
  mUi->edtAuthor->setReadOnly(mContext.readOnly);
  mUi->edtVersion->setReadOnly(mContext.readOnly);
  mUi->cbxDeprecated->setCheckable(!mContext.readOnly);
  mUi->edtDatasheet->setReadOnly(mContext.readOnly);
  connect(mUi->btnDownloadDatasheet, &QToolButton::clicked, this, [this]() {
    if (auto dbRes = mDevice->getResources().value(0)) {
      DesktopServices::downloadAndOpenResourceAsync(
          mContext.workspace.getSettings(), *dbRes->getName(),
          dbRes->getMediaType(), dbRes->getUrl(), this);
    }
  });
  mUi->btnChoosePackage->setHidden(mContext.readOnly);
  mUi->btnChooseComponent->setHidden(mContext.readOnly);
  mUi->padSignalMapEditorWidget->setReadOnly(mContext.readOnly);
  mUi->padSignalMapEditorWidget->setFrameStyle(QFrame::NoFrame);
  connect(mUi->padSignalMapEditorWidget,
          &PadSignalMapEditorWidget::statusTipChanged, this,
          [this](const QString& statusTip) { setStatusBarMessage(statusTip); });
  mUi->partsEditorWidget->setReadOnly(mContext.readOnly);
  mUi->partsEditorWidget->setFrameStyle(QFrame::NoFrame);
  if (mContext.library) {
    mUi->partsEditorWidget->setInitialManufacturer(
        mContext.library->getManufacturer());
  }
  mUi->attributesEditorWidget->setReadOnly(mContext.readOnly);
  mUi->attributesEditorWidget->setFrameStyle(QFrame::NoFrame);
  setupErrorNotificationWidget(*mUi->errorNotificationWidget);
  setWindowIcon(QIcon(":/img/library/device.png"));

  // Setup graphics scenes.
  const Theme& theme = mContext.workspace.getSettings().themes.getActive();
  mComponentGraphicsScene.reset(new GraphicsScene());
  mPackageGraphicsScene.reset(new GraphicsScene());
  mComponentGraphicsScene->setBackgroundColors(
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mPackageGraphicsScene->setBackgroundColors(
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());

  // Setup graphics views.
  mUi->viewComponent->setSpinnerColor(
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mUi->viewPackage->setSpinnerColor(
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  mUi->viewComponent->setScene(mComponentGraphicsScene.data());
  mUi->viewPackage->setScene(mPackageGraphicsScene.data());
  mLayers = GraphicsLayerList::previewLayers(&mContext.workspace.getSettings());

  // Insert category list editor widget.
  mCategoriesEditorWidget.reset(new CategoryListEditorWidget(
      mContext.workspace, CategoryListEditorWidget::Categories::Component,
      this));
  mCategoriesEditorWidget->setReadOnly(mContext.readOnly);
  mCategoriesEditorWidget->setRequiresMinimumOneEntry(true);
  int row;
  QFormLayout::ItemRole role;
  mUi->formLayout->getWidgetPosition(mUi->lblCategories, &row, &role);
  mUi->formLayout->setWidget(row, QFormLayout::FieldRole,
                             mCategoriesEditorWidget.data());

  // Load element.
  mDevice = Device::open(std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mFileSystem)));  // can throw
  mUi->padSignalMapEditorWidget->setReferences(mUndoStack.data(),
                                               &mDevice->getPadSignalMap());
  updateDeviceComponentUuid(mDevice->getComponentUuid());
  updateDevicePackageUuid(mDevice->getPackageUuid());
  updateMetadata();

  // Load parts editor.
  mUi->partsEditorWidget->setReferences(mUndoStack.data(),
                                        &mDevice->getParts());
  connect(mUi->partsEditorWidget, &PartListEditorWidget::currentItemChanged,
          this, &DeviceEditorWidget::setSelectedPart);
  setSelectedPart(-1);

  // Show "interface broken" warning when related properties are modified.
  memorizeDeviceInterface();
  setupInterfaceBrokenWarningWidget(*mUi->interfaceBrokenWarningWidget);

  // Reload metadata on undo stack state changes.
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &DeviceEditorWidget::updateMetadata);

  // Reload data on device object changes.
  connect(mDevice.get(), &Device::componentUuidChanged, this,
          &DeviceEditorWidget::updateDeviceComponentUuid);
  connect(mDevice.get(), &Device::packageUuidChanged, this,
          &DeviceEditorWidget::updateDevicePackageUuid);
  connect(mUi->btnChooseComponent, &QToolButton::clicked, this,
          &DeviceEditorWidget::btnChooseComponentClicked);
  connect(mUi->btnChoosePackage, &QToolButton::clicked, this,
          &DeviceEditorWidget::btnChoosePackageClicked);

  // Handle changes of metadata.
  connect(mUi->edtName, &QLineEdit::editingFinished, this,
          &DeviceEditorWidget::commitMetadata);
  connect(mUi->edtDescription, &PlainTextEdit::editingFinished, this,
          &DeviceEditorWidget::commitMetadata);
  connect(mUi->edtKeywords, &QLineEdit::editingFinished, this,
          &DeviceEditorWidget::commitMetadata);
  connect(mUi->edtAuthor, &QLineEdit::editingFinished, this,
          &DeviceEditorWidget::commitMetadata);
  connect(mUi->edtVersion, &QLineEdit::editingFinished, this,
          &DeviceEditorWidget::commitMetadata);
  connect(mUi->cbxDeprecated, &QCheckBox::clicked, this,
          &DeviceEditorWidget::commitMetadata);
  connect(mUi->edtDatasheet, &QLineEdit::editingFinished, this,
          &DeviceEditorWidget::commitMetadata);
  connect(mCategoriesEditorWidget.data(), &CategoryListEditorWidget::edited,
          this, &DeviceEditorWidget::commitMetadata);
}

DeviceEditorWidget::~DeviceEditorWidget() noexcept {
  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();

  // Disconnect UI from library element to avoid dangling pointers.
  mUi->padSignalMapEditorWidget->setReferences(nullptr, nullptr);
  mUi->partsEditorWidget->setReferences(nullptr, nullptr);
  mUi->attributesEditorWidget->setReferences(nullptr, nullptr);
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QSet<EditorWidgetBase::Feature> DeviceEditorWidget::getAvailableFeatures()
    const noexcept {
  return {
      EditorWidgetBase::Feature::Close,
  };
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool DeviceEditorWidget::save() noexcept {
  // Remove obsolete message approvals (bypassing the undo stack).
  mDevice->setMessageApprovals(mDevice->getMessageApprovals() -
                               mDisappearedApprovals);

  // Commit metadata.
  QString errorMsg = commitMetadata();
  if (!errorMsg.isEmpty()) {
    QMessageBox::critical(this, tr("Invalid metadata"), errorMsg);
    return false;
  }

  // Save element.
  try {
    mDevice->save();  // can throw
    mFileSystem->save();  // can throw
    memorizeDeviceInterface();
    return EditorWidgetBase::save();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Save failed"), e.getMsg());
    return false;
  }
}

bool DeviceEditorWidget::zoomIn() noexcept {
  mUi->viewComponent->zoomIn();
  mUi->viewPackage->zoomIn();
  return true;
}

bool DeviceEditorWidget::zoomOut() noexcept {
  mUi->viewComponent->zoomOut();
  mUi->viewPackage->zoomOut();
  return true;
}

bool DeviceEditorWidget::zoomAll() noexcept {
  mUi->viewComponent->zoomAll();
  mUi->viewPackage->zoomAll();
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void DeviceEditorWidget::updateMetadata() noexcept {
  setWindowTitle(*mDevice->getNames().getDefaultValue());
  mUi->edtName->setText(*mDevice->getNames().getDefaultValue());
  mUi->edtDescription->setPlainText(
      mDevice->getDescriptions().getDefaultValue());
  mUi->edtKeywords->setText(mDevice->getKeywords().getDefaultValue());
  mUi->edtAuthor->setText(mDevice->getAuthor());
  mUi->edtVersion->setText(mDevice->getVersion().toStr());
  mUi->cbxDeprecated->setChecked(mDevice->isDeprecated());
  if (auto dbRes = mDevice->getResources().value(0)) {
    mUi->edtDatasheet->setText(dbRes->getUrl().toDisplayString());
  } else {
    mUi->edtDatasheet->clear();
  }
  mUi->btnDownloadDatasheet->setEnabled(!mUi->edtDatasheet->text().isEmpty());
  mUi->lstMessages->setApprovals(mDevice->getMessageApprovals());
  mCategoriesEditorWidget->setUuids(mDevice->getCategories());
}

QString DeviceEditorWidget::commitMetadata() noexcept {
  try {
    std::unique_ptr<CmdLibraryElementEdit> cmd(
        new CmdLibraryElementEdit(*mDevice, tr("Edit device metadata")));
    try {
      // throws on invalid name
      cmd->setName("", ElementName(mUi->edtName->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setDescription("", mUi->edtDescription->toPlainText().trimmed());
    cmd->setKeywords("", mUi->edtKeywords->text().trimmed());
    try {
      // throws on invalid version
      cmd->setVersion(Version::fromString(mUi->edtVersion->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setAuthor(mUi->edtAuthor->text().trimmed());
    cmd->setDeprecated(mUi->cbxDeprecated->isChecked());
    cmd->setCategories(mCategoriesEditorWidget->getUuids());
    try {
      ResourceList resources = mDevice->getResources();
      const ElementName name(
          cleanElementName("Datasheet " % mUi->edtName->text().trimmed()));
      const QString dbUrlStr = mUi->edtDatasheet->text().trimmed();
      const QUrl dbUrl = QUrl::fromUserInput(dbUrlStr);
      std::shared_ptr<Resource> res = resources.value(0);
      if ((dbUrl.isValid()) && (!res)) {
        resources.append(
            std::make_shared<Resource>(name, "application/pdf", dbUrl));
      } else if ((!dbUrl.isValid()) && res) {
        resources.remove(res.get());
      } else if ((dbUrl.isValid()) && res &&
                 (dbUrlStr != res->getUrl().toDisplayString())) {
        res->setName(name);
        res->setUrl(dbUrl);
      }
      cmd->setResources(resources);
    } catch (const Exception& e) {
    }

    // Commit all changes.
    mUndoStack->execCmd(cmd.release());  // can throw

    // Reload metadata into widgets to discard invalid input.
    updateMetadata();
  } catch (const Exception& e) {
    return e.getMsg();
  }
  return QString();
}

void DeviceEditorWidget::btnChooseComponentClicked() noexcept {
  ComponentChooserDialog dialog(mContext.workspace, mLayers.get(), this);
  if (dialog.exec() == QDialog::Accepted) {
    std::optional<Uuid> cmpUuid = dialog.getSelectedComponentUuid();
    if (cmpUuid && (*cmpUuid != mDevice->getComponentUuid())) {
      try {
        // load component
        FilePath fp = mContext.workspace.getLibraryDb().getLatest<Component>(
            *cmpUuid);  // can throw
        if (!fp.isValid()) {
          throw RuntimeError(__FILE__, __LINE__, tr("Component not found!"));
        }
        std::unique_ptr<Component> cmp = Component::open(
            std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
                TransactionalFileSystem::openRO(fp))));  // can throw

        // edit device
        std::unique_ptr<UndoCommandGroup> cmdGroup(
            new UndoCommandGroup(tr("Change component")));
        std::unique_ptr<CmdDeviceEdit> cmdDevEdit(new CmdDeviceEdit(*mDevice));
        cmdDevEdit->setComponentUuid(*cmpUuid);
        cmdGroup->appendChild(cmdDevEdit.release());
        for (DevicePadSignalMapItem& item : mDevice->getPadSignalMap()) {
          std::optional<Uuid> signalUuid = item.getSignalUuid();
          if (!signalUuid || !cmp->getSignals().contains(*signalUuid)) {
            std::unique_ptr<CmdDevicePadSignalMapItemEdit> cmdItem(
                new CmdDevicePadSignalMapItemEdit(item));
            cmdItem->setSignalUuid(std::nullopt);
            cmdGroup->appendChild(cmdItem.release());
          }
        }
        mUndoStack->execCmd(cmdGroup.release());
      } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not set component"), e.getMsg());
      }
    }
  }
}

void DeviceEditorWidget::btnChoosePackageClicked() noexcept {
  PackageChooserDialog dialog(mContext.workspace, mLayers.get(), this);
  if (dialog.exec() == QDialog::Accepted) {
    std::optional<Uuid> pkgUuid = dialog.getSelectedPackageUuid();
    if (pkgUuid && (*pkgUuid != mDevice->getPackageUuid())) {
      try {
        // load package
        FilePath fp = mContext.workspace.getLibraryDb().getLatest<Package>(
            *pkgUuid);  // can throw
        if (!fp.isValid()) {
          throw RuntimeError(__FILE__, __LINE__, tr("Package not found!"));
        }
        std::unique_ptr<Package> pkg = Package::open(
            std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
                TransactionalFileSystem::openRO(fp))));  // can throw
        QSet<Uuid> pads = pkg->getPads().getUuidSet();

        // edit device
        std::unique_ptr<UndoCommandGroup> cmdGroup(
            new UndoCommandGroup(tr("Change package")));
        std::unique_ptr<CmdDeviceEdit> cmdDevEdit(new CmdDeviceEdit(*mDevice));
        cmdDevEdit->setPackageUuid(*pkgUuid);
        cmdGroup->appendChild(cmdDevEdit.release());
        for (const DevicePadSignalMapItem& item : mDevice->getPadSignalMap()) {
          if (!pads.contains(item.getPadUuid())) {
            cmdGroup->appendChild(new CmdDevicePadSignalMapItemRemove(
                mDevice->getPadSignalMap(), &item));
          }
        }
        foreach (const Uuid& pad,
                 pads - mDevice->getPadSignalMap().getUuidSet()) {
          cmdGroup->appendChild(new CmdDevicePadSignalMapItemInsert(
              mDevice->getPadSignalMap(),
              std::make_shared<DevicePadSignalMapItem>(pad, std::nullopt)));
        }
        mUndoStack->execCmd(cmdGroup.release());
        Q_ASSERT(mDevice->getPadSignalMap().getUuidSet() == pads);
      } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not set package"), e.getMsg());
      }
    }
  }
}

void DeviceEditorWidget::updateDeviceComponentUuid(const Uuid& uuid) noexcept {
  mSymbolGraphicsItems.clear();
  mSymbols.clear();
  try {
    FilePath fp = mContext.workspace.getLibraryDb().getLatest<Component>(
        uuid);  // can throw
    if (!fp.isValid()) {
      throw RuntimeError(__FILE__, __LINE__, tr("Component not found!"));
    }
    mComponent.reset(
        Component::open(
            std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
                TransactionalFileSystem::openRO(fp))))
            .release());  // can throw
    mUi->padSignalMapEditorWidget->setSignalList(mComponent->getSignals());
    mUi->lblComponentName->setText(
        *mComponent->getNames().value(getLibLocaleOrder()));
    mUi->lblComponentName->setToolTip(
        mComponent->getDescriptions().value(getLibLocaleOrder()));
    mUi->lblComponentName->setStyleSheet("");
    updateComponentPreview();
  } catch (const Exception& e) {
    mUi->padSignalMapEditorWidget->setSignalList(ComponentSignalList());
    mUi->lblComponentName->setText(e.getMsg());
    mUi->lblComponentName->setToolTip(QString());
    mUi->lblComponentName->setStyleSheet("color: red;");
  }
}

void DeviceEditorWidget::updateComponentPreview() noexcept {
  if (mComponent && mComponent->getSymbolVariants().count() > 0) {
    const ComponentSymbolVariant& symbVar =
        *mComponent->getSymbolVariants().first();
    for (const ComponentSymbolVariantItem& item : symbVar.getSymbolItems()) {
      try {
        FilePath fp = mContext.workspace.getLibraryDb().getLatest<Symbol>(
            item.getSymbolUuid());  // can throw
        std::shared_ptr<Symbol> sym(
            Symbol::open(std::unique_ptr<TransactionalDirectory>(
                             new TransactionalDirectory(
                                 TransactionalFileSystem::openRO(fp))))
                .release());  // can throw
        mSymbols.append(sym);

        std::shared_ptr<SymbolGraphicsItem> graphicsItem =
            std::make_shared<SymbolGraphicsItem>(
                *sym, *mLayers, mComponent.get(),
                symbVar.getSymbolItems().get(item.getUuid()),
                getLibLocaleOrder());
        graphicsItem->setPosition(item.getSymbolPosition());
        graphicsItem->setRotation(item.getSymbolRotation());
        mComponentGraphicsScene->addItem(*graphicsItem);
        mSymbolGraphicsItems.append(graphicsItem);
      } catch (const Exception& e) {
        // what could we do here? ;)
      }
    }
    mUi->viewComponent->zoomAll();
  }
}

void DeviceEditorWidget::updateDevicePackageUuid(const Uuid& uuid) noexcept {
  mFootprintGraphicsItem.reset();
  try {
    FilePath fp = mContext.workspace.getLibraryDb().getLatest<Package>(
        uuid);  // can throw
    if (!fp.isValid()) {
      throw RuntimeError(__FILE__, __LINE__, tr("Package not found!"));
    }
    mPackage.reset(Package::open(std::unique_ptr<TransactionalDirectory>(
                                     new TransactionalDirectory(
                                         TransactionalFileSystem::openRO(fp))))
                       .release());  // can throw
    mUi->padSignalMapEditorWidget->setPadList(mPackage->getPads());
    mUi->lblPackageName->setText(
        *mPackage->getNames().value(getLibLocaleOrder()));
    mUi->lblPackageName->setToolTip(
        mPackage->getDescriptions().value(getLibLocaleOrder()));
    mUi->lblPackageName->setStyleSheet("");
    updatePackagePreview();
  } catch (const Exception& e) {
    mUi->padSignalMapEditorWidget->setPadList(PackagePadList());
    mUi->lblPackageName->setText(e.getMsg());
    mUi->lblPackageName->setToolTip(QString());
    mUi->lblPackageName->setStyleSheet("color: red;");
  }
}

void DeviceEditorWidget::updatePackagePreview() noexcept {
  if (mPackage && mPackage->getFootprints().count() > 0) {
    mFootprintGraphicsItem.reset(new FootprintGraphicsItem(
        mPackage->getFootprints().first(), *mLayers,
        Application::getDefaultStrokeFont(), &mPackage->getPads(),
        mComponent.get(), getLibLocaleOrder()));
    mPackageGraphicsScene->addItem(*mFootprintGraphicsItem);
    mUi->viewPackage->zoomAll();
  }
}

void DeviceEditorWidget::setSelectedPart(int index) noexcept {
  if (auto part = mDevice->getParts().value(index)) {
    mUi->gbxAttributes->setTitle(tr("Attributes of Selected Part"));
    mUi->attributesEditorWidget->setReferences(mUndoStack.data(),
                                               &part->getAttributes());
  } else {
    mUi->gbxAttributes->setTitle(tr("Device Attributes"));
    mUi->attributesEditorWidget->setReferences(mUndoStack.data(),
                                               &mDevice->getAttributes());
  }
}

void DeviceEditorWidget::memorizeDeviceInterface() noexcept {
  mOriginalComponentUuid = mDevice->getComponentUuid();
  mOriginalPackageUuid = mDevice->getPackageUuid();
  mOriginalPadSignalMap = mDevice->getPadSignalMap();
}

bool DeviceEditorWidget::isInterfaceBroken() const noexcept {
  if (mDevice->getComponentUuid() != mOriginalComponentUuid) return true;
  if (mDevice->getPackageUuid() != mOriginalPackageUuid) return true;
  if (mDevice->getPadSignalMap() != mOriginalPadSignalMap) return true;
  return false;
}

bool DeviceEditorWidget::runChecks(RuleCheckMessageList& msgs) const {
  msgs = mDevice->runChecks();  // can throw
  mUi->lstMessages->setMessages(msgs);
  return true;
}

template <>
void DeviceEditorWidget::fixMsg(const MsgNameNotTitleCase& msg) {
  mUi->edtName->setText(*msg.getFixedName());
  commitMetadata();
}

template <>
void DeviceEditorWidget::fixMsg(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mUi->edtAuthor->setText(getWorkspaceSettingsUserName());
  commitMetadata();
}

template <>
void DeviceEditorWidget::fixMsg(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mCategoriesEditorWidget->openAddCategoryDialog();
}

template <typename MessageType>
bool DeviceEditorWidget::fixMsgHelper(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (applyFix) fixMsg(*m);  // can throw
      return true;
    }
  }
  return false;
}

bool DeviceEditorWidget::processRuleCheckMessage(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  if (fixMsgHelper<MsgNameNotTitleCase>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingAuthor>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingCategories>(msg, applyFix)) return true;
  return false;
}

void DeviceEditorWidget::ruleCheckApproveRequested(
    std::shared_ptr<const RuleCheckMessage> msg, bool approve) noexcept {
  setMessageApproved(*mDevice, msg, approve);
  updateMetadata();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
