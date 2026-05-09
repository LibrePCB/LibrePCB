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

#ifndef LIBREPCB_EDITOR_PROJECTLIBRARYMODEL_H
#define LIBREPCB_EDITOR_PROJECTLIBRARYMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "ui.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/version.h>

#include <QtCore>
#include <QtGui>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class LibraryBaseElement;
class Uuid;
class WorkspaceLibraryDb;

namespace editor {

class ProjectEditor;

/*******************************************************************************
 *  Class ProjectLibraryModel
 ******************************************************************************/

/**
 * @brief The ProjectLibraryModel class
 */
class ProjectLibraryModel : public QObject,
                            public slint::Model<ui::ProjectLibraryItemData> {
  Q_OBJECT

public:
  // Types
  struct LibraryInfo {
    QPixmap icon;
    QString name;
  };
  struct ElementInfo {
    FilePath libFp;
    FilePath fp;
    Version version;
    bool deprecated;
  };

  // Constructors / Destructor
  ProjectLibraryModel() = delete;
  ProjectLibraryModel(const ProjectLibraryModel& other) = delete;
  explicit ProjectLibraryModel(ProjectEditor& editor,
                               const WorkspaceLibraryDb& db,
                               QObject* parent = nullptr) noexcept;
  ~ProjectLibraryModel() noexcept override;

  // General Methods
  int getDowngradedCount() const noexcept { return mDowngradedElements; }
  int getCheckableCount() const noexcept { return mCheckableElements.count(); }
  int getCheckedCount() const noexcept { return mCheckedElements.count(); }
  void setAllChecked(bool checked) noexcept;
  bool copyToLibrary(const FilePath& libFp, QStringList& errors) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::ProjectLibraryItemData> row_data(
      std::size_t i) const override;
  void set_row_data(std::size_t index,
                    const ui::ProjectLibraryItemData& data) noexcept override;

  // Operator Overloadings
  ProjectLibraryModel& operator=(const ProjectLibraryModel& rhs) = delete;

signals:
  void statisticsModified();
  void openTriggered(ui::LibraryTreeViewItemType type, const FilePath& fp);

private:  // Methods
  void reset() noexcept;
  void refresh() noexcept;
  const LibraryInfo& getLibraryInfo(const FilePath& fp) noexcept;
  template <typename T>
  const std::optional<ElementInfo>& getElementInfo(
      const LibraryBaseElement& obj) noexcept;

private:
  QPointer<ProjectEditor> mProjectEditor;
  QPointer<const WorkspaceLibraryDb> mDb;
  std::vector<ui::ProjectLibraryItemData> mItems;
  int mDowngradedElements;
  QSet<QString> mCheckableElements;
  QSet<QString> mCheckedElements;

  // Cache
  QHash<FilePath, LibraryInfo> mLibraryCache;
  QHash<QString, std::optional<ElementInfo>> mElementCache;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
