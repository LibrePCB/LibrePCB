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

#ifndef LIBREPCB_LIBRARY_EDITOR_CATEGORYLISTEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_CATEGORYLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/cat/componentcategory.h>
#include <librepcb/library/cat/packagecategory.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace workspace {
class Workspace;
}

namespace library {
namespace editor {

namespace Ui {
class CategoryListEditorWidget;
}

/*******************************************************************************
 *  Class CategoryListEditorWidgetBase
 ******************************************************************************/

/**
 * @brief The CategoryListEditorWidgetBase class
 *
 * @author  ubruhin
 * @date    2016-10-29
 */
class CategoryListEditorWidgetBase : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  CategoryListEditorWidgetBase() = delete;
  explicit CategoryListEditorWidgetBase(const workspace::Workspace& ws,
                                        QWidget* parent = nullptr) noexcept;
  CategoryListEditorWidgetBase(const CategoryListEditorWidgetBase& other) =
      delete;
  virtual ~CategoryListEditorWidgetBase() noexcept;

  // Getters
  const QSet<Uuid>& getUuids() const noexcept { return mUuids; }

  // Setters
  void setRequiresMinimumOneEntry(bool v) noexcept;
  void setUuids(const QSet<Uuid>& uuids) noexcept;

  // Operator Overloadings
  CategoryListEditorWidgetBase& operator       =(
      const CategoryListEditorWidgetBase& rhs) = delete;

protected:
  virtual tl::optional<Uuid> chooseCategoryWithDialog() noexcept            = 0;
  virtual FilePath           getLatestCategory(const Uuid& category) const  = 0;
  virtual QList<Uuid>        getCategoryParents(const Uuid& category) const = 0;
  virtual QString            getCategoryName(const FilePath& fp) const      = 0;

private:
  void btnAddClicked() noexcept;
  void btnRemoveClicked() noexcept;
  void addItem(const tl::optional<Uuid>& category) noexcept;
  void addItem(const tl::optional<Uuid>& category,
               const QStringList&        lines) noexcept;
  void addItem(const tl::optional<Uuid>& category,
               const QString&            text) noexcept;
  void updateColor() noexcept;

signals:
  void edited();
  void categoryAdded(const Uuid& category);
  void categoryRemoved(const Uuid& category);

protected:  // Data
  const workspace::Workspace&                  mWorkspace;
  QScopedPointer<Ui::CategoryListEditorWidget> mUi;
  bool                                         mRequiresMinimumOneEntry;
  QSet<Uuid>                                   mUuids;
};

/*******************************************************************************
 *  Class CategoryListEditorWidget
 ******************************************************************************/

/**
 * @brief The CategoryListEditorWidget class
 *
 * @author  ubruhin
 * @date    2016-10-29
 */
template <typename ElementType>
class CategoryListEditorWidget final : public CategoryListEditorWidgetBase {
public:
  // Constructors / Destructor
  CategoryListEditorWidget() = delete;
  explicit CategoryListEditorWidget(const workspace::Workspace& ws,
                                    QWidget* parent = nullptr) noexcept;
  CategoryListEditorWidget(const CategoryListEditorWidget& other) = delete;
  ~CategoryListEditorWidget() noexcept;

  // Operator Overloadings
  CategoryListEditorWidget& operator=(const CategoryListEditorWidget& rhs) =
      delete;

private:
  tl::optional<Uuid> chooseCategoryWithDialog() noexcept override;
  FilePath           getLatestCategory(const Uuid& category) const override;
  QList<Uuid>        getCategoryParents(const Uuid& category) const override;
  QString            getCategoryName(const FilePath& fp) const override;
};

typedef CategoryListEditorWidget<library::ComponentCategory>
    ComponentCategoryListEditorWidget;
typedef CategoryListEditorWidget<library::PackageCategory>
    PackageCategoryListEditorWidget;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_CATEGORYLISTEDITORWIDGET_H
