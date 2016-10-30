/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_LIBRARY_EDITOR_COMPONENTCATEGORYEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_COMPONENTCATEGORYEDITORWIDGET_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>
#include "../common/editorwidgetbase.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

class ComponentCategory;

namespace editor {

namespace Ui {
class ComponentCategoryEditorWidget;
}

/*****************************************************************************************
 *  Class ComponentCategoryEditorWidget
 ****************************************************************************************/

/**
 * @brief The ComponentCategoryEditorWidget class
 *
 * @author ubruhin
 * @date 2016-10-16
 */
class ComponentCategoryEditorWidget final : public EditorWidgetBase
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        ComponentCategoryEditorWidget() = delete;
        ComponentCategoryEditorWidget(const ComponentCategoryEditorWidget& other) = delete;
        ComponentCategoryEditorWidget(const Context& context, const FilePath& fp,
                                      QWidget* parent = nullptr);
        ~ComponentCategoryEditorWidget() noexcept;

        // Operator Overloadings
        ComponentCategoryEditorWidget& operator=(const ComponentCategoryEditorWidget& rhs) = delete;


    public slots:

        bool save() noexcept override;


    private: // Methods
        bool isInterfaceBroken() const noexcept override {return false;}
        void btnChooseParentCategoryClicked() noexcept;
        void edtnameTextChanged(const QString& text) noexcept;
        void edtParentTextChanged(const QString& text) noexcept;


    private: // Data

        QScopedPointer<Ui::ComponentCategoryEditorWidget> mUi;
        QScopedPointer<ComponentCategory> mCategory;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_COMPONENTCATEGORYEDITORWIDGET_H
