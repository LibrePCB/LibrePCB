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
#include "partinformationtooltip.h"

#include "../widgets/waitingspinnerwidget.h"
#include "../workspace/desktopservices.h"
#include "ui_partinformationtooltip.h"

#include <librepcb/core/network/networkrequest.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using PartInformation = PartInformationProvider::PartInformation;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PartInformationToolTip::PartInformationToolTip(
    const WorkspaceSettings& settings, QWidget* parent) noexcept
  : QFrame(parent),
    mSettings(settings),
    mUi(new Ui::PartInformationToolTip),
    mWaitingSpinner(new WaitingSpinnerWidget(this)),
    mExpandAnimation(new QVariantAnimation(this)),
    mPopUpDelayTimer(new QTimer(this)),
    mArrowPositionY(0),
    mPictureDelayTimer(new QTimer(this)) {
  mUi->setupUi(this);
  mUi->lblSourceDetails->setMinimumWidth(minimumWidth() - 20);  // Fix sizHint()
  setWindowFlags(Qt::ToolTip);
  mWaitingSpinner->hide();

  // Set up stylesheet.
  mWaitingSpinner->setColor(Qt::darkGray);
  mUi->line->setStyleSheet("border-top: 0.5px solid darkgray;");
  mUi->lblPicture->setStyleSheet("border: 0.5px solid darkgray;");
  setStyleSheet(QString("QWidget{"
                        " background-color: #FFFFCA;"
                        " color: black;"
                        "}"
                        "librepcb--editor--PartInformationToolTip{"
                        " border: %1px solid darkgray; "
                        " border-right: 1px solid gray; "
                        " border-top: 1px solid gray;"
                        " border-bottom: 1px solid gray;"
                        " padding: 0px;"
                        " margin: 0px;"
                        "};")
                    .arg(sWindowArrowSize + 1));

  // Set up expand/collapse animation.
  mExpandAnimation->setStartValue(0);
  mExpandAnimation->setEndValue(0);
  mExpandAnimation->setEasingCurve(QEasingCurve::InQuad);
  mExpandAnimation->setDuration(300);
  connect(mExpandAnimation.data(), &QVariantAnimation::valueChanged, this,
          [this](const QVariant& value) {
            mUi->lblSourceDetails->setFixedHeight(value.toInt());
            updateShape();
          });

  // Install label click event handlers.
  mUi->lblExpand->installEventFilter(this);
  mUi->lblSource->installEventFilter(this);
  mUi->lblProviderLogo->installEventFilter(this);
  connect(mUi->lblHeader, &QLabel::linkActivated, this,
          &PartInformationToolTip::openUrl);
  connect(mUi->lblDetails, &QLabel::linkActivated, this,
          &PartInformationToolTip::openUrl);
  connect(mUi->lblSourceDetails, &QLabel::linkActivated, this,
          &PartInformationToolTip::openUrl);

  // Close popup if parent has been hidden.
  if (parent) {
    parent->installEventFilter(this);
  }

  // Setup popup delay timer.
  mPopUpDelayTimer->setSingleShot(true);
  connect(mPopUpDelayTimer.data(), &QTimer::timeout, this,
          &PartInformationToolTip::show);

  // Set up picture loading delay timer.
  mPictureDelayTimer->setSingleShot(true);
  connect(mPictureDelayTimer.data(), &QTimer::timeout, this,
          [this]() { startLoadPicture(false); });

  setProviderInfo(QString(), QUrl(), QPixmap(), QUrl());
  hideAndReset();
}

PartInformationToolTip::~PartInformationToolTip() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PartInformationToolTip::setProviderInfo(const QString& name,
                                             const QUrl& url,
                                             const QPixmap& logo,
                                             const QUrl& infoUrl) noexcept {
  QString text;
  if (name.isEmpty()) {
    text = tr("This service is currently not available.");
  } else {
    const QString provider =
        QString("<a href=\"%1\" style=\"color:black\">%2</a>")
            .arg(url.toString())
            .arg(name.toHtmlEscaped());
    text = tr("This information is kindly provided by %1 through the "
              "LibrePCB&nbsp;API, see details "
              "<a href=\"%2\" style=\"color:black;\">here</a>.")
               .arg(provider)
               .arg(infoUrl.toString().toHtmlEscaped());
    text += " ";
    text += tr(
        "For more information about the part, click on the source logo above.");
  }
  mUi->lblSourceDetails->setText(text);
  if (logo.isNull()) {
    mUi->lblProviderLogo->setText(name);
  } else {
    setLabelPixmap(mUi->lblProviderLogo, logo, QSize(150, 13));
  }
  updateShape();
}

