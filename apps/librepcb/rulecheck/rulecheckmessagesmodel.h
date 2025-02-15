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

#ifndef LIBREPCB_RULECHECK_RULECHECKMESSAGESMODEL_H
#define LIBREPCB_RULECHECK_RULECHECKMESSAGESMODEL_H

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
namespace app {

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
  // Constructors / Destructor
  RuleCheckMessagesModel(const RuleCheckMessagesModel& other) = delete;
  explicit RuleCheckMessagesModel(QObject* parent = nullptr) noexcept;
  virtual ~RuleCheckMessagesModel() noexcept;

  // General Methods
  void setMessages(const RuleCheckMessageList& messages,
                   const QSet<SExpression>& approvals) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::RuleCheckMessageData> row_data(
      std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::RuleCheckMessageData& data) noexcept override;

  // Operator Overloadings
  RuleCheckMessagesModel& operator=(const RuleCheckMessagesModel& rhs) = delete;

signals:
  void approvalChanged(const SExpression& approval, bool approved);
  void autofixRequested(std::shared_ptr<const RuleCheckMessage> msg);

private:
  RuleCheckMessageList mMessages;
  QSet<SExpression> mApprovals;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
