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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "categorytreelabeltextbuilder.h"
#include <librepcb/library/cat/componentcategory.h>
#include <librepcb/library/cat/packagecategory.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

template <typename ElementType>
CategoryTreeLabelTextBuilder<ElementType>::CategoryTreeLabelTextBuilder(
        const workspace::WorkspaceLibraryDb& db, const QStringList& localeOrder, QLabel& label) noexcept :
    mDb(db), mLocaleOrder(localeOrder), mLabel(label), mHighlightLastLine(false),
    mEndlessRecursionUuid(), mOneLine(false)
{
}

template <typename ElementType>
CategoryTreeLabelTextBuilder<ElementType>::~CategoryTreeLabelTextBuilder() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

template <typename ElementType>
void CategoryTreeLabelTextBuilder<ElementType>::setText(const QString& text) noexcept
{
    mLabel.setText(text);
    mLabel.setStyleSheet(QString());
}

template <typename ElementType>
void CategoryTreeLabelTextBuilder<ElementType>::setErrorText(const QString& error) noexcept
{
    mLabel.setText(error);
    mLabel.setStyleSheet("QLabel { color: red; }");
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

template <typename ElementType>
bool CategoryTreeLabelTextBuilder<ElementType>::updateText(const tl::optional<Uuid>& category,
                                                           const QString& lastLine) noexcept
{
    try {
        QList<Uuid> uuids;
        if (category) {
            uuids.append(*category);
            uuids.append(getCategoryParents(*category)); // can throw
            if (mEndlessRecursionUuid && uuids.contains(*mEndlessRecursionUuid)) {
                throw RuntimeError(__FILE__, __LINE__, tr("Endless recursion detected!"));
            }
        }
        return updateText(uuids, lastLine);
    } catch (const Exception& e) {
        setErrorText(e.getMsg());
        return false;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

template <typename ElementType>
bool CategoryTreeLabelTextBuilder<ElementType>::updateText(const  QList<Uuid>& uuids, const QString& lastLine) noexcept
{
    try {
        QStringList lines;
        foreach (const Uuid& uuid, uuids) {
            FilePath filepath = getLatestCategory(uuid); // can throw
            QString name;
            mDb.getElementTranslations<ElementType>(filepath, mLocaleOrder, &name); // can throw
            lines.prepend(name);
        }
        lines.prepend(tr("Root category"));
        if (!lastLine.isNull()) {
            lines.append(lastLine);
        }
        setText(lines);
        return true;
    } catch (const Exception& e) {
        setErrorText(e.getMsg());
        return false;
    }
}

template <typename ElementType>
void CategoryTreeLabelTextBuilder<ElementType>::setText(const QStringList& lines) noexcept
{
    QString text;
    for (int i = 0; i < lines.count(); ++i) {
        QString line = lines.value(i);
        QString spaces =  QString("&nbsp;").repeated(i * 2);
        QString separator = mOneLine ? QString(" &rArr; ") : QString("<br>%1⤷ ").arg(spaces);
        if (i == 0) {
            text.append(line);
        } else if ((i == lines.count() - 1) && mHighlightLastLine) {
            text.append(QString("%1<b>%2</b>").arg(separator, line));
        } else {
            text.append(QString("%1%2").arg(separator, line));
        }
    }
    setText(text);
}

template <>
FilePath CategoryTreeLabelTextBuilder<ComponentCategory>::getLatestCategory(const Uuid& category) const
{
    return mDb.getLatestComponentCategory(category);
}

template <>
FilePath CategoryTreeLabelTextBuilder<PackageCategory>::getLatestCategory(const Uuid& category) const
{
    return mDb.getLatestPackageCategory(category);
}

template <>
QList<Uuid> CategoryTreeLabelTextBuilder<ComponentCategory>::getCategoryParents(const Uuid& category) const
{
    return mDb.getComponentCategoryParents(category);
}

template <>
QList<Uuid> CategoryTreeLabelTextBuilder<PackageCategory>::getCategoryParents(const Uuid& category) const
{
    return mDb.getComponentCategoryParents(category);
}

/*****************************************************************************************
 *  Explicit template instantiations
 ****************************************************************************************/
template class CategoryTreeLabelTextBuilder<library::ComponentCategory>;
template class CategoryTreeLabelTextBuilder<library::PackageCategory>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

