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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdlibrarybaseelementedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdLibraryBaseElementEdit::CmdLibraryBaseElementEdit(
    LibraryBaseElement& element, const QString& text) noexcept
  : UndoCommand(text),
    mElement(element),
    mOldNames(element.getNames()),
    mNewNames(mOldNames),
    mOldDescriptions(element.getDescriptions()),
    mNewDescriptions(mOldDescriptions),
    mOldKeywords(element.getKeywords()),
    mNewKeywords(mOldKeywords),
    mOldVersion(element.getVersion()),
    mNewVersion(mOldVersion),
    mOldAuthor(element.getAuthor()),
    mNewAuthor(mOldAuthor),
    mOldDeprecated(element.isDeprecated()),
    mNewDeprecated(mOldDeprecated) {
}

CmdLibraryBaseElementEdit::~CmdLibraryBaseElementEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdLibraryBaseElementEdit::setName(const QString& locale,
                                        const ElementName& name) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewNames.insert(locale, name);
}

void CmdLibraryBaseElementEdit::setNames(
    const LocalizedNameMap& names) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewNames = names;
}

void CmdLibraryBaseElementEdit::setDescription(const QString& locale,
                                               const QString& desc) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDescriptions.insert(locale, desc);
}

void CmdLibraryBaseElementEdit::setDescriptions(
    const LocalizedDescriptionMap& descriptions) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDescriptions = descriptions;
}

void CmdLibraryBaseElementEdit::setKeywords(const QString& locale,
                                            const QString& keywords) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewKeywords.insert(locale, keywords);
}

void CmdLibraryBaseElementEdit::setKeywords(
    const LocalizedKeywordsMap& keywords) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewKeywords = keywords;
}

void CmdLibraryBaseElementEdit::setVersion(const Version& version) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewVersion = version;
}

void CmdLibraryBaseElementEdit::setAuthor(const QString& author) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAuthor = author;
}

void CmdLibraryBaseElementEdit::setDeprecated(bool deprecated) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDeprecated = deprecated;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdLibraryBaseElementEdit::performExecute() {
  performRedo();  // can throw

  if (mNewNames != mOldNames) return true;
  if (mNewDescriptions != mOldDescriptions) return true;
  if (mNewKeywords != mOldKeywords) return true;
  if (mNewVersion != mOldVersion) return true;
  if (mNewAuthor != mOldAuthor) return true;
  if (mNewDeprecated != mOldDeprecated) return true;
  return false;
}

void CmdLibraryBaseElementEdit::performUndo() {
  mElement.setNames(mOldNames);
  mElement.setDescriptions(mOldDescriptions);
  mElement.setKeywords(mOldKeywords);
  mElement.setVersion(mOldVersion);
  mElement.setAuthor(mOldAuthor);
  mElement.setDeprecated(mOldDeprecated);
}

void CmdLibraryBaseElementEdit::performRedo() {
  mElement.setNames(mNewNames);
  mElement.setDescriptions(mNewDescriptions);
  mElement.setKeywords(mNewKeywords);
  mElement.setVersion(mNewVersion);
  mElement.setAuthor(mNewAuthor);
  mElement.setDeprecated(mNewDeprecated);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
