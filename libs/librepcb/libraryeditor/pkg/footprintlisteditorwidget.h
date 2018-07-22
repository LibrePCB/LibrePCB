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

#ifndef LIBREPCB_LIBRARY_EDITOR_FOOTPRINTLISTEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_FOOTPRINTLISTEDITORWIDGET_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/library/pkg/footprint.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class UndoStack;

namespace library {
namespace editor {

/*****************************************************************************************
 *  Class FootprintListEditorWidget
 ****************************************************************************************/

/**
 * @brief The FootprintListEditorWidget class
 *
 * @author ubruhin
 * @date 2017-05-27
 */
class FootprintListEditorWidget final : public QWidget, private FootprintList::IF_Observer
{
        Q_OBJECT

    private: // Types
        enum Column {
            COLUMN_NAME = 0,
            COLUMN_BUTTONS,
            _COLUMN_COUNT
        };


    public:
        // Constructors / Destructor
        explicit FootprintListEditorWidget(QWidget* parent = nullptr) noexcept;
        FootprintListEditorWidget(const FootprintListEditorWidget& other) = delete;
        ~FootprintListEditorWidget() noexcept;

        // Setters
        void setReferences(FootprintList& list, UndoStack& stack) noexcept;

        // Operator Overloadings
        FootprintListEditorWidget& operator=(const FootprintListEditorWidget& rhs) = delete;


    signals:
        void currentFootprintChanged(int index);


    private: // Slots
        void currentCellChanged(int currentRow, int currentColumn,
                                int previousRow, int previousColumn) noexcept;
        void tableCellChanged(int row, int column) noexcept;
        void btnUpClicked() noexcept;
        void btnDownClicked() noexcept;
        void btnCopyClicked() noexcept;
        void btnAddRemoveClicked() noexcept;


    private: // Methods
        void updateTable(tl::optional<Uuid> selected = tl::nullopt) noexcept;
        void setTableRowContent(int row, const tl::optional<Uuid>& uuid, const QString& name) noexcept;
        void addFootprint(const QString& name) noexcept;
        void removeFootprint(const Uuid& uuid) noexcept;
        void moveFootprintUp(int index) noexcept;
        void moveFootprintDown(int index) noexcept;
        void copyFootprint(const Uuid& uuid) noexcept;
        QString setName(const Uuid& uuid, const QString& name) noexcept;
        int getRowOfTableCellWidget(QObject* obj) const noexcept;
        tl::optional<Uuid> getUuidOfRow(int row) const noexcept;
        void throwIfNameEmptyOrExists(const QString& name) const;
        static QString cleanName(const QString& name) noexcept;

        // row index <-> signal index conversion methods
        int newFootprintRow() const noexcept {return mFootprintList->count();}
        int indexToRow(int index) const noexcept {return index;}
        int rowToIndex(int row) const noexcept {return row;}
        bool isExistingFootprintRow(int row) const noexcept {return row >= 0 && row < mFootprintList->count();}
        bool isNewFootprintRow(int row) const noexcept {return row == newFootprintRow();}

        // Observer Methods
        void listObjectAdded(const FootprintList& list,
                             int newIndex, const std::shared_ptr<Footprint>& ptr) noexcept override;
        void listObjectRemoved(const FootprintList& list,
                               int oldIndex, const std::shared_ptr<Footprint>& ptr) noexcept override;


    private: // Data
        QTableWidget* mTable;
        FootprintList* mFootprintList;
        UndoStack* mUndoStack;
        tl::optional<Uuid> mSelectedFootprint;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_FOOTPRINTLISTEDITORWIDGET_H
