/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef PROJECT_ADDGENCOMPDIALOG_H
#define PROJECT_ADDGENCOMPDIALOG_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/fileio/filepath.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class GraphicsScene;
class Workspace;

namespace project {
class Project;
}

namespace library {
class Component;
class ComponentSymbolVariant;
class Symbol;
class SymbolPreviewGraphicsItem;
class CategoryTreeModel;
class ComponentCategory;
}

namespace Ui {
class AddGenCompDialog;
}

namespace project {

/*****************************************************************************************
 *  Class AddGenCompDialog
 ****************************************************************************************/

/**
 * @brief The AddGenCompDialog class
 *
 * @todo This class is VERY provisional!
 *
 * @author ubruhin
 * @date 2015-02-16
 */
class AddGenCompDialog final : public QDialog
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit AddGenCompDialog(Workspace& workspace, Project& project, QWidget* parent = nullptr);
        ~AddGenCompDialog() noexcept;

        // Getters
        FilePath getSelectedGenCompFilePath() const noexcept;
        QUuid getSelectedSymbVarUuid() const noexcept;


    private slots:

        void treeCategories_currentItemChanged(const QModelIndex& current, const QModelIndex& previous);
        void on_listGenericComponents_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
        void on_cbxSymbVar_currentIndexChanged(int index);


    private:

        // Private Methods
        void setSelectedCategory(const QUuid& categoryUuid);
        void setSelectedGenComp(const library::Component* genComp);
        void setSelectedSymbVar(const library::ComponentSymbolVariant* symbVar);
        void accept() noexcept;


        // General
        Workspace& mWorkspace;
        Project& mProject;
        Ui::AddGenCompDialog* mUi;
        GraphicsScene* mPreviewScene;
        library::CategoryTreeModel* mCategoryTreeModel;


        // Attributes
        QUuid mSelectedCategoryUuid;
        const library::Component* mSelectedGenComp;
        const library::ComponentSymbolVariant* mSelectedSymbVar;
        QList<library::SymbolPreviewGraphicsItem*> mPreviewSymbolGraphicsItems;
};

} // namespace project

#endif // PROJECT_ADDGENCOMPDIALOG_H
