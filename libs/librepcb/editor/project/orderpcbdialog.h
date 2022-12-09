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

#ifndef LIBREPCB_EDITOR_ORDERPCBDIALOG_H
#define LIBREPCB_EDITOR_ORDERPCBDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

#include <functional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class OrderPcbApiRequest;
class WorkspaceSettings;

namespace editor {

namespace Ui {
class OrderPcbDialog;
}

/*******************************************************************************
 *  Class OrderPcbDialog
 ******************************************************************************/

/**
 * @brief The OrderPcbDialog class
 */
class OrderPcbDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  OrderPcbDialog() = delete;
  OrderPcbDialog(const OrderPcbDialog& other) = delete;
  explicit OrderPcbDialog(const WorkspaceSettings& settings,
                          std::function<QByteArray()> createLppzCallback,
                          QWidget* parent = nullptr) noexcept;
  ~OrderPcbDialog() noexcept;

  // Operator Overloads
  OrderPcbDialog& operator=(const OrderPcbDialog& rhs) = delete;

private:  // Methods
  void infoRequestSucceeded(QUrl infoUrl, int maxFileSize) noexcept;
  void infoRequestFailed(QString errorMsg) noexcept;
  void uploadButtonClicked() noexcept;
  void startUpload() noexcept;
  void uploadProgressPercent(int percent) noexcept;
  void uploadSucceeded(const QUrl& redirectUrl) noexcept;
  void uploadFailed(const QString& errorMsg) noexcept;
  void setStatus(const QString msg) noexcept;
  void setError(const QString& msg) noexcept;

private:  // Data
  const WorkspaceSettings& mSettings;
  QScopedPointer<OrderPcbApiRequest> mRequest;
  std::function<QByteArray()> mCreateLppzCallback;
  QScopedPointer<Ui::OrderPcbDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
