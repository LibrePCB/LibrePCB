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

#ifndef LIBREPCB_EDITOR_POLYGONPROPERTIESDIALOG_H
#define LIBREPCB_EDITOR_POLYGONPROPERTIESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Polygon;
class Layer;
class LengthUnit;
class Polygon;

namespace editor {

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
                          const QSet<const Layer*>& layers,
                          const LengthUnit& lengthUnit,
                          const QString& settingsPrefix,
                          QWidget* parent = nullptr) noexcept;
  PolygonPropertiesDialog(BI_Polygon& polygon, UndoStack& undoStack,
                          const QSet<const Layer*>& layers,
                          const LengthUnit& lengthUnit,
                          const QString& settingsPrefix,
                          QWidget* parent = nullptr) noexcept;
  ~PolygonPropertiesDialog() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;

  // Operator Overloadings
  PolygonPropertiesDialog& operator=(const PolygonPropertiesDialog& rhs) =
      delete;

private:  // Methods
  PolygonPropertiesDialog(Polygon* libPolygon, BI_Polygon* boardPolygon,
                          UndoStack& undoStack,
                          const QSet<const Layer*>& layers,
                          const LengthUnit& lengthUnit,
                          const QString& settingsPrefix,
                          QWidget* parent = nullptr) noexcept;
  template <typename T>
  void load(const T& obj) noexcept;
  void buttonBoxClicked(QAbstractButton* button) noexcept;
  bool applyChanges() noexcept;
  template <typename T>
  void applyChanges(T& cmd);

private:  // Data
  Polygon* mLibraryObj;
  BI_Polygon* mBoardObj;
  UndoStack& mUndoStack;
  QScopedPointer<Ui::PolygonPropertiesDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
