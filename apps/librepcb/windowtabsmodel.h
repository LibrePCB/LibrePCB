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

#ifndef LIBREPCB_WINDOWTABSMODEL_H
#define LIBREPCB_WINDOWTABSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>
#include <QtGui>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

class GuiApplication;
class ProjectEditor;
class WindowTab;

/*******************************************************************************
 *  Class WindowTabsModel
 ******************************************************************************/

/**
 * @brief The WindowTabsModel class
 */
class WindowTabsModel : public QObject, public slint::Model<ui::Tab> {
  Q_OBJECT

public:
  // Constructors / Destructor
  WindowTabsModel() = delete;
  WindowTabsModel(const WindowTabsModel& other) = delete;
  explicit WindowTabsModel(GuiApplication& app,
                           int sectionId,
                           QObject* parent = nullptr) noexcept;
  virtual ~WindowTabsModel() noexcept;

  // General Methods
  std::shared_ptr<WindowTab> getTabById(int id) noexcept;
  std::shared_ptr<WindowTab> getTabByIndex(int idx) noexcept{return mItems.value(idx);}
  void addTab(ui::TabType type, std::shared_ptr<ProjectEditor> prj,
              int objIndex) noexcept;
  void closeTab(int index) noexcept;
  void setCurrentTab(int index) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::Tab> row_data(std::size_t i) const override;

  // Operator Overloadings
  WindowTabsModel& operator=(const WindowTabsModel& rhs) = delete;

signals:
  void cursorCoordinatesChanged(qreal x, qreal y);
  void requestRepaint();

private:
  void updateIndex() noexcept;

  GuiApplication& mApp;
  const int mSectionId;
  QList<std::shared_ptr<WindowTab>> mItems;
  QHash<int, int> mIndex;  ///< Map from tab ID to #mItems index
  int mNextId;  ///< Next free tab ID
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
