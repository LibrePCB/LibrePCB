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
#include "projectlibrarymodel.h"

#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../projecteditor.h"

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/utils/scopeguard.h>
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

ProjectLibraryModel::ProjectLibraryModel(ProjectEditor& editor,
                                         const WorkspaceLibraryDb& db,
                                         QObject* parent) noexcept
  : QObject(parent), mProjectEditor(&editor), mDb(&db), mDowngradedElements(0) {
  connect(&editor.getUndoStack(), &UndoStack::stateModified, this,
          &ProjectLibraryModel::refresh, Qt::QueuedConnection);
  connect(mDb, &WorkspaceLibraryDb::scanSucceeded, this,
          &ProjectLibraryModel::reset, Qt::QueuedConnection);
  refresh();
}

ProjectLibraryModel::~ProjectLibraryModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ProjectLibraryModel::setAllChecked(bool checked) noexcept {
  for (std::size_t i = 0; i < mItems.size(); ++i) {
    auto& item = mItems[i];
    const QString path = s2q(item.path);
    if (mCheckableElements.contains(path) && (item.checked != checked)) {
      item.checked = checked;
      if (checked) {
        mCheckedElements.insert(path);
      } else {
        mCheckedElements.remove(path);
      }
      notify_row_changed(i);
    }
  }
  emit statisticsModified();
}

bool ProjectLibraryModel::copyToLibrary(const FilePath& libFp,
                                        QStringList& errors) noexcept {
  QApplication::setOverrideCursor(Qt::WaitCursor);
  auto cursorScopeGuard =
      scopeGuard([]() { QApplication::restoreOverrideCursor(); });

  int count = 0;
  const QSet<QString> validPrefixes = {"dev", "cmp", "sym", "pkg"};
  for (std::size_t i = 0; i < mItems.size(); ++i) {
    auto& item = mItems[i];
    if (item.latest_version.empty() && item.checked && mProjectEditor) {
      // Show spinner in UI.
      item.copy_in_progress = true;
      notify_row_changed(i);

      // Try to copy.
      const QString srcRelPath = s2q(item.path);
      QStringList dstRelPath = srcRelPath.split("/");
      if (dstRelPath.count() >= 2) {
        dstRelPath = dstRelPath.last(2);
      }
      const FilePath dst = libFp.getPathTo(dstRelPath.join("/"));
      qDebug().nospace() << "Copy from " << srcRelPath << " to "
                         << dst.toNative() << "...";
      try {
        const TransactionalDirectory srcDir(
            mProjectEditor->getProject().getDirectory(), srcRelPath);
        if ((srcDir.getFiles().isEmpty()) || (!dst.isValid()) ||
            (dstRelPath.count() != 2) ||
            (!validPrefixes.contains(dstRelPath.first()))) {
          throw LogicError(__FILE__, __LINE__);
        }
        if (dst.isExistingDir()) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("Directory exists already: %1").arg(dst.toNative()));
        }
        ++count;
        auto sg =
            ScopeGuard([dst]() { QDir(dst.toStr()).removeRecursively(); });
        {
          auto dstFs = TransactionalFileSystem::openRW(dst);  // can throw
          TransactionalDirectory dstDir(dstFs);
          srcDir.copyTo(dstDir);  // can throw
          dstFs->save();  // can throw
        }
        sg.dismiss();
      } catch (const Exception& e) {
        errors.append(e.getMsg());
      }
    }
  }
  return count > 0;
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ProjectLibraryModel::row_count() const {
  return mItems.size();
}

std::optional<ui::ProjectLibraryItemData> ProjectLibraryModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