void PartInformationToolTip::showPart(
    const std::shared_ptr<const PartInformationProvider::PartInformation>& info,
    const QPoint& pos) noexcept {
  if (!info) {
    hideAndReset();
    return;
  }

  if ((!mPartInfo) || (info->mpn != mPartInfo->mpn) ||
      (info->manufacturer != mPartInfo->manufacturer)) {
    mPartInfo = info;

    QString header = QString(
                         "<span style=\"font-size:large\"><b><a href=\"%1\" "
                         "style=\"color:black\">%2</a></b></span>")
                         .arg(info->productUrl.toString())
                         .arg(info->mpn.toHtmlEscaped());
    if (!info->manufacturer.isEmpty()) {
      header +=
          QString("&nbsp;&nbsp;%1").arg(info->manufacturer.toHtmlEscaped());
    }
    mUi->lblHeader->setText(header);

    QString details;
    if (!info->getStatusTr().isEmpty()) {
      details += QString("<div><span style=\"color:%1\">⬤</span> %2</div>")
                     .arg(info->getStatusColorName())
                     .arg(info->getStatusTr().toHtmlEscaped());
    }
    if (!info->getAvailabilityTr().isEmpty()) {
      details += QString("<div><span style=\"color:%1\">⬤</span> %2</div>")
                     .arg(info->getAvailabilityColorName())
                     .arg(info->getAvailabilityTr().toHtmlEscaped());
    }
    if (!info->prices.isEmpty()) {
      details += "<div><table>";
      foreach (int quantity, info->prices.keys().mid(0, 3)) {
        details +=
            QString("<tr><td align=\"right\">%1 %2:</td><td>%3</td></tr>")
                .arg(PartInformation::formatQuantity(locale(), quantity))
                //: Abbreviation for "pieces", keep it very short!
                .arg(tr("pcs").toHtmlEscaped())
                .arg(info->getPriceStr(quantity, "", " USD").toHtmlEscaped());
      }
      details += "</table></div>";
    }
    foreach (const auto& resource, info->resources.mid(0, 2)) {
      details +=
          QString("<div>➤ <a href=\"%1\" style=\"color:black\">%2</a></div>")
              .arg(resource.url.toString())
              .arg(resource.name.toHtmlEscaped());
    }
    mUi->lblDetails->setText(details);

    if (info->pricingUrl.isValid()) {
      mUi->lblProviderLogo->setCursor(Qt::PointingHandCursor);
    } else {
      mUi->lblProviderLogo->unsetCursor();
    }

    mUi->lblPicture->hide();
    mPictureDelayTimer->stop();
  }

  mArrowPositionY = height() / 2;
  move(pos - QPoint(sWindowArrowSize, mArrowPositionY));
  setSourceDetailsExpanded(false, false);

  if (isVisible()) {
    mWaitingSpinner->hide();
    scheduleLoadPicture();
    updateShape();
  } else {
    mPopUpDelayTimer->start();
  }
}

void PartInformationToolTip::hideAndReset(bool resetTimer) noexcept {
  mPopUpDelayTimer->stop();
  if (resetTimer) {
    mPopUpDelayTimer->setInterval(sPopupDelayMs);
  }
  QFrame::hide();
}

