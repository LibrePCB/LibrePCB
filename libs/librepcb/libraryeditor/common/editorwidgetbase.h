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

#ifndef LIBREPCB_LIBRARY_EDITOR_EDITORWIDGETBASE_H
#define LIBREPCB_LIBRARY_EDITOR_EDITORWIDGETBASE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/units/all_length_units.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class ExclusiveActionGroup;
class UndoStackActionGroup;
class ToolBarProxy;
class IF_GraphicsLayerProvider;

namespace workspace {
class Workspace;
}

namespace library {

class LibraryBaseElement;

namespace editor {

/*****************************************************************************************
 *  Class EditorWidgetBase
 ****************************************************************************************/

/**
 * @brief The EditorWidgetBase class
 *
 * @author ubruhin
 * @date 2016-10-17
 */
class EditorWidgetBase : public QWidget
{
        Q_OBJECT

    public:

        // Types

        struct Context {
            workspace::Workspace& workspace;
            const IF_GraphicsLayerProvider& layerProvider;
            bool elementIsNewlyCreated;
        };

        enum Tool {
            NONE,
            SELECT,
            DRAW_LINE,
            DRAW_RECT,
            DRAW_POLYGON,
            DRAW_ELLIPSE,
            DRAW_TEXT,
            ADD_NAMES,
            ADD_VALUES,
            ADD_PINS,
            ADD_THT_PADS,
            ADD_SMT_PADS,
            ADD_HOLES,
        };

        // Constructors / Destructor
        EditorWidgetBase() = delete;
        EditorWidgetBase(const EditorWidgetBase& other) = delete;
        explicit EditorWidgetBase(const Context& context, const FilePath& fp,
                                  QWidget* parent = nullptr);
        virtual ~EditorWidgetBase() noexcept;

        // Getters
        const FilePath& getFilePath() const noexcept {return mFilePath;}
        bool isDirty() const noexcept;
        virtual bool hasGraphicalEditor() const noexcept {return false;}

        // Setters
        virtual void setUndoStackActionGroup(UndoStackActionGroup* group) noexcept;
        virtual void setToolsActionGroup(ExclusiveActionGroup* group) noexcept;
        virtual void setCommandToolBar(QToolBar* toolbar) noexcept;

        // Operator Overloadings
        EditorWidgetBase& operator=(const EditorWidgetBase& rhs) = delete;


    public slots:
        virtual bool save() noexcept;
        virtual bool rotateCw() noexcept {return false;}
        virtual bool rotateCcw() noexcept {return false;}
        virtual bool remove() noexcept {return false;}
        virtual bool zoomIn() noexcept {return false;}
        virtual bool zoomOut() noexcept {return false;}
        virtual bool zoomAll() noexcept {return false;}
        virtual bool abortCommand() noexcept {return false;}
        virtual bool editGridProperties() noexcept {return false;}


    protected: // Methods
        void setupInterfaceBrokenWarningWidget(QWidget& widget) noexcept;
        virtual bool isInterfaceBroken() const noexcept = 0;
        virtual bool toolChangeRequested(Tool newTool) noexcept {Q_UNUSED(newTool); return false;}
        void setDirty() noexcept;
        void undoStackStateModified() noexcept;
        const QStringList& getLibLocaleOrder() const noexcept;


    private: // Methods
        void toolActionGroupChangeTriggered(const QVariant& newTool) noexcept;
        void undoStackCleanChanged(bool clean) noexcept;


    signals:
        void dirtyChanged(bool dirty);
        void elementEdited(const FilePath& fp);
        void interfaceBrokenChanged(bool broken);
        void cursorPositionChanged(const Point& pos);


    protected: // Data
        Context mContext;
        FilePath mFilePath;
        QScopedPointer<UndoStack> mUndoStack;
        UndoStackActionGroup* mUndoStackActionGroup;
        ExclusiveActionGroup* mToolsActionGroup;
        QScopedPointer<ToolBarProxy> mCommandToolBarProxy;
        bool mIsDirty;
        bool mIsInterfaceBroken;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_EDITORWIDGETBASE_H
