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
#include "componentassemblyoptionlisteditorwidget.h"

#include "../widgets/editabletablewidget.h"
#include "addcomponentdialog.h"

#include <librepcb/core/attribute/attribute.h>
#include <librepcb/core/attribute/attributetype.h>
#include <librepcb/core/attribute/attributeunit.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentassemblyoption.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
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

// See
// https://stackoverflow.com/questions/2801959/making-only-one-column-of-a-qtreewidgetitem-editable
class NoEditDelegate : public QStyledItemDelegate {
public:
  NoEditDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
  virtual QWidget* createEditor(QWidget*, const QStyleOptionViewItem&,
                                const QModelIndex&) const {
    return nullptr;
  }
};

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentAssemblyOptionListEditorWidget::
    ComponentAssemblyOptionListEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mOptions(),
    mTreeWidget(new QTreeWidget(this)),
    mOnListEditedSlot(
        *this, &ComponentAssemblyOptionListEditorWidget::optionListEdited) {
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(mTreeWidget.data());

  QFrame* vLine = new QFrame(this);
  vLine->setFrameShape(QFrame::VLine);
  vLine->setFrameShadow(QFrame::Sunken);
  vLine->setLineWidth(1);
  vLine->setFixedWidth(3);
  layout->addWidget(vLine);

  QVBoxLayout* buttonsLayout = new QVBoxLayout();
  buttonsLayout->setContentsMargins(0, 0, 0, 0);
  buttonsLayout->setSpacing(0);
  layout->addLayout(buttonsLayout);

  mAddOptionButton.reset(new QToolButton(this));
  mAddOptionButton->setIcon(QIcon(":/img/library/device.png"));
  connect(mAddOptionButton.data(), &QToolButton::clicked, this,
          &ComponentAssemblyOptionListEditorWidget::addOption);
  buttonsLayout->addWidget(mAddOptionButton.data());

  mAddPartButton.reset(new QToolButton(this));
  mAddPartButton->setIcon(QIcon(":/img/library/part.png"));
  connect(mAddPartButton.data(), &QToolButton::clicked, this,
          &ComponentAssemblyOptionListEditorWidget::addPart);
  buttonsLayout->addWidget(mAddPartButton.data());

  mEditButton.reset(new QToolButton(this));
  mEditButton->setIcon(QIcon(":/img/actions/edit.png"));
  connect(mEditButton.data(), &QToolButton::clicked, this,
          &ComponentAssemblyOptionListEditorWidget::editOptionOrPart);
  buttonsLayout->addWidget(mEditButton.data());

  mRemoveButton.reset(new QToolButton(this));
  mRemoveButton->setIcon(QIcon(":/img/actions/minus.png"));
  connect(mRemoveButton.data(), &QToolButton::clicked, this,
          &ComponentAssemblyOptionListEditorWidget::removeOptionOrPart);
  buttonsLayout->addWidget(mRemoveButton.data());

  buttonsLayout->addStretch(100);

  mTreeWidget->setRootIsDecorated(false);
  mTreeWidget->setAllColumnsShowFocus(true);
  mTreeWidget->setExpandsOnDoubleClick(false);
  mTreeWidget->setHeaderLabels({tr("Board Device"), tr("Part Number"),
                                tr("Manufacturer"), tr("Attributes")});
  mTreeWidget->header()->setMinimumSectionSize(10);
  mTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  mTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  mTreeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  mTreeWidget->header()->setSectionResizeMode(3, QHeaderView::Stretch);
  mTreeWidget->setItemDelegateForColumn(0, new NoEditDelegate(this));
  mTreeWidget->setItemDelegateForColumn(3, new NoEditDelegate(this));
  connect(mTreeWidget.data(), &QTreeWidget::itemChanged, this,
          &ComponentAssemblyOptionListEditorWidget::itemChanged);
  connect(mTreeWidget.data(), &QTreeWidget::itemSelectionChanged, this,
          &ComponentAssemblyOptionListEditorWidget::itemSelectionChanged);

  mOptions.onEdited.attach(mOnListEditedSlot);

  itemSelectionChanged();  // Update enabled state of buttons.
  setEnabled(false);
}

