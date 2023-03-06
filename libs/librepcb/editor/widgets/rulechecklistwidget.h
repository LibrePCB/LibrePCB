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

#ifndef LIBREPCB_EDITOR_RULECHECKLISTWIDGET_H
#define LIBREPCB_EDITOR_RULECHECKLISTWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/rulecheck/rulecheckmessage.h>
#include <optional/tl/optional.hpp>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Interface IF_RuleCheckHandler
 ******************************************************************************/

class IF_RuleCheckHandler {
public:
  virtual bool ruleCheckFixAvailable(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept = 0;
  virtual void ruleCheckFixRequested(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept = 0;
  virtual void ruleCheckDescriptionRequested(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept = 0;
  virtual void ruleCheckApproveRequested(
      std::shared_ptr<const RuleCheckMessage> msg, bool approve) noexcept = 0;
  virtual void ruleCheckMessageSelected(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept = 0;
  virtual void ruleCheckMessageDoubleClicked(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept = 0;

protected:
  IF_RuleCheckHandler() noexcept {}
  IF_RuleCheckHandler(const IF_RuleCheckHandler&) noexcept {}
  virtual ~IF_RuleCheckHandler() noexcept {}
};

/*******************************************************************************
 *  Class RuleCheckListItemWidget
 ******************************************************************************/

/**
 * @brief The RuleCheckListItemWidget class
 */
class RuleCheckListItemWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit RuleCheckListItemWidget(std::shared_ptr<const RuleCheckMessage> msg,
                                   IF_RuleCheckHandler& handler, bool approved,
                                   QWidget* parent = nullptr) noexcept
    : QWidget(parent),
      mMessage(msg),
      mHandler(handler),
      mIconLabel(new QLabel(this)) {
    if (!msg) return;
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // severity icon
    mIconLabel->setScaledContents(true);
    mIconLabel->setPixmap(msg->getSeverityIcon().pixmap(
        48, approved ? QIcon::Disabled : QIcon::Normal));
    layout->addWidget(mIconLabel.data());
    layout->addSpacing(3);

    // message
    QLabel* lblMsg = new QLabel(msg->getMessage(), this);
    lblMsg->setToolTip(msg->getMessage());
    if (approved) {
      QFont font = lblMsg->font();
      font.setItalic(true);
      font.setStrikeOut(true);
      lblMsg->setFont(font);
    }
    layout->addWidget(lblMsg);
    layout->addSpacing(3);
    layout->setStretch(1, 100);

    // "fix" button
    if (mHandler.ruleCheckFixAvailable(mMessage)) {
      QToolButton* btnFix = new QToolButton(this);
      btnFix->setText(tr("Fix"));
      btnFix->setToolTip(tr("Fix Problem"));
      btnFix->setStatusTip(
          tr("Automatically apply a modification to fix this message"));
      connect(btnFix, &QToolButton::clicked, this,
              [this]() { mHandler.ruleCheckFixRequested(mMessage); });
      layout->addWidget(btnFix);
    }

    // "approve" button
    QToolButton* btnApprove = new QToolButton(this);
    btnApprove->setText("✔");
    btnApprove->setToolTip(tr("Approve/Disapprove"));
    btnApprove->setStatusTip(tr("Mark/unmark this message as approved"));
    btnApprove->setCheckable(true);
    btnApprove->setChecked(approved);
    connect(btnApprove, &QToolButton::clicked, this, [this, msg](bool checked) {
      mHandler.ruleCheckApproveRequested(mMessage, checked);
    });
    layout->addWidget(btnApprove);

    // "details" button
    QToolButton* btnDetails = new QToolButton(this);
    btnDetails->setText("?");
    btnDetails->setToolTip(tr("Details"));
    btnDetails->setStatusTip(tr("Show more information about this message"));
    connect(btnDetails, &QToolButton::clicked, this,
            [this]() { mHandler.ruleCheckDescriptionRequested(mMessage); });
    layout->addWidget(btnDetails);
  }
  RuleCheckListItemWidget(const RuleCheckListItemWidget& other) = delete;
  ~RuleCheckListItemWidget() noexcept {}

  // Operator Overloadings
  RuleCheckListItemWidget& operator=(const RuleCheckListItemWidget& rhs) =
      delete;

private:  // Methods
  void resizeEvent(QResizeEvent* event) override {
    QWidget::resizeEvent(event);
    mIconLabel->setFixedWidth(mIconLabel->height());
  }

private:  // Data
  std::shared_ptr<const RuleCheckMessage> mMessage;
  IF_RuleCheckHandler& mHandler;
  QScopedPointer<QLabel> mIconLabel;
};

/*******************************************************************************
 *  Class RuleCheckListWidget
 ******************************************************************************/

/**
 * @brief The RuleCheckListWidget class
 */
class RuleCheckListWidget final : public QWidget, private IF_RuleCheckHandler {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit RuleCheckListWidget(QWidget* parent = nullptr) noexcept;
  RuleCheckListWidget(const RuleCheckListWidget& other) = delete;
  ~RuleCheckListWidget() noexcept;

  // Getters
  const tl::optional<int>& getUnapprovedMessageCount() const noexcept {
    return mUnapprovedMessageCount;
  }

  // Setters
  void setReadOnly(bool readOnly) noexcept;
  void setHandler(IF_RuleCheckHandler* handler) noexcept;
  void setMessages(const tl::optional<RuleCheckMessageList>& messages) noexcept;
  void setApprovals(const QSet<SExpression>& approvals) noexcept;

  // Operator Overloadings
  RuleCheckListWidget& operator=(const RuleCheckListWidget& rhs) = delete;

private:  // Methods
  void updateList() noexcept;
  void currentItemChanged(QListWidgetItem* current,
                          QListWidgetItem* previous) noexcept;
  void itemDoubleClicked(QListWidgetItem* item) noexcept;
  bool ruleCheckFixAvailable(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  void ruleCheckFixRequested(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  void ruleCheckDescriptionRequested(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  void ruleCheckApproveRequested(std::shared_ptr<const RuleCheckMessage> msg,
                                 bool approve) noexcept override;
  void ruleCheckMessageSelected(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  void ruleCheckMessageDoubleClicked(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;

private:  // Data
  QScopedPointer<QListWidget> mListWidget;
  bool mReadOnly;
  IF_RuleCheckHandler* mHandler;
  tl::optional<RuleCheckMessageList> mMessages;
  RuleCheckMessageList mDisplayedMessages;
  QSet<SExpression> mApprovals;
  tl::optional<int> mUnapprovedMessageCount;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
