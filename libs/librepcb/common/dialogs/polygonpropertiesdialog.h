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

#ifndef LIBREPCB_COMMON_POLYGONPROPERTIESDIALOG_H
#define LIBREPCB_COMMON_POLYGONPROPERTIESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/vertex.h"
#include "../units/all_length_units.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;
class Polygon;
class UndoStack;

namespace Ui {
class PolygonPropertiesDialog;
}

/*******************************************************************************
 *  Class PolygonPropertiesDialog
 ******************************************************************************/

/**
 * @brief The PolygonPropertiesDialog class
 */
class PolygonPropertiesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  PolygonPropertiesDialog() = delete;
  PolygonPropertiesDialog(const PolygonPropertiesDialog& other) = delete;
  PolygonPropertiesDialog(Polygon& polygon, UndoStack& undoStack,
                          QList<GraphicsLayer*> layers,
                          const LengthUnit& lengthUnit,
                          const QString& settingsPrefix,
                          QWidget* parent = nullptr) noexcept;
  ~PolygonPropertiesDialog() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;

  // Operator Overloadings
  PolygonPropertiesDialog& operator=(const PolygonPropertiesDialog& rhs) =
      delete;

private:  // GUI Events
  void buttonBoxClicked(QAbstractButton* button) noexcept;

private:  // Methods
  bool applyChanges() noexcept;
  void selectLayerNameInCombobox(const QString& name) noexcept;

private:  // Data
  Polygon& mPolygon;
  UndoStack& mUndoStack;
  QScopedPointer<Ui::PolygonPropertiesDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
