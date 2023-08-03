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

#ifndef LIBREPCB_EDITOR_GRAPHICSOUTPUTJOBWIDGET_H
#define LIBREPCB_EDITOR_GRAPHICSOUTPUTJOBWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/job/graphicsoutputjob.h>
#include <optional/tl/optional.hpp>

#include <QtCore>
#include <QtWidgets>

#include <functional>
#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsExport;
class GraphicsOutputJob;
class LengthUnit;
class OutputJobRunner;
class Project;

namespace editor {

namespace Ui {
class GraphicsOutputJobWidget;
}

/*******************************************************************************
 *  Class GraphicsOutputJobWidget
 ******************************************************************************/

/**
 * @brief The GraphicsOutputJobWidget class
 */
class GraphicsOutputJobWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  GraphicsOutputJobWidget() = delete;
  GraphicsOutputJobWidget(const GraphicsOutputJobWidget& other) = delete;
  explicit GraphicsOutputJobWidget(Project& project,
                                   std::shared_ptr<GraphicsOutputJob> job,
                                   const LengthUnit& lengthUnit,
                                   const QString& settingsPrefix,
                                   QWidget* parent = nullptr) noexcept;
  ~GraphicsOutputJobWidget() noexcept;

  // Operator Overloads
  GraphicsOutputJobWidget& operator=(const GraphicsOutputJobWidget& rhs) =
      delete;

private:  // Methods
  void addClicked() noexcept;
  void copyClicked() noexcept;
  void removeClicked() noexcept;
  void currentContentChanged(int index) noexcept;
  void updateContentList() noexcept;
  void layerListItemDoubleClicked(QListWidgetItem* item) noexcept;
  void modify(std::function<void(GraphicsOutputJob::Content&)> fun) noexcept;
  void updatePreview() noexcept;

private:  // Data
  Project& mProject;
  std::shared_ptr<GraphicsOutputJob> mJob;
  QScopedPointer<OutputJobRunner> mPreviewRunner;
  QScopedPointer<GraphicsExport> mPreviewGraphicsExport;
  QList<tl::optional<QPageSize>> mPageSizes;
  bool mDisableModification;
  QScopedPointer<Ui::GraphicsOutputJobWidget> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
