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

#ifndef LIBREPCB_EDITOR_KEYSEQUENCESEDITORWIDGET_H
#define LIBREPCB_EDITOR_KEYSEQUENCESEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class KeySequencesEditorWidget
 ******************************************************************************/

/**
 * @brief A widget to modify a list of QKeySequence objects
 *
 * Used for ::librepcb::editor::KeySequenceDelegate.
 */
class KeySequencesEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  KeySequencesEditorWidget() = delete;
  explicit KeySequencesEditorWidget(const QList<QKeySequence>& defaultSequences,
                                    QWidget* parent = nullptr) noexcept;
  KeySequencesEditorWidget(const KeySequencesEditorWidget& other) = delete;
  ~KeySequencesEditorWidget() noexcept;

  // General Methods
  const std::optional<QList<QKeySequence>>& getOverrides() const noexcept {
    return mOverrides;
  }
  void setOverrides(
      const std::optional<QList<QKeySequence>>& overrides) noexcept;
  void setRowHeight(int height) noexcept;

  // Operator Overloadings
  KeySequencesEditorWidget& operator=(const KeySequencesEditorWidget& rhs) =
      delete;

signals:
  void applyTriggered();
  void cancelTriggered();

private:  // Methods
  void updateWidgets() noexcept;

private:  // Data
  QPointer<QVBoxLayout> mLayout;
  QList<QKeySequence> mDefault;
  std::optional<QList<QKeySequence>> mOverrides;
  int mRowHeight;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
