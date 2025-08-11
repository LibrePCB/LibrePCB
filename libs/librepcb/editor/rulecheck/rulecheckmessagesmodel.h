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

#ifndef LIBREPCB_EDITOR_RULECHECKMESSAGESMODEL_H
#define LIBREPCB_EDITOR_RULECHECKMESSAGESMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/rulecheck/rulecheckmessage.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class RuleCheckMessagesModel
 ******************************************************************************/

/**
 * @brief The RuleCheckMessagesModel class
 */
class RuleCheckMessagesModel : public QObject,
                               public slint::Model<ui::RuleCheckMessageData> {
  Q_OBJECT

public:
  // Types
  typedef std::function<bool(const std::shared_ptr<const RuleCheckMessage>& msg,
                             bool checkOnly)>
      AutofixHandler;

  // Constructors / Destructor
  RuleCheckMessagesModel(const RuleCheckMessagesModel& other) = delete;
  explicit RuleCheckMessagesModel(QObject* parent = nullptr) noexcept;
  virtual ~RuleCheckMessagesModel() noexcept;

  // General Methods
  void clear() noexcept;
  void setAutofixHandler(AutofixHandler handler) noexcept;
  void setMessages(const RuleCheckMessageList& messages,
                   const QSet<SExpression>& approvals) noexcept;
  int getUnapprovedCount() const noexcept { return mUnapprovedCount; }
  int getErrorCount() const noexcept { return mErrorCount; }

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::RuleCheckMessageData> row_data(
      std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::RuleCheckMessageData& data) noexcept override;

  // Operator Overloadings
  RuleCheckMessagesModel& operator=(const RuleCheckMessagesModel& rhs) = delete;

signals:
  void unapprovedCountChanged(int count);
  void errorCountChanged(int count);
  void approvalChanged(const SExpression& approval, bool approved);
  void highlightRequested(std::shared_ptr<const RuleCheckMessage> msg,
                          bool zoomTo);

private:
  void sortMessages() noexcept;
  void updateCounters() noexcept;

  AutofixHandler mAutofixHandler;
  RuleCheckMessageList mMessages;
  QSet<SExpression> mApprovals;
  int mUnapprovedCount;
  int mErrorCount;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
