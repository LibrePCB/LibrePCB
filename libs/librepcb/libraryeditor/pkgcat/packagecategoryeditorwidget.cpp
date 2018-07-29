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
#include "packagecategoryeditorwidget.h"
#include "ui_packagecategoryeditorwidget.h"
#include <librepcb/library/cat/packagecategory.h>
#include <librepcb/workspace/workspace.h>
#include "../common/categorychooserdialog.h"
#include "../common/categorytreelabeltextbuilder.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PackageCategoryEditorWidget::PackageCategoryEditorWidget(const Context& context,
        const FilePath& fp, QWidget* parent) :
    EditorWidgetBase(context, fp, parent), mUi(new Ui::PackageCategoryEditorWidget)
{
    mUi->setupUi(this);
    setWindowIcon(QIcon(":/img/places/folder_green.png"));
    connect(mUi->btnChooseParentCategory, &QToolButton::clicked,
            this, &PackageCategoryEditorWidget::btnChooseParentCategoryClicked);
    connect(mUi->edtName, &QLineEdit::textChanged,
            this, &PackageCategoryEditorWidget::edtnameTextChanged);
    connect(mUi->edtParent, &QLineEdit::textChanged,
            this, &PackageCategoryEditorWidget::edtParentTextChanged);

    mCategory.reset(new PackageCategory(fp, false)); // can throw
    setWindowTitle(*mCategory->getNames().value(getLibLocaleOrder()));
    mUi->lblUuid->setText(QString("<a href=\"%1\">%2</a>").arg(
        mCategory->getFilePath().toQUrl().toString(), mCategory->getUuid().toStr()));
    mUi->lblUuid->setToolTip(mCategory->getFilePath().toNative());
    mUi->edtName->setText(*mCategory->getNames().value(getLibLocaleOrder()));
    mUi->edtDescription->setPlainText(mCategory->getDescriptions().value(getLibLocaleOrder()));
    mUi->edtKeywords->setText(mCategory->getKeywords().value(getLibLocaleOrder()));
    mUi->edtAuthor->setText(mCategory->getAuthor());
    mUi->edtVersion->setText(mCategory->getVersion().toStr());
    mUi->edtParent->setText(mCategory->getParentUuid() ? mCategory->getParentUuid()->toStr() : QString());
    mUi->cbxDeprecated->setChecked(mCategory->isDeprecated());

    connect(mUi->edtName, &QLineEdit::textChanged, this, &QWidget::setWindowTitle);
    connect(mUi->edtName, &QLineEdit::textEdited, this, &PackageCategoryEditorWidget::setDirty);
    connect(mUi->edtDescription, &QPlainTextEdit::textChanged, this, &PackageCategoryEditorWidget::setDirty);
    connect(mUi->edtKeywords, &QLineEdit::textEdited, this, &PackageCategoryEditorWidget::setDirty);
    connect(mUi->edtAuthor, &QLineEdit::textEdited, this, &PackageCategoryEditorWidget::setDirty);
    connect(mUi->edtVersion, &QLineEdit::textEdited, this, &PackageCategoryEditorWidget::setDirty);
    connect(mUi->cbxDeprecated, &QCheckBox::clicked, this, &PackageCategoryEditorWidget::setDirty);
    connect(mUi->edtParent, &QLineEdit::textChanged, this, &PackageCategoryEditorWidget::setDirty);
}

PackageCategoryEditorWidget::~PackageCategoryEditorWidget() noexcept
{
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

bool PackageCategoryEditorWidget::save() noexcept
{
    try {
        ElementName name(mUi->edtName->text().trimmed()); // can throw
        Version version = Version::fromString(mUi->edtVersion->text().trimmed()); // can throw
        QString parentUuidStr = mUi->edtParent->text().trimmed();
        tl::optional<Uuid> parentUuid = Uuid::tryFromString(parentUuidStr);
        if (!parentUuid && !parentUuidStr.isEmpty()) {
            throw RuntimeError(__FILE__, __LINE__, tr("The parent UUID is invalid."));
        }

        mCategory->setName("", name);
        mCategory->setDescription("", mUi->edtDescription->toPlainText().trimmed());
        mCategory->setKeywords("", mUi->edtKeywords->text().trimmed());
        mCategory->setAuthor(mUi->edtAuthor->text().trimmed());
        mCategory->setVersion(version);
        mCategory->setParentUuid(parentUuid);
        mCategory->setDeprecated(mUi->cbxDeprecated->isChecked());
        mCategory->save();
        return EditorWidgetBase::save();
    } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Save failed"), e.getMsg());
        return false;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void PackageCategoryEditorWidget::btnChooseParentCategoryClicked() noexcept
{
    PackageCategoryChooserDialog dialog(mContext.workspace);
    if (dialog.exec()) {
        tl::optional<Uuid> uuid = dialog.getSelectedCategoryUuid();
        mUi->edtParent->setText(uuid ? uuid->toStr() : QString());
    }
}

void PackageCategoryEditorWidget::edtnameTextChanged(const QString& text) noexcept
{
    // force updating parent categories
    Q_UNUSED(text);
    edtParentTextChanged(mUi->edtParent->text());
}

void PackageCategoryEditorWidget::edtParentTextChanged(const QString& text) noexcept
{
    const workspace::WorkspaceLibraryDb& db = mContext.workspace.getLibraryDb();
    PackageCategoryTreeLabelTextBuilder textBuilder(db, getLibLocaleOrder(),
                                                    *mUi->lblParentCategories);
    textBuilder.setEndlessRecursionUuid(mCategory->getUuid());
    textBuilder.setHighlightLastLine(true);

    QString trimmed = text.trimmed();
    tl::optional<Uuid> parentUuid = Uuid::tryFromString(trimmed);
    if ((trimmed.length() > 0) && !parentUuid) {
        textBuilder.setErrorText(tr("Invalid UUID!"));
    } else {
        textBuilder.updateText(parentUuid, mUi->edtName->text());
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
