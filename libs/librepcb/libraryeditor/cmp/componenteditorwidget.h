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

#ifndef LIBREPCB_LIBRARY_EDITOR_COMPONENTEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_COMPONENTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../common/categorylisteditorwidget.h"
#include "../common/editorwidgetbase.h"
#include "if_componentsymbolvarianteditorprovider.h"

#include <librepcb/library/cmp/componentsymbolvariant.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

class Component;

namespace editor {

class ComponentSymbolVariantEditDialog;

namespace Ui {
class ComponentEditorWidget;
}

/*******************************************************************************
 *  Class ComponentEditorWidget
 ******************************************************************************/

/**
 * @brief The ComponentEditorWidget class
 */
class ComponentEditorWidget final
  : public EditorWidgetBase,
    public IF_ComponentSymbolVariantEditorProvider {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentEditorWidget()                                   = delete;
  ComponentEditorWidget(const ComponentEditorWidget& other) = delete;
  ComponentEditorWidget(const Context& context, const FilePath& fp,
                        QWidget* parent = nullptr);
  ~ComponentEditorWidget() noexcept;

  // Operator Overloadings
  ComponentEditorWidget& operator=(const ComponentEditorWidget& rhs) = delete;

public slots:
  bool save() noexcept override;

private:  // Methods
  void    updateMetadata() noexcept;
  QString commitMetadata() noexcept;
  bool    openComponentSymbolVariantEditor(
         ComponentSymbolVariant& variant) noexcept override;
  void memorizeComponentInterface() noexcept;
  bool isInterfaceBroken() const noexcept override;
  bool runChecks(LibraryElementCheckMessageList& msgs) const override;
  template <typename MessageType>
  void fixMsg(const MessageType& msg);
  template <typename MessageType>
  bool fixMsgHelper(std::shared_ptr<const LibraryElementCheckMessage> msg,
                    bool                                              applyFix);
  bool processCheckMessage(
      std::shared_ptr<const LibraryElementCheckMessage> msg,
      bool                                              applyFix) override;

private:  // Data
  QScopedPointer<Ui::ComponentEditorWidget>         mUi;
  QScopedPointer<ComponentCategoryListEditorWidget> mCategoriesEditorWidget;
  QSharedPointer<Component>                         mComponent;

  // broken interface detection
  bool                       mOriginalIsSchematicOnly;
  QSet<Uuid>                 mOriginalSignalUuids;
  ComponentSymbolVariantList mOriginalSymbolVariants;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_COMPONENTEDITORWIDGET_H
