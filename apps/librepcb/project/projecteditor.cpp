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
#include "projecteditor.h"

#include "../apptoolbox.h"
#include "../uitypes.h"

#include <librepcb/core/project/erc/electricalrulecheck.h>
#include <librepcb/core/project/project.h>
#include <librepcb/editor/undostack.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectEditor::ProjectEditor(std::unique_ptr<Project> project,
                             QObject* parent) noexcept
  : QObject(parent),
    mProject(std::move(project)),
    mUndoStack(new UndoStack()),
    mErcMessages(new slint::VectorModel<ui::RuleCheckMessageData>()) {
  // Run the ERC after opening and after every modification.
  QTimer::singleShot(200, this, &ProjectEditor::runErc);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &ProjectEditor::runErc);
}

ProjectEditor::~ProjectEditor() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectEditor::runErc() noexcept {
  try {
    QElapsedTimer timer;
    timer.start();
    ElectricalRuleCheck erc(*mProject);
    const auto messages = erc.runChecks();

    // Detect disappeared messages & remove their approvals.
    QSet<SExpression> approvals = RuleCheckMessage::getAllApprovals(messages);
    mSupportedErcApprovals |= approvals;
    mDisappearedErcApprovals = mSupportedErcApprovals - approvals;
    approvals = mProject->getErcMessageApprovals() - mDisappearedErcApprovals;
    saveErcMessageApprovals(approvals);

    // Update UI.
    mErcMessages->clear();
    for (const auto& msg : messages) {
      mErcMessages->push_back(ui::RuleCheckMessageData{
          l2s(msg->getSeverity()),  // Severity
          q2s(msg->getMessage()),  // Message
          q2s(msg->getDescription()),  // Description
          approvals.contains(msg->getApproval()),  // Approved
      });
    }

    qDebug() << "ERC succeeded after" << timer.elapsed() << "ms.";
  } catch (const Exception& e) {
    qCritical() << "ERC failed:" << e.getMsg();
  }
}

void ProjectEditor::saveErcMessageApprovals(
    const QSet<SExpression>& approvals) noexcept {
  if (mProject->setErcMessageApprovals(approvals)) {
    // setManualModificationsMade(); TODO
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