ComponentAssemblyOptionListEditorWidget::
    ~ComponentAssemblyOptionListEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentAssemblyOptionListEditorWidget::setFrameStyle(
    int style) noexcept {
  mTreeWidget->setFrameStyle(style);
}

void ComponentAssemblyOptionListEditorWidget::setReferences(
    const Workspace* ws, const Project* project,
    ComponentInstance* component) noexcept {
  mWorkspace = ws;
  mProject = project;
  mComponent = component;
  if (component) {
    mOptions = component->getAssemblyOptions();
  } else {
    mOptions.clear();
  }
  setEnabled(mWorkspace && mProject && mComponent);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentAssemblyOptionListEditorWidget::addOption() noexcept {
  if ((!mWorkspace) || (!mProject) || (!mComponent)) {
    return;
  }

  try {
    const std::pair<int, int> currentIndices =
        getIndices(mTreeWidget->currentItem());
    const int newIndex = ((currentIndices.first >= 0) &&
                          (currentIndices.first < mOptions.count()))
        ? (currentIndices.first + 1)
        : mOptions.count();

    AddComponentDialog dlg(mWorkspace->getLibraryDb(),
                           mProject->getLocaleOrder(), mProject->getNormOrder(),
                           mWorkspace->getSettings().themes.getActive(), this);
    dlg.selectComponentByKeyword(
        mComponent->getLibComponent().getUuid().toStr());
    if (dlg.exec() != QDialog::Accepted) {
      return;
    }

    auto device = dlg.getSelectedDevice();
    if (!device) {
      return;
    }

    // Check compatibility.
    if (device->getComponentUuid() != mComponent->getLibComponent().getUuid()) {
      const int answer = QMessageBox::warning(
          this, tr("Device Compatibility Unknown"),
          tr("The selected device is not related to the component placed in "
             "the schematic, thus LibrePCB cannot validate if it is compatible!"
             "\n\nAre you sure the footprint and pinout of the selected device "
             "are compatible with the component?"),
          QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);
      if (answer != QMessageBox::Yes) {
        return;
      }
    }

    PartList parts;
    if (auto part = dlg.getSelectedPart()) {
      auto copy = std::make_shared<Part>(*part);
      copy->getAttributes() = part->getAttributes() | device->getAttributes();
      parts.append(copy);
    }
    mOptions.insert(newIndex,
                    std::make_shared<ComponentAssemblyOption>(
                        device->getUuid(), device->getAttributes(), parts));
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

bool ComponentAssemblyOptionListEditorWidget::addPart() noexcept {
  if ((!mWorkspace) || (!mProject) || (!mComponent)) {
    return false;
  }

  try {
    const std::pair<int, int> currentIndices =
        getIndices(mTreeWidget->currentItem());
    auto option = mOptions.value(currentIndices.first);
    if (!option) {
      return false;
    }
    const int newIndex = ((currentIndices.second >= 0) &&
                          (currentIndices.second < option->getParts().count()))
        ? (currentIndices.second + 1)
        : option->getParts().count();

    AddComponentDialog dlg(mWorkspace->getLibraryDb(),
                           mProject->getLocaleOrder(), mProject->getNormOrder(),
                           mWorkspace->getSettings().themes.getActive(), this);
    dlg.selectComponentByKeyword(
        mComponent->getLibComponent().getUuid().toStr(), option->getDevice());
    std::shared_ptr<const Device> device;
    std::shared_ptr<const Part> part;
    if (dlg.exec() == QDialog::Accepted) {
      device = dlg.getSelectedDevice();
      part = dlg.getSelectedPart();
    }

    // Check compatibility.
    if (device && (device->getUuid() != option->getDevice())) {
      const int answer = QMessageBox::warning(
          this, tr("Part Compatibility Unknown"),
          tr("The selected part is taken from a different device than this "
             "assembly option is valid for, thus LibrePCB cannot validate if "
             "it is compatible!\n\n"
             "Are you sure the footprint and pinout of the selected part are "
             "compatible with the device?"),
          QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);
      if (answer != QMessageBox::Yes) {
        return false;
      }
    }

    auto newPart = std::make_shared<Part>(SimpleString(""), SimpleString(""),
                                          AttributeList{});
    if (device && part) {
      newPart->setMpn(part->getMpn());
      newPart->setManufacturer(part->getManufacturer());
      newPart->getAttributes() =
          part->getAttributes() | device->getAttributes();
    } else if (device) {
      newPart->getAttributes() = device->getAttributes();
    }
    option->getParts().insert(newIndex, newPart);
    return true;
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
    return false;
  }
}

void ComponentAssemblyOptionListEditorWidget::editOptionOrPart() noexcept {
  const std::pair<int, int> currentIndices =
      getIndices(mTreeWidget->currentItem());
  auto option = mOptions.value(currentIndices.first);
  const bool hasParts =
      option && option->getParts().contains(currentIndices.second);
  if (addPart() && hasParts) {
    removeOptionOrPart();
  }
}

void ComponentAssemblyOptionListEditorWidget::removeOptionOrPart() noexcept {
  if ((!mWorkspace) || (!mProject) || (!mComponent)) {
    return;
  }

  try {
    const std::pair<int, int> currentIndices =
        getIndices(mTreeWidget->currentItem());
    auto option = mOptions.value(currentIndices.first);
    if (!option) {
      return;
    }

    if (option->getParts().contains(currentIndices.second)) {
      option->getParts().remove(currentIndices.second);
    } else {
      mOptions.remove(currentIndices.first);
    }
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

void ComponentAssemblyOptionListEditorWidget::itemChanged(QTreeWidgetItem* item,
                                                          int column) noexcept {
  if (!item) {
    return;
  }

  try {
    const std::pair<int, int> indices = getIndices(item);
    auto option = mOptions.value(indices.first);
    if (!option) {
      return;
    }

    std::shared_ptr<Part> part = option->getParts().value(indices.second);
    const bool newPartCreated = !part;
    if (newPartCreated) {
      part = std::make_shared<Part>(SimpleString(""), SimpleString(""),
                                    option->getAttributes());
    }
    if (column == 1) {
      part->setMpn(cleanSimpleString(item->text(column)));
    } else if (column == 2) {
      part->setManufacturer(cleanSimpleString(item->text(column)));
    }
    if (newPartCreated) {
      option->getParts().append(part);
    }
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

void ComponentAssemblyOptionListEditorWidget::itemSelectionChanged() noexcept {
  const std::pair<int, int> pair = getIndices(mTreeWidget->currentItem());
  std::shared_ptr<ComponentAssemblyOption> option = mOptions.value(pair.first);
  std::shared_ptr<Part> part =
      option ? option->getParts().value(pair.second) : nullptr;
  mAddPartButton->setEnabled(option || part);
  mEditButton->setEnabled(option || part);
  mRemoveButton->setEnabled(option || part);
  selectedPartChanged(part);
}

std::pair<int, int> ComponentAssemblyOptionListEditorWidget::getIndices(
    QTreeWidgetItem* item) noexcept {
  int optionIndex = -1;
  int partIndex = -1;

  if (item) {
    if (QTreeWidgetItem* parentItem = item->parent()) {
      optionIndex = mTreeWidget->indexOfTopLevelItem(parentItem);
      partIndex = parentItem->indexOfChild(item) + 1;
    } else {
      optionIndex = mTreeWidget->indexOfTopLevelItem(item);
      partIndex = 0;
    }
  }

  return std::make_pair(optionIndex, partIndex);
}

void ComponentAssemblyOptionListEditorWidget::optionListEdited(
    const ComponentAssemblyOptionList& list, int index,
    const std::shared_ptr<const ComponentAssemblyOption>& obj,
    ComponentAssemblyOptionList::Event event) noexcept {
  Q_UNUSED(list);

  if ((!mWorkspace) || (!mProject) || (!mComponent)) {
    return;
  }

  auto fillOptionRow = [this](QTreeWidgetItem* item,
                              const ComponentAssemblyOption& option) {
    item->setIcon(0, QIcon(":/img/library/device.png"));
    QString devName = option.getDevice().toStr().left(8) % "...";
    QString toolTip;
    try {
      FilePath fp =
          mWorkspace->getLibraryDb().getLatest<Device>(option.getDevice());
      mWorkspace->getLibraryDb().getTranslations<Device>(
          fp, mProject->getLocaleOrder(), &devName);

      QString pkgName;
      Uuid pkgUuid = Uuid::createRandom();
      mWorkspace->getLibraryDb().getDeviceMetadata(fp, nullptr, &pkgUuid);
      fp = mWorkspace->getLibraryDb().getLatest<Package>(pkgUuid);
      mWorkspace->getLibraryDb().getTranslations<Package>(
          fp, mProject->getLocaleOrder(), &pkgName);
      toolTip = tr("Package: %1").arg(pkgName);
    } catch (const Exception& e) {
      qWarning() << "Failed to fetch device metadata:" << e.getMsg();
    }
    item->setText(0, devName);
    item->setToolTip(0, toolTip);
  };

  auto fillPartRow = [](QTreeWidgetItem* item, std::shared_ptr<const Part> part,
                        int index) {
    if (index > 0) {
      item->setText(0, "↳ " % tr("Alternative %1:").arg(index));
    }
    item->setIcon(1, part ? QIcon(":/img/library/part.png") : QIcon());
    item->setText(1, part ? *part->getMpn() : QString());
    item->setBackground(
        1,
        (!part) ? QBrush(Qt::red)
                : (part->getMpn()->isEmpty() ? QBrush(Qt::yellow) : QBrush()));
    item->setText(2, part ? *part->getManufacturer() : QString());
    item->setBackground(
        2,
        (!part) ? QBrush(Qt::red)
                : (part->getManufacturer()->isEmpty() ? QBrush(Qt::yellow)
                                                      : QBrush()));
    item->setText(3, part ? part->getAttributeValuesTr().join(" ") : QString());
    item->setBackground(3, (!part) ? QBrush(Qt::red) : QBrush());
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable |
                   Qt::ItemIsSelectable);
  };

  // Avoid recursion!
  QSignalBlocker blocker(mTreeWidget.data());

  switch (event) {
    case ComponentAssemblyOptionList::Event::ElementAdded: {
      QTreeWidgetItem* optItem = new QTreeWidgetItem();
      fillOptionRow(optItem, *obj);
      fillPartRow(optItem, obj->getParts().value(0), 0);
      for (int i = 1; i < obj->getParts().count(); ++i) {
        QTreeWidgetItem* partItem = new QTreeWidgetItem(optItem);
        fillPartRow(partItem, obj->getParts().value(i), i);
      }
      mTreeWidget->insertTopLevelItem(index, optItem);
      optItem->setExpanded(true);
      break;
    }
    case ComponentAssemblyOptionList::Event::ElementRemoved: {
      delete mTreeWidget->takeTopLevelItem(index);
      itemSelectionChanged();  // Might be needed or not.
      break;
    }
    case ComponentAssemblyOptionList::Event::ElementEdited: {
      if (QTreeWidgetItem* optItem = mTreeWidget->topLevelItem(index)) {
        fillOptionRow(optItem, *obj);
        fillPartRow(optItem, obj->getParts().value(0), 0);
        optItem->setExpanded(false);  // Workaround for missing UI update.
        mTreeWidget->update();  // Workaround for missing UI update.
        const int alternativesCount = std::max(obj->getParts().count() - 1, 0);
        while (optItem->childCount() > alternativesCount) {
          optItem->takeChild(alternativesCount);
        }
        for (int i = 1; i < obj->getParts().count(); ++i) {
          QTreeWidgetItem* partItem = optItem->child(i - 1);
          if (!partItem) {
            partItem = new QTreeWidgetItem(optItem);
          }
          fillPartRow(partItem, obj->getParts().value(i), i);
        }
        optItem->setExpanded(true);
        itemSelectionChanged();  // Might be needed or not.
      } else {
        qWarning() << "ComponentAssemblyOptionListEditorWidget: Invalid index.";
      }
      break;
    }
    default:
      break;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
