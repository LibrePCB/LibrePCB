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

#ifndef LIBREPCB_GRAPHICSLAYERCOMBOBOX_H
#define LIBREPCB_GRAPHICSLAYERCOMBOBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;

/*******************************************************************************
 *  Class GraphicsLayerComboBox
 ******************************************************************************/

/**
 * @brief The GraphicsLayerComboBox class
 *
 * @author ubruhin
 * @date 2017-08-15
 */
class GraphicsLayerComboBox final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit GraphicsLayerComboBox(QWidget* parent = nullptr) noexcept;
  GraphicsLayerComboBox(const GraphicsLayerComboBox& other) = delete;
  ~GraphicsLayerComboBox() noexcept;

  // Getters
  QString getCurrentLayerName() const noexcept;

  // Setters
  void setLayers(const QList<GraphicsLayer*>& layers) noexcept;
  void setCurrentLayer(const QString& name) noexcept;

  // Operator Overloadings
  GraphicsLayerComboBox& operator=(const GraphicsLayerComboBox& rhs) = delete;

signals:
  void currentLayerChanged(const QString& name);

private:  // Methods
  void currentIndexChanged(int index) noexcept;

private:  // Data
  QComboBox* mComboBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_GRAPHICSLAYERCOMBOBOX_H
