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

#ifndef LIBREPCB_LIBRARY_EDITOR_LIBRARYELEMENTCHECKLISTWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_LIBRARYELEMENTCHECKLISTWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/msg/libraryelementcheckmessage.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Interface IF_LibraryElementCheckHandler
 ******************************************************************************/

class IF_LibraryElementCheckHandler {
public:
  virtual bool libraryElementCheckFixAvailable(
      std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept = 0;
  virtual void libraryElementCheckFixRequested(
      std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept = 0;
  virtual void libraryElementCheckDescriptionRequested(
      std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept = 0;

protected:
  IF_LibraryElementCheckHandler() noexcept {}
  IF_LibraryElementCheckHandler(const IF_LibraryElementCheckHandler&) noexcept {
  }
  virtual ~IF_LibraryElementCheckHandler() noexcept {}
};

/*******************************************************************************
 *  Class LibraryElementCheckListItemWidget
 ******************************************************************************/

/**
 * @brief The LibraryElementCheckListItemWidget class
 */
class LibraryElementCheckListItemWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit LibraryElementCheckListItemWidget(
      std::shared_ptr<const LibraryElementCheckMessage> msg,
      IF_LibraryElementCheckHandler&                    handler,
      QWidget* parent = nullptr) noexcept
    : QWidget(parent),
      mMessage(msg),
      mHandler(handler),
      mIconLabel(new QLabel(this)) {
    if (!msg) return;
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);

    // severity icon
    mIconLabel->setScaledContents(true);
    mIconLabel->setPixmap(msg->getSeverityPixmap());
    layout->addWidget(mIconLabel.data());

    // message
    QLabel* lblMsg = new QLabel(msg->getMessage(), this);
    lblMsg->setToolTip(msg->getMessage());
    layout->addWidget(lblMsg);
    layout->setStretch(1, 100);

    // "fix" button
    if (mHandler.libraryElementCheckFixAvailable(mMessage)) {
      QToolButton* btnFix = new QToolButton(this);
      btnFix->setText(tr("Fix"));
      connect(btnFix, &QToolButton::clicked,
              [this]() { mHandler.libraryElementCheckFixRequested(mMessage); });
      layout->addWidget(btnFix);
    }

    // "details" button
    QToolButton* btnDetails = new QToolButton(this);
    btnDetails->setText("?");
    btnDetails->setToolTip(tr("Details"));
    connect(btnDetails, &QToolButton::clicked, [this]() {
      mHandler.libraryElementCheckDescriptionRequested(mMessage);
    });
    layout->addWidget(btnDetails);
  }
  LibraryElementCheckListItemWidget(
      const LibraryElementCheckListItemWidget& other) = delete;
  ~LibraryElementCheckListItemWidget() noexcept {}

  // Operator Overloadings
  LibraryElementCheckListItemWidget& operator       =(
      const LibraryElementCheckListItemWidget& rhs) = delete;

private:  // Methods
  void resizeEvent(QResizeEvent* event) override {
    QWidget::resizeEvent(event);
    mIconLabel->setFixedWidth(mIconLabel->height());
  }

private:  // Data
  std::shared_ptr<const LibraryElementCheckMessage> mMessage;
  IF_LibraryElementCheckHandler&                    mHandler;
  QScopedPointer<QLabel>                            mIconLabel;
};

/*******************************************************************************
 *  Class LibraryElementCheckListWidget
 ******************************************************************************/

/**
 * @brief The LibraryElementCheckListWidget class
 */
class LibraryElementCheckListWidget final
  : public QWidget,
    private IF_LibraryElementCheckHandler {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit LibraryElementCheckListWidget(QWidget* parent = nullptr) noexcept;
  LibraryElementCheckListWidget(const LibraryElementCheckListWidget& other) =
      delete;
  ~LibraryElementCheckListWidget() noexcept;

  // Setters
  void setHandler(IF_LibraryElementCheckHandler* handler) noexcept;
  void setMessages(LibraryElementCheckMessageList messages) noexcept;

  // Operator Overloadings
  LibraryElementCheckListWidget& operator       =(
      const LibraryElementCheckListWidget& rhs) = delete;

private:  // Methods
  void updateList() noexcept;
  void itemDoubleClicked(QListWidgetItem* item) noexcept;
  bool libraryElementCheckFixAvailable(
      std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept override;
  void libraryElementCheckFixRequested(
      std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept override;
  void libraryElementCheckDescriptionRequested(
      std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept override;

private:  // Data
  QScopedPointer<QListWidget>    mListWidget;
  IF_LibraryElementCheckHandler* mHandler;
  LibraryElementCheckMessageList mMessages;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_LIBRARYELEMENTCHECKLISTWIDGET_H
