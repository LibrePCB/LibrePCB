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

#ifndef LIBREPCB_EDITOR_PARTINFORMATIONDELEGATE_H
#define LIBREPCB_EDITOR_PARTINFORMATIONDELEGATE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../project/partinformationprovider.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class PartInformationDelegate
 ******************************************************************************/

/**
 * @brief Subclass of QStyledItemDelegate to display part status/price
 */
class PartInformationDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  struct Data {
    bool initialized = false;
    bool infoRequested = false;
    int progress = 0;
    PartInformationProvider::Part part;
    std::shared_ptr<const PartInformationProvider::PartInformation> info;
    int priceQuantity = 1;

    QSize calcSizeHint(const QStyleOptionViewItem& option) const noexcept;
    QString getDisplayText(bool maxLen = false) const noexcept;
    bool getColors(QBrush& background, QPen& outline,
                   QPen& text) const noexcept;
  };

  // Constructors / Destructor
  explicit PartInformationDelegate(bool fillCell,
                                   QObject* parent = nullptr) noexcept;
  PartInformationDelegate(const PartInformationDelegate& other) = delete;
  ~PartInformationDelegate() noexcept;

  // Inherited from QStyledItemDelegate
  virtual QSize sizeHint(const QStyleOptionViewItem& option,
                         const QModelIndex& index) const noexcept override;
  virtual void paint(QPainter* painter, const QStyleOptionViewItem& option,
                     const QModelIndex& index) const override;

  // Operator Overloadings
  PartInformationDelegate& operator=(const PartInformationDelegate& rhs) =
      delete;

private:  // Methods
  bool getData(const QModelIndex& index, Data& data) const noexcept;

private:  // Data
  const bool mFillCell;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::editor::PartInformationDelegate::Data)

#endif
