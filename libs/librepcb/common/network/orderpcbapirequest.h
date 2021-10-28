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

#ifndef LIBREPCB_ORDERPCBAPIREQUEST_H
#define LIBREPCB_ORDERPCBAPIREQUEST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class OrderPcbApiRequest
 ******************************************************************************/

/**
 * @brief Order a PCB via a LibrePCB API server
 *
 * See details at @ref doc_server_api_resources_order.
 */
class OrderPcbApiRequest final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  OrderPcbApiRequest() = delete;
  OrderPcbApiRequest(const OrderPcbApiRequest& other) = delete;

  /**
   * @brief Constructor
   *
   * @param apiServerUrl  URL of the API server (as set in workspace settings).
   * @param parent        Parent widget.
   */
  explicit OrderPcbApiRequest(const QUrl& apiServerUrl,
                              QObject* parent = nullptr) noexcept;

  /**
   * @brief Destructor
   */
  ~OrderPcbApiRequest() noexcept;

  // Getters

  /**
   * @brief Check if the information for upload was successfully received
   *
   * @retval true   Ready to call #startUpload().
   * @retval false  Not ready, #startUpload() should not be called.
   */
  bool isReadyForUpload() const noexcept { return mUploadUrl.isValid(); }

  /**
   * @brief Get received URL to service information
   *
   * @return URL (might be invalid if N/A or failed to receive information).
   */
  const QUrl& getReceivedInfoUrl() const noexcept { return mInfoUrl; }

  /**
   * @brief Get received URL where to upload the project
   *
   * @return Upload URL (might be invalid if failed to receive information).
   */
  const QUrl& getReceivedUploadUrl() const noexcept { return mUploadUrl; }

  /**
   * @brief Get maximum allowed project file size to upload
   *
   * @return Max. size in bytes (-1 if unknown).
   */
  int getReceivedMaxFileSize() const noexcept { return mMaxFileSize; }

  // General Methods

  /**
   * @brief Request the upload information from the API server
   *
   * Will request information from the API server, and then emits either
   * #infoRequestSucceeded() or #infoRequestFailed().
   */
  void startInfoRequest() noexcept;

  /**
   * @brief Upload the project
   *
   * Will emit #uploadProgressState() and #uploadProgressPercent() during the
   * upload, and either #uploadSucceeded() or #uploadFailed() when the upload
   * is finished.
   *
   * @param lppz        Project as a ZIP export.
   * @param boardPath   Path to the pre-selected board (e.g.
   *                    "boards/default/board.lp"). Leave empty if unknown.
   */
  void startUpload(const QByteArray& lppz, const QString& boardPath) const
      noexcept;

  // Operators
  OrderPcbApiRequest& operator=(const OrderPcbApiRequest& rhs) = delete;

signals:
  /**
   * @brief Information request succeeded
   *
   * @param infoUrl       The received service information URL.
   * @param maxFileSize   The received maximum file size in bytes (-1 if N/A).
   */
  void infoRequestSucceeded(QUrl infoUrl, int maxFileSize) const;

  /**
   * @brief Information request failed
   *
   * @param errorMsg  Error message.
   */
  void infoRequestFailed(QString errorMsg) const;

  /**
   * @brief Upload progress state changed
   *
   * @param state   SHort description of current state.
   */
  void uploadProgressState(QString state) const;

  /**
   * @brief Upload progress in percent changed
   *
   * @param percent   Current upload progress in percent.
   */
  void uploadProgressPercent(int percent) const;

  /**
   * @brief Upload succeeded
   *
   * @param redirectUrl   The received URL to be opened in the web browser.
   */
  void uploadSucceeded(QUrl redirectUrl) const;

  /**
   * @brief Information request failed
   *
   * @param errorMsg  Error message.
   */
  void uploadFailed(QString errorMsg) const;

private:  // Methods
  void infoRequestResponseReceived(const QByteArray& data) noexcept;
  void uploadResponseReceived(const QByteArray& data) const noexcept;

private:  // Data
  QUrl mApiServerUrl;

  // Data received from the info request
  QUrl mInfoUrl;
  QUrl mUploadUrl;
  int mMaxFileSize;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
