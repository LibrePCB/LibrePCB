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

#ifndef LIBREPCB_EDITOR_WINDOWTABSMODELADAPTER_H
#define LIBREPCB_EDITOR_WINDOWTABSMODELADAPTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"
#include "windowtabsmodel.h"

#include <QtCore>
#include <QtGui>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Class WindowTabsModelAdapter
 ******************************************************************************/

/**
 * @brief The WindowTabsModelAdapter class
 */
template <typename TTab, typename TModelData>
class WindowTabsModelAdapter : public QObject, public slint::Model<TModelData> {
public:
  // Constructors / Destructor
  WindowTabsModelAdapter() = delete;
  WindowTabsModelAdapter(const WindowTabsModelAdapter& other) = delete;
  explicit WindowTabsModelAdapter(std::shared_ptr<WindowTabsModel> tabs,
                                  QObject* parent = nullptr) noexcept
    : QObject(parent), mModel(tabs) {
    connect(mModel.get(), &WindowTabsModel::uiDataChanged, this,
            [this](std::size_t index) {
              slint::Model<TModelData>::row_changed(index);
            });
  }
  virtual ~WindowTabsModelAdapter() noexcept {}

  // Implementations
  std::size_t row_count() const override { return mModel->row_count(); }
  std::optional<TModelData> row_data(std::size_t i) const override {
    if (auto t = std::dynamic_pointer_cast<TTab>(mModel->getTab(i))) {
      return t->getUiData();
    }
    return std::nullopt;
  }
  void set_row_data(size_t i, const TModelData& data) override {
    if (auto t = std::dynamic_pointer_cast<TTab>(mModel->getTab(i))) {
      t->setUiData(data);
    }
  }

  // Operator Overloadings
  WindowTabsModelAdapter& operator=(const WindowTabsModelAdapter& rhs) = delete;

private:
  std::shared_ptr<WindowTabsModel> mModel;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
