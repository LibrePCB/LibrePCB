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

#ifndef LIBREPCB_GUI_OBJECTLISTMODEL_H
#define LIBREPCB_GUI_OBJECTLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace gui {

/*******************************************************************************
 *  Class ObjectListModel
 ******************************************************************************/

/**
 * @brief Model for a list of objects
 */
class ObjectListModel : public QAbstractListModel {
  Q_OBJECT

public:
  // Types
  enum Role : int {
    ROLE_OBJECT = Qt::UserRole,
  };

  // Constructors / Destructor
  ObjectListModel() = delete;
  ObjectListModel(QObject* parent = nullptr);
  ObjectListModel(const ObjectListModel& other) noexcept = delete;
  virtual ~ObjectListModel() noexcept;

  // Properties
  Q_PROPERTY(bool empty READ isEmpty NOTIFY countChanged)

  // Getters
  bool isEmpty() const noexcept { return mObjects.isEmpty(); }

  // General Methods
  void insert(int index, std::shared_ptr<QObject> obj) noexcept;

  // Reimplemented Methods
  virtual QHash<int, QByteArray> roleNames() const noexcept override;
  virtual int rowCount(const QModelIndex& parent) const noexcept override;
  virtual QVariant data(const QModelIndex& index,
                        int role) const noexcept override;

signals:
  void countChanged(int count);

private:
  QVector<std::shared_ptr<QObject>> mObjects;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb

#endif
