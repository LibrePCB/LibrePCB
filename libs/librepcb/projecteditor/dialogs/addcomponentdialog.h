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

#ifndef LIBREPCB_PROJECT_ADDCOMPONENTDIALOG_H
#define LIBREPCB_PROJECT_ADDCOMPONENTDIALOG_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/uuid.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/workspace/library/cat/categorytreemodel.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GraphicsScene;

namespace library {
class Component;
class ComponentSymbolVariant;
class Symbol;
class SymbolPreviewGraphicsItem;
class ComponentCategory;
}

namespace workspace {
class Workspace;
}

namespace project {

class Project;

namespace editor {

namespace Ui {
class AddComponentDialog;
}

/*****************************************************************************************
 *  Class AddComponentDialog
 ****************************************************************************************/

/**
 * @brief The AddComponentDialog class
 *
 * @todo This class is VERY provisional!
 *
 * @author ubruhin
 * @date 2015-02-16
 */
class AddComponentDialog final : public QDialog
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit AddComponentDialog(workspace::Workspace& workspace, Project& project,
                                    QWidget* parent = nullptr);
        ~AddComponentDialog() noexcept;

        // Getters
        Uuid getSelectedComponentUuid() const noexcept;
        Uuid getSelectedSymbVarUuid() const noexcept;


    private slots:

        void treeCategories_currentItemChanged(const QModelIndex& current, const QModelIndex& previous);
        void on_listComponents_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
        void on_cbxSymbVar_currentIndexChanged(int index);


    private:

        // Private Methods
        void setSelectedCategory(const Uuid& categoryUuid);
        void setSelectedComponent(const library::Component* cmp);
        void setSelectedSymbVar(const library::ComponentSymbolVariant* symbVar);
        void accept() noexcept;


        // General
        workspace::Workspace& mWorkspace;
        Project& mProject;
        Ui::AddComponentDialog* mUi;
        GraphicsScene* mPreviewScene;
        workspace::ComponentCategoryTreeModel* mCategoryTreeModel;


        // Attributes
        Uuid mSelectedCategoryUuid;
        const library::Component* mSelectedComponent;
        const library::ComponentSymbolVariant* mSelectedSymbVar;
        QList<library::SymbolPreviewGraphicsItem*> mPreviewSymbolGraphicsItems;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_ADDCOMPONENTDIALOG_H
