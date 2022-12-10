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

#ifndef LIBREPCB_EDITOR_PATHMODEL_H
#define LIBREPCB_EDITOR_PATHMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/path.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class PathModel
 ******************************************************************************/

/**
 * @brief The PathModel class implements QAbstractTableModel for librepcb::Path
 */
class PathModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    COLUMN_X,
    COLUMN_Y,
    COLUMN_ANGLE,
    COLUMN_ACTIONS,
    _COLUMN_COUNT
  };

  // Constructors / Destructor
  PathModel() = delete;
  PathModel(const PathModel& other) noexcept;
  explicit PathModel(QObject* parent = nullptr) noexcept;
  ~PathModel() noexcept;

  // Setters
  void setPath(const Path& path) noexcept;
  const Path& getPath() const noexcept { return mPath; }

  // Slots
  void addItem(const QVariant& editData) noexcept;
  void copyItem(const QVariant& editData) noexcept;
  void removeItem(const QVariant& editData) noexcept;
  void moveItemUp(const QVariant& editData) noexcept;
  void moveItemDown(const QVariant& editData) noexcept;

  // Inherited from QAbstractItemModel
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) override;

  // Operator Overloadings
  PathModel& operator=(const PathModel& rhs) noexcept;

signals:
  void pathChanged(const Path& path);

private:  // Data
  Path mPath;
  Vertex mNewVertex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
