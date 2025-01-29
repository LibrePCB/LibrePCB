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

#ifndef LIBREPCB_PROJECT_PROJECTEDITOR_H
#define LIBREPCB_PROJECT_PROJECTEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/serialization/sexpression.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;

namespace editor {

class UndoStack;

namespace app {

/*******************************************************************************
 *  Class ProjectEditor
 ******************************************************************************/

/**
 * @brief The ProjectEditor class
 */
class ProjectEditor : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectEditor() = delete;
  ProjectEditor(const ProjectEditor& other) = delete;
  explicit ProjectEditor(std::unique_ptr<Project> project,
                         QObject* parent = nullptr) noexcept;
  virtual ~ProjectEditor() noexcept;

  // Getters
  Project& getProject() noexcept { return *mProject; }
  UndoStack& getUndoStack() noexcept { return *mUndoStack; }
  auto getErcMessages() noexcept { return mErcMessages; }

  // Operator Overloadings
  ProjectEditor& operator=(const ProjectEditor& rhs) = delete;

private:
  void runErc() noexcept;
  void saveErcMessageApprovals(const QSet<SExpression>& approvals) noexcept;

private:
  std::unique_ptr<Project> mProject;
  std::unique_ptr<UndoStack> mUndoStack;

  QSet<SExpression> mSupportedErcApprovals;
  QSet<SExpression> mDisappearedErcApprovals;
  std::shared_ptr<slint::VectorModel<ui::RuleCheckMessageData>> mErcMessages;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
