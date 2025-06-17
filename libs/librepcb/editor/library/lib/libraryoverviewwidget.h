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

#ifndef LIBREPCB_EDITOR_LIBRARYOVERVIEWWIDGET_H
#define LIBREPCB_EDITOR_LIBRARYOVERVIEWWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../editorwidgetbase.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Library;

namespace editor {

class LibraryListEditorWidget;

namespace Ui {
class LibraryOverviewWidget;
}

/*******************************************************************************
 *  Class LibraryOverviewWidget
 ******************************************************************************/

/**
 * @brief The LibraryOverviewWidget class
 */
class LibraryOverviewWidget final : public EditorWidgetBase {
  Q_OBJECT

  struct LibraryMenuItem {
    QString name;
    QPixmap pixmap;
    FilePath filepath;
  };

public:
  // Constructors / Destructor
  LibraryOverviewWidget() = delete;
  LibraryOverviewWidget(const LibraryOverviewWidget& other) = delete;
  LibraryOverviewWidget(const Context& context, const FilePath& fp,
                        QWidget* parent = nullptr);
  ~LibraryOverviewWidget() noexcept;

  // Getters
  Library& getLibrary() const noexcept { return *mLibrary; }
  QSet<Feature> getAvailableFeatures() const noexcept override;

  // Operator Overloadings
  LibraryOverviewWidget& operator=(const LibraryOverviewWidget& rhs) = delete;

public slots:
  bool save() noexcept override;
  bool remove() noexcept override;

signals:
  void newComponentCategoryTriggered();
  void newPackageCategoryTriggered();
  void newSymbolTriggered();
  void newPackageTriggered();
  void newComponentTriggered();
  void newDeviceTriggered();
  void editSymbolTriggered(const FilePath& fp);
  void editPackageTriggered(const FilePath& fp);
  void editComponentTriggered(const FilePath& fp);
  void editDeviceTriggered(const FilePath& fp);
  void duplicateComponentCategoryTriggered(const FilePath& fp);
  void duplicatePackageCategoryTriggered(const FilePath& fp);
  void duplicateSymbolTriggered(const FilePath& fp);
  void duplicatePackageTriggered(const FilePath& fp);
  void duplicateComponentTriggered(const FilePath& fp);
  void duplicateDeviceTriggered(const FilePath& fp);
  void removeElementTriggered(const FilePath& fp);

private:  // Methods
  bool isInterfaceBroken() const noexcept override { return false; }
  bool runChecks(RuleCheckMessageList& msgs) const override;
  bool processRuleCheckMessage(std::shared_ptr<const RuleCheckMessage> msg,
                               bool applyFix) override;
  void ruleCheckApproveRequested(std::shared_ptr<const RuleCheckMessage> msg,
                                 bool approve) noexcept override;
  void removeItems(
      const QHash<QListWidgetItem*, FilePath>& selectedItemPaths) noexcept;
  void copyElementsToOtherLibrary(
      const QHash<QListWidgetItem*, FilePath>& selectedItemPaths,
      const FilePath& libFp, const QString& libName) noexcept;
  QList<LibraryMenuItem> getLocalLibraries() const noexcept;

private:  // Data
  QScopedPointer<Ui::LibraryOverviewWidget> mUi;
  std::unique_ptr<Library> mLibrary;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
