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

#ifndef LIBREPCB_EDITOR_GRAPHICSLAYER_H
#define LIBREPCB_EDITOR_GRAPHICSLAYER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/utils/signalslot.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

namespace editor {

/*******************************************************************************
 *  Class GraphicsLayer
 ******************************************************************************/

/**
 * @brief The GraphicsLayer class represents a graphical layer used in
 * schematics and boards
 *
 * These layers are used in graphics items (QGraphicsItem) to determine their
 * visibility and colors.
 */
class GraphicsLayer {
  Q_DECLARE_TR_FUNCTIONS(GraphicsLayer)

public:
  // Signals
  enum class Event {
    ColorChanged,
    HighlightColorChanged,
    VisibleChanged,
    EnabledChanged,
  };
  Signal<GraphicsLayer, Event> onEdited;
  typedef Slot<GraphicsLayer, Event> OnEditedSlot;

  // Constructors / Destructor
  GraphicsLayer() = delete;
  GraphicsLayer(const GraphicsLayer& other) noexcept;
  explicit GraphicsLayer(const QString& name, const QString& nameTr,
                         const QColor& color, const QColor& colorHighlighted,
                         bool visible = true, bool enabled = true) noexcept;
  virtual ~GraphicsLayer() noexcept;

  // Getters
  const QString& getName() const noexcept { return mName; }
  const QString& getNameTr() const noexcept { return mNameTr; }
  const QColor& getColor(bool highlighted = false) const noexcept {
    return highlighted ? mColorHighlighted : mColor;
  }
  bool getVisible() const noexcept { return mIsVisible; }
  bool isEnabled() const noexcept { return mIsEnabled; }
  bool isVisible() const noexcept { return mIsEnabled && mIsVisible; }

  // Setters
  void setColor(const QColor& color) noexcept;
  void setColorHighlighted(const QColor& color) noexcept;
  void setVisible(bool visible) noexcept;
  void setEnabled(bool enable) noexcept;

  // Operator Overloadings
  GraphicsLayer& operator=(const GraphicsLayer& rhs) = delete;

protected:  // Data
  const QString mName;  ///< Theme color name
  const QString mNameTr;  ///< Translated layer name as shown in the GUI
  QColor mColor;  ///< Color of graphics items
  QColor mColorHighlighted;  ///< Color of highlighted graphics items
  bool mIsVisible;  ///< Visibility of graphics items on that layer
  bool mIsEnabled;  ///< Availability of the layer itself
};

/*******************************************************************************
 *  Interface IF_GraphicsLayerProvider
 ******************************************************************************/

/**
 * @brief The IF_GraphicsLayerProvider class defines an interface for classes
 * which provide layers
 */
class IF_GraphicsLayerProvider {
public:
  virtual ~IF_GraphicsLayerProvider() noexcept {}
  virtual QList<std::shared_ptr<GraphicsLayer>> getAllLayers() const
      noexcept = 0;
  virtual std::shared_ptr<GraphicsLayer> getLayer(const QString& name) const
      noexcept = 0;
  std::shared_ptr<GraphicsLayer> getLayer(const Layer& layer) const noexcept;
  std::shared_ptr<GraphicsLayer> getGrabAreaLayer(
      const Layer& outlineLayer) const noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