void ProjectLibraryModel::set_row_data(
    std::size_t index, const ui::ProjectLibraryItemData& data) noexcept {
  if (index >= static_cast<size_t>(mItems.size())) return;

  const QString path = s2q(data.path);
  if (data.action == ui::ProjectLibraryItemAction::Open) {
    if (auto info = mElementCache.value(path)) {
      emit openTriggered(data.type, info->fp);
    }
  } else if (mCheckableElements.contains(path)) {
    mItems[index].checked = data.checked;
    if (data.checked) {
      mCheckedElements.insert(path);
    } else {
      mCheckedElements.remove(path);
    }
    notify_row_changed(index);
    emit statisticsModified();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectLibraryModel::reset() noexcept {
  mLibraryCache.clear();
  mElementCache.clear();
  refresh();
}

void ProjectLibraryModel::refresh() noexcept {
  mItems.clear();
  mCheckableElements.clear();
  mDowngradedElements = 0;

  auto add = [this](
                 ui::LibraryTreeViewItemType type,
                 const LibraryBaseElement* obj,
                 const std::optional<ProjectLibraryModel::ElementInfo>& info) {
    const ProjectLibraryModel::LibraryInfo* libInfo =
        info ? &getLibraryInfo(info->libFp) : nullptr;
    const QString path = obj->getDirectory().getPath();
    const bool downgrade = info && (info->version < obj->getVersion());
    const bool deprecated = obj->isDeprecated() || (info && info->deprecated);
    mItems.push_back(ui::ProjectLibraryItemData{
        type,  // Type
        q2s(path),  // Path
        q2s(*obj->getNames().getDefaultValue()),  // Name
        q2s(obj->getVersion().toPrettyStr(3)),  // Version
        info ? q2s(info->version.toPrettyStr(3))
             : slint::SharedString(),  // Latest version
        libInfo ? q2s(libInfo->icon) : slint::Image(),  // Library icon
        libInfo ? q2s(libInfo->name) : slint::SharedString(),  // Library name
        deprecated,  // Deprecated
        downgrade,  // Downgrade
        mCheckedElements.contains(path),  // Checked
        false,  // Copy in progress
        ui::ProjectLibraryItemAction::None,  // Action
    });
    if (downgrade) {
      ++mDowngradedElements;
    }
    if (!info) {
      mCheckableElements.insert(path);
    }
  };

  if (mProjectEditor && mDb) {
    const auto& lib = mProjectEditor->getProject().getLibrary();
    for (const Device* prjObj : lib.getDevices()) {
      add(ui::LibraryTreeViewItemType::Device, prjObj,
          getElementInfo<Device>(*prjObj));
    }
    for (const Component* prjObj : lib.getComponents()) {
      add(ui::LibraryTreeViewItemType::Component, prjObj,
          getElementInfo<Component>(*prjObj));
    }
    for (const Symbol* prjObj : lib.getSymbols()) {
      add(ui::LibraryTreeViewItemType::Symbol, prjObj,
          getElementInfo<Symbol>(*prjObj));
    }
    for (const Package* prjObj : lib.getPackages()) {
      add(ui::LibraryTreeViewItemType::Package, prjObj,
          getElementInfo<Package>(*prjObj));
    }
  }

  Toolbox::sortNumeric(
      mItems,
      [](QCollator& compare, const ui::ProjectLibraryItemData& a,
         const ui::ProjectLibraryItemData& b) {
        if (a.type != b.type) {
          return a.type < b.type;
        } else {
          return compare(s2q(a.name), s2q(b.name));
        }
      });

  mCheckedElements &= mCheckableElements;

  notify_reset();
  emit statisticsModified();
}

const ProjectLibraryModel::LibraryInfo& ProjectLibraryModel::getLibraryInfo(
    const FilePath& fp) noexcept {
  auto it = mLibraryCache.find(fp);
  if (it == mLibraryCache.end()) {
    ProjectLibraryModel::LibraryInfo info;
    try {
      mDb->getLibraryMetadata(fp, &info.icon);  // can throw
      mDb->getTranslations<Library>(
          fp, mProjectEditor->getProject().getLocaleOrder(),
          &info.name);  // can throw
    } catch (const Exception& e) {
      qCritical() << "Failed to get library info:" << e.getMsg();
    }
    it = mLibraryCache.insert(fp, info);
  }
  return *it;
}

template <typename T>
const std::optional<ProjectLibraryModel::ElementInfo>&
    ProjectLibraryModel::getElementInfo(
        const LibraryBaseElement& obj) noexcept {
  const QString path = obj.getDirectory().getPath();
  auto it = mElementCache.find(path);
  if (it == mElementCache.end()) {
    std::optional<ElementInfo> info;
    try {
      const FilePath fp = mDb->getLatest<T>(obj.getUuid());  // can throw
      if (fp.isValid()) {
        ProjectLibraryModel::ElementInfo obj{fp.getParentDir().getParentDir(),
                                             fp, Version::fromString("0.1"),
                                             false};
        if (mDb->getMetadata<T>(fp, nullptr, &obj.version,
                                &obj.deprecated)) {  // can throw
          info = obj;
        }
      }
    } catch (const Exception& e) {
      qCritical() << "Failed to get library element info:" << e.getMsg();
    }
    it = mElementCache.insert(path, info);
  }
  return *it;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
