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

#include "../../library/cmd/cmdlibraryelementedit.h"
#include "../../undocommandgroup.h"
#include "../cmd/cmddeviceedit.h"
#include "../cmd/cmddevicepadsignalmapitemedit.h"
#include "../cmp/componentchooserdialog.h"
#include "../pkg/footprintpreviewgraphicsitem.h"
#include "../pkg/packagechooserdialog.h"
#include "../sym/symbolpreviewgraphicsitem.h"
#include "ui_deviceeditorwidget.h"

#include <librepcb/core/graphics/defaultgraphicslayerprovider.h>
#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/msg/msgmissingauthor.h>
#include <librepcb/core/library/msg/msgmissingcategories.h>
#include <librepcb/core/library/msg/msgnamenottitlecase.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

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
  mUi->lstMessages->setProvideFixes(!mContext.readOnly);
  mUi->edtName->setReadOnly(mContext.readOnly);
  mUi->edtDescription->setReadOnly(mContext.readOnly);
  mUi->edtKeywords->setReadOnly(mContext.readOnly);
  mUi->edtAuthor->setReadOnly(mContext.readOnly);
  mUi->edtVersion->setReadOnly(mContext.readOnly);
  mUi->cbxDeprecated->setCheckable(!mContext.readOnly);
  mUi->btnChoosePackage->setHidden(mContext.readOnly);
  mUi->btnChooseComponent->setHidden(mContext.readOnly);
  mUi->padSignalMapEditorWidget->setReadOnly(mContext.readOnly);
  setupErrorNotificationWidget(*mUi->errorNotificationWidget);
  setWindowIcon(QIcon(":/img/library/device.png"));

  // Show graphics scenes.
  mComponentGraphicsScene.reset(new GraphicsScene());
  mPackageGraphicsScene.reset(new GraphicsScene());
  mUi->viewComponent->setScene(mComponentGraphicsScene.data());
  mUi->viewPackage->setScene(mPackageGraphicsScene.data());
  mUi->viewPackage->setBackgroundBrush(Qt::black);
  mGraphicsLayerProvider.reset(new DefaultGraphicsLayerProvider());

  // Insert category list editor widget.
  mCategoriesEditorWidget.reset(
      new ComponentCategoryListEditorWidget(mContext.workspace, this));
  mCategoriesEditorWidget->setReadOnly(mContext.readOnly);
  mCategoriesEditorWidget->setRequiresMinimumOneEntry(true);
  int row;
  QFormLayout::ItemRole role;
  mUi->formLayout->getWidgetPosition(mUi->lblCategories, &row, &role);
  mUi->formLayout->setWidget(row, QFormLayout::FieldRole,
                             mCategoriesEditorWidget.data());

  // Load element.
  mDevice.reset(new Device(std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mFileSystem))));  // can throw
  mUi->padSignalMapEditorWidget->setReferences(mUndoStack.data(),
                                               &mDevice->getPadSignalMap());
  updateDeviceComponentUuid(mDevice->getComponentUuid());
  updateDevicePackageUuid(mDevice->getPackageUuid());
  updateMetadata();

  // Show "interface broken" warning when related properties are modified.
  memorizeDeviceInterface();
  setupInterfaceBrokenWarningWidget(*mUi->interfaceBrokenWarningWidget);

  // Reload metadata on undo stack state changes.
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &DeviceEditorWidget::updateMetadata);

  // Reload data on device object changes.
  connect(mDevice.data(), &Device::componentUuidChanged, this,
          &DeviceEditorWidget::updateDeviceComponentUuid);
  connect(mDevice.data(), &Device::packageUuidChanged, this,
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
  connect(mCategoriesEditorWidget.data(),
          &ComponentCategoryListEditorWidget::edited, this,
          &DeviceEditorWidget::commitMetadata);
}

DeviceEditorWidget::~DeviceEditorWidget() noexcept {
  mUi->padSignalMapEditorWidget->setReferences(nullptr, nullptr);
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool DeviceEditorWidget::save() noexcept {
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
  mCategoriesEditorWidget->setUuids(mDevice->getCategories());
}

QString DeviceEditorWidget::commitMetadata() noexcept {
  try {
    QScopedPointer<CmdLibraryElementEdit> cmd(
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

    // Commit all changes.
    mUndoStack->execCmd(cmd.take());  // can throw

    // Reload metadata into widgets to discard invalid input.
    updateMetadata();
  } catch (const Exception& e) {
    return e.getMsg();
  }
  return QString();
}

void DeviceEditorWidget::btnChooseComponentClicked() noexcept {
  ComponentChooserDialog dialog(mContext.workspace,
                                mGraphicsLayerProvider.data(), this);
  if (dialog.exec() == QDialog::Accepted) {
    tl::optional<Uuid> cmpUuid = dialog.getSelectedComponentUuid();
    if (cmpUuid && (*cmpUuid != mDevice->getComponentUuid())) {
      try {
        // load component
        FilePath fp = mContext.workspace.getLibraryDb().getLatestComponent(
            *cmpUuid);  // can throw
        if (!fp.isValid()) {
          throw RuntimeError(__FILE__, __LINE__, tr("Component not found!"));
        }
        Component component(
            std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
                TransactionalFileSystem::openRO(fp))));  // can throw

        // edit device
        QScopedPointer<UndoCommandGroup> cmdGroup(
            new UndoCommandGroup(tr("Change component")));
        QScopedPointer<CmdDeviceEdit> cmdDevEdit(new CmdDeviceEdit(*mDevice));
        cmdDevEdit->setComponentUuid(*cmpUuid);
        cmdGroup->appendChild(cmdDevEdit.take());
        for (DevicePadSignalMapItem& item : mDevice->getPadSignalMap()) {
          tl::optional<Uuid> signalUuid = item.getSignalUuid();
          if (!signalUuid || !component.getSignals().contains(*signalUuid)) {
            QScopedPointer<CmdDevicePadSignalMapItemEdit> cmdItem(
                new CmdDevicePadSignalMapItemEdit(item));
            cmdItem->setSignalUuid(tl::nullopt);
            cmdGroup->appendChild(cmdItem.take());
          }
        }
        mUndoStack->execCmd(cmdGroup.take());
      } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Could not set component"), e.getMsg());
      }
    }
  }
}

