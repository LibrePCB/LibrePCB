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

#ifndef LIBREPCB_EDITOR_ZONEPROPERTIESDIALOG_H
#define LIBREPCB_EDITOR_ZONEPROPERTIESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Zone;
class Layer;
class LengthUnit;
class Zone;

namespace editor {

class IF_GraphicsLayerProvider;
class UndoStack;

namespace Ui {
class ZonePropertiesDialog;
}

/*******************************************************************************
 *  Class ZonePropertiesDialog
 ******************************************************************************/

/**
 * @brief The ZonePropertiesDialog class
 */
class ZonePropertiesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  ZonePropertiesDialog() = delete;
  ZonePropertiesDialog(const ZonePropertiesDialog& other) = delete;
  ZonePropertiesDialog(Zone& zone, UndoStack& undoStack,
                       const LengthUnit& lengthUnit,
                       const IF_GraphicsLayerProvider& lp,
                       const QString& settingsPrefix,
                       QWidget* parent = nullptr) noexcept;
  ZonePropertiesDialog(BI_Zone& zone, UndoStack& undoStack,
                       const LengthUnit& lengthUnit,
                       const IF_GraphicsLayerProvider& lp,
                       const QString& settingsPrefix,
                       QWidget* parent = nullptr) noexcept;
  ~ZonePropertiesDialog() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;

  // Operator Overloadings
  ZonePropertiesDialog& operator=(const ZonePropertiesDialog& rhs) = delete;

private:  // Methods
  ZonePropertiesDialog(Zone* libZone, BI_Zone* boardZone,
                       const QList<const Layer*> allLayers,
                       UndoStack& undoStack, const LengthUnit& lengthUnit,
                       const IF_GraphicsLayerProvider& lp,
                       const QString& settingsPrefix,
                       QWidget* parent = nullptr) noexcept;
  template <typename T>
  void load(const T& obj, const QSet<const Layer*> checkedLayers) noexcept;
  void buttonBoxClicked(QAbstractButton* button) noexcept;
  bool applyChanges() noexcept;
  template <typename T>
  void applyChanges(T& cmd);

private:  // Data
  Zone* mLibraryObj;
  BI_Zone* mBoardObj;
  UndoStack& mUndoStack;
  QScopedPointer<Ui::ZonePropertiesDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
