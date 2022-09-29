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

#ifndef LIBREPCB_EDITOR_EDITORCOMMANDCATEGORY_H
#define LIBREPCB_EDITOR_EDITORCOMMANDCATEGORY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class EditorCommandCategory
 ******************************************************************************/

/**
 * @brief Category for ::librepcb::editor::EditorCommand
 */
class EditorCommandCategory final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  EditorCommandCategory() = delete;
  EditorCommandCategory(const EditorCommandCategory& other) = delete;
  EditorCommandCategory(const QString& objectName, const char* text,
                        bool configurable, QObject* parent = nullptr) noexcept
    : QObject(parent),
      mTextNoTr(text),
      // Note: Translations are done within the EditorCommandSet context.
      mText(QCoreApplication::translate("EditorCommandSet", text)),
      mConfigurable(configurable) {
    setObjectName(objectName);
  }
  ~EditorCommandCategory() noexcept {}

  // Getters
  const char* getTextNoTr() const noexcept { return mTextNoTr; }
  const QString& getText() const noexcept { return mText; }
  bool isConfigurable() const noexcept { return mConfigurable; }

  // Operator Overloadings
  EditorCommandCategory& operator=(const EditorCommandCategory& rhs) = delete;

private:  // Data
  const char* mTextNoTr;
  QString mText;
  bool mConfigurable;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