void DeviceEditorWidget::btnChoosePackageClicked() noexcept {
  PackageChooserDialog dialog(mContext.workspace, mGraphicsLayerProvider.data(),
                              this);
  if (dialog.exec() == QDialog::Accepted) {
    tl::optional<Uuid> pkgUuid = dialog.getSelectedPackageUuid();
    if (pkgUuid && (*pkgUuid != mDevice->getPackageUuid())) {
      try {
        // load package
        FilePath fp = mContext.workspace.getLibraryDb().getLatestPackage(
            *pkgUuid);  // can throw
        if (!fp.isValid()) {
          throw RuntimeError(__FILE__, __LINE__, tr("Package not found!"));
        }
        Package package(
            std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
                TransactionalFileSystem::openRO(fp))));  // can throw
        QSet<Uuid> pads = package.getPads().getUuidSet();

        // edit device
        QScopedPointer<UndoCommandGroup> cmdGroup(
            new UndoCommandGroup(tr("Change package")));
        QScopedPointer<CmdDeviceEdit> cmdDevEdit(new CmdDeviceEdit(*mDevice));
        cmdDevEdit->setPackageUuid(*pkgUuid);
        cmdGroup->appendChild(cmdDevEdit.take());
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
              std::make_shared<DevicePadSignalMapItem>(pad, tl::nullopt)));
        }
        mUndoStack->execCmd(cmdGroup.take());
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
    FilePath fp = mContext.workspace.getLibraryDb().getLatestComponent(
        uuid);  // can throw
    if (!fp.isValid()) {
      throw RuntimeError(__FILE__, __LINE__, tr("Component not found!"));
    }
    mComponent.reset(new Component(
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
            TransactionalFileSystem::openRO(fp)))));  // can throw
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
        FilePath fp = mContext.workspace.getLibraryDb().getLatestSymbol(
            item.getSymbolUuid());  // can throw
        std::shared_ptr<Symbol> sym = std::make_shared<Symbol>(
            std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
                TransactionalFileSystem::openRO(fp))));  // can throw
        mSymbols.append(sym);
        std::shared_ptr<SymbolPreviewGraphicsItem> graphicsItem =
            std::make_shared<SymbolPreviewGraphicsItem>(
                *mGraphicsLayerProvider, QStringList(), *sym, mComponent.data(),
                symbVar.getUuid(), item.getUuid());
        graphicsItem->setPos(item.getSymbolPosition().toPxQPointF());
        graphicsItem->setRotation(-item.getSymbolRotation().toDeg());
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
    FilePath fp =
        mContext.workspace.getLibraryDb().getLatestPackage(uuid);  // can throw
    if (!fp.isValid()) {
      throw RuntimeError(__FILE__, __LINE__, tr("Package not found!"));
    }
    mPackage.reset(new Package(
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
            TransactionalFileSystem::openRO(fp)))));  // can throw
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
    mFootprintGraphicsItem.reset(
        new FootprintPreviewGraphicsItem(*mGraphicsLayerProvider, QStringList(),
                                         *mPackage->getFootprints().first(),
                                         mPackage.data(), mComponent.data()));
    mPackageGraphicsScene->addItem(*mFootprintGraphicsItem);
    mUi->viewPackage->zoomAll();
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

bool DeviceEditorWidget::runChecks(LibraryElementCheckMessageList& msgs) const {
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
    std::shared_ptr<const LibraryElementCheckMessage> msg, bool applyFix) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (applyFix) fixMsg(*m);  // can throw
      return true;
    }
  }
  return false;
}

bool DeviceEditorWidget::processCheckMessage(
    std::shared_ptr<const LibraryElementCheckMessage> msg, bool applyFix) {
  if (fixMsgHelper<MsgNameNotTitleCase>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingAuthor>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingCategories>(msg, applyFix)) return true;
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
