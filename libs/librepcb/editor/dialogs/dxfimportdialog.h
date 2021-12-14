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

#ifndef LIBREPCB_EDITOR_DXFIMPORTDIALOG_H
#define LIBREPCB_EDITOR_DXFIMPORTDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/graphics/graphicslayername.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;
class LengthUnit;

namespace editor {

namespace Ui {
class DxfImportDialog;
}

/*******************************************************************************
 *  Class DxfImportDialog
 ******************************************************************************/

/**
 * @brief This class provides a Dialog (GUI) to choose DXF import settings
 */
class DxfImportDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  DxfImportDialog() = delete;
  DxfImportDialog(const DxfImportDialog& other) = delete;
  explicit DxfImportDialog(QList<GraphicsLayer*> layers,
                           const GraphicsLayerName& defaultLayer,
                           bool supportHoles, const LengthUnit& lengthUnit,
                           const QString& settingsPrefix,
                           QWidget* parent = nullptr) noexcept;
  ~DxfImportDialog() noexcept;

  // Getters
  GraphicsLayerName getLayerName() const noexcept;
  bool getImportCirclesAsDrills() const noexcept;
  UnsignedLength getLineWidth() const noexcept;
  qreal getScaleFactor() const noexcept;
  tl::optional<Point> getPlacementPosition() const noexcept;

  // General Methods
  FilePath chooseFile() const noexcept;
  static void throwNoObjectsImportedError();

  // Operator Overloadings
  DxfImportDialog& operator=(const DxfImportDialog& rhs) = delete;

private:  // Data
  QScopedPointer<Ui::DxfImportDialog> mUi;
  QString mSettingsPrefix;
  GraphicsLayerName mDefaultLayer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
