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

#ifndef LIBREPCB_EDITOR_RULECHECKDOCK_H
#define LIBREPCB_EDITOR_RULECHECKDOCK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "rulechecklistwidget.h"

#include <librepcb/core/rulecheck/rulecheckmessage.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

namespace Ui {
class RuleCheckDock;
}

/*******************************************************************************
 *  Class RuleCheckDock
 ******************************************************************************/

/**
 * @brief The RuleCheckDock class
 */
class RuleCheckDock final : public QDockWidget, private IF_RuleCheckHandler {
  Q_OBJECT

public:
  // Types
  enum class Mode {
    ElectricalRuleCheck,
    BoardDesignRuleCheck,
  };

  // Constructors / Destructor
  explicit RuleCheckDock(Mode mode, QWidget* parent = nullptr) noexcept;
  RuleCheckDock(const RuleCheckDock& other) = delete;
  ~RuleCheckDock() noexcept;

  // Setters
  /**
   * @brief Set whether the dock widget should be interactive or not
   *
   * @param interactive   True if enabled, false if disabled.
   *
   * @return  Whether the widget was interactive *before* calling this method.
   *          Useful to temporarily disable widget & restore previous state.
   */
  bool setInteractive(bool interactive) noexcept;
  void setProgressPercent(int percent) noexcept;
  void setProgressStatus(const QString& status) noexcept;
  void setMessages(const tl::optional<RuleCheckMessageList>& messages) noexcept;
  void setApprovals(const QSet<SExpression>& approvals) noexcept;

  // Operator Overloadings
  RuleCheckDock& operator=(const RuleCheckDock& rhs) = delete;

signals:
  void settingsDialogRequested();
  void runDrcRequested();
  void runQuickCheckRequested();
  void messageApprovalRequested(const RuleCheckMessage& msg, bool approve);
  void messageSelected(const RuleCheckMessage& msg, bool zoomTo);

private:  // Methods
  void updateTitle(tl::optional<int> unapprovedMessages) noexcept;
  virtual bool ruleCheckFixAvailable(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  virtual void ruleCheckFixRequested(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  virtual void ruleCheckDescriptionRequested(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  virtual void ruleCheckApproveRequested(
      std::shared_ptr<const RuleCheckMessage> msg,
      bool approve) noexcept override;
  virtual void ruleCheckMessageSelected(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  virtual void ruleCheckMessageDoubleClicked(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;

private:
  const Mode mMode;
  QScopedPointer<Ui::RuleCheckDock> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
