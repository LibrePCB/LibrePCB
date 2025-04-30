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

#ifndef LIBREPCB_EDITOR_GRAPHICSLAYERLIST_H
#define LIBREPCB_EDITOR_GRAPHICSLAYERLIST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;
class Theme;
class WorkspaceSettings;

namespace editor {

class GraphicsLayer;

/*******************************************************************************
 *  Class GraphicsLayerList
 ******************************************************************************/

/**
 * @brief The GraphicsLayerList class
 */
class GraphicsLayerList final : public QObject {
public:
  // Constructors / Destructor
  GraphicsLayerList() = delete;
  ~GraphicsLayerList() noexcept;

  // Getters
  std::shared_ptr<GraphicsLayer> get(const QString& name) noexcept;
  std::shared_ptr<const GraphicsLayer> get(const QString& name) const noexcept;
  std::shared_ptr<GraphicsLayer> get(const Layer& layer) noexcept;
  std::shared_ptr<const GraphicsLayer> get(const Layer& layer) const noexcept;
  std::shared_ptr<const GraphicsLayer> grabArea(
      const Layer& outlineLayer) const noexcept;
  const QList<std::shared_ptr<GraphicsLayer>>& all() const noexcept {
    return mLayers;
  }

  // General Methods
  void showTop() noexcept;
  void showBottom() noexcept;
  void showTopAndBottom() noexcept;
  void showAll() noexcept;
  void showNone() noexcept;

  // Static Methods
  static std::unique_ptr<GraphicsLayerList> previewLayers(
      const WorkspaceSettings* ws) noexcept;
  static std::unique_ptr<GraphicsLayerList> libraryLayers(
      const WorkspaceSettings* ws) noexcept;
  static std::unique_ptr<GraphicsLayerList> schematicLayers(
      const WorkspaceSettings* ws) noexcept;
  static std::unique_ptr<GraphicsLayerList> boardLayers(
      const WorkspaceSettings* ws) noexcept;

private:
  GraphicsLayerList(const WorkspaceSettings* ws) noexcept;
  void add(const Theme& theme, const QString& name,
           bool visible = true) noexcept;
  void reloadSettings() noexcept;
  void setVisibleLayers(const QSet<QString>& layers) noexcept;
  static QSet<QString> getCommonLayers() noexcept;
  static QSet<QString> getTopLayers() noexcept;
  static QSet<QString> getBottomLayers() noexcept;
  static QSet<QString> getAllLayers() noexcept;

  QPointer<const WorkspaceSettings> mSettings;
  QList<std::shared_ptr<GraphicsLayer>> mLayers;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