bool PartInformationToolTip::eventFilter(QObject* watched,
                                         QEvent* event) noexcept {
  if (event->type() == QEvent::MouseButtonPress) {
    if ((watched == mUi->lblExpand) || (watched == mUi->lblSource)) {
      setSourceDetailsExpanded(mUi->lblSourceDetails->height() == 0, true);
    } else if ((watched == mUi->lblProviderLogo) && mPartInfo &&
               mPartInfo->pricingUrl.isValid()) {
      openUrl(mPartInfo->pricingUrl);
    }
  }
  if ((watched == parentWidget()) &&
      ((event->type() == QEvent::Hide) || (event->type() == QEvent::Close))) {
    hideAndReset();
  }
  return QFrame::eventFilter(watched, event);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void PartInformationToolTip::showEvent(QShowEvent* e) noexcept {
  QFrame::showEvent(e);
  mPopUpDelayTimer->setInterval(50);
  mWaitingSpinner->hide();
  scheduleLoadPicture();
  updateShape();
}

void PartInformationToolTip::hideEvent(QHideEvent* e) noexcept {
  QFrame::hideEvent(e);
  mPictureDelayTimer->stop();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PartInformationToolTip::scheduleLoadPicture() noexcept {
  if (mPartInfo && mPartInfo->pictureUrl.isValid() &&
      (!mUi->lblPicture->isVisible())) {
    mWaitingSpinner->show();
    startLoadPicture(true);
    mPictureDelayTimer->start(1000);
  }
}

void PartInformationToolTip::startLoadPicture(bool onlyCache) noexcept {
  if (mPartInfo && mPartInfo->pictureUrl.isValid() &&
      (!mUi->lblPicture->isVisible())) {
    NetworkRequest* request = new NetworkRequest(mPartInfo->pictureUrl);
    if (onlyCache) {
      // Immediately after showing the tooltip, only load the image from cache
      // to avoid extensive network load!
      request->setCacheLoadControl(QNetworkRequest::AlwaysCache);
    }
    request->setMinimumCacheTime(14 * 24 * 3600);  // 14 days
    const QString format =
        mPartInfo->pictureUrl.fileName().split('.').last().toLower();
    connect(
        request, &NetworkRequest::dataReceived, this,
        [this, format](const QByteArray& data) {
          QPixmap pixmap;
          if (!format.isEmpty()) {
            pixmap.loadFromData(data, qPrintable(format));
          }
          if (pixmap.isNull()) {
            pixmap.loadFromData(data);
          }
          if (!pixmap.isNull()) {
            mUi->lblPicture->setFrameShape(pixmap.hasAlpha() ? QFrame::NoFrame
                                                             : QFrame::Box);
            setLabelPixmap(mUi->lblPicture, pixmap,
                           mUi->pictureContainer->contentsRect().size());
          } else if (!data.isEmpty()) {
            qWarning().nospace()
                << "Failed to display image of format " << format
                << ". Maybe the Qt image formats plugin is not installed?";
          }
          mPictureDelayTimer->stop();
        },
        Qt::QueuedConnection);
    connect(request, &NetworkRequest::finished, this,
            [this, onlyCache](bool success) {
              if (success || (!onlyCache)) {
                mWaitingSpinner->hide();
              }
            });
    request->start();
  }
}

void PartInformationToolTip::setLabelPixmap(QLabel* label,
                                            const QPixmap& pixmap,
                                            const QSize& space) noexcept {
  const qreal scaleFactor = std::min(space.width() / qreal(pixmap.width()),
                                     space.height() / qreal(pixmap.height()));
  label->setFixedSize(pixmap.size() * scaleFactor);
  label->setPixmap(pixmap);
  label->show();
}

void PartInformationToolTip::updateShape() noexcept {
  adjustSize();
  const int w = width();
  const int h = height();
  setMask(QRegion(QPolygon({
      QPoint(0, mArrowPositionY),
      QPoint(sWindowArrowSize, mArrowPositionY - sWindowArrowSize),
      QPoint(sWindowArrowSize, 0),
      QPoint(w, 0),
      QPoint(w, h),
      QPoint(sWindowArrowSize, h),
      QPoint(sWindowArrowSize, mArrowPositionY + sWindowArrowSize),
      QPoint(0, mArrowPositionY),
  })));
}

void PartInformationToolTip::setSourceDetailsExpanded(bool expanded,
                                                      bool animated) noexcept {
  if (expanded) {
    mUi->lblExpand->setText("▼");
    mExpandAnimation->setEndValue(mUi->lblSourceDetails->sizeHint().height());
    mExpandAnimation->setDirection(QVariantAnimation::Forward);
  } else {
    mUi->lblExpand->setText("▶");
    mExpandAnimation->setEndValue(mUi->lblSourceDetails->height());
    mExpandAnimation->setDirection(QVariantAnimation::Backward);
  }
  if (animated) {
    mExpandAnimation->start();
  } else {
    mExpandAnimation->stop();
    mUi->lblSourceDetails->setFixedHeight(
        expanded ? mExpandAnimation->endValue().toInt()
                 : mExpandAnimation->startValue().toInt());
  }
}

void PartInformationToolTip::openUrl(const QUrl& url) noexcept {
  DesktopServices ds(mSettings, this);
  if (ds.openWebUrl(url)) {
    hideAndReset();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
