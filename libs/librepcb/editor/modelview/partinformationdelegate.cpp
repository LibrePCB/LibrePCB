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
#include "partinformationdelegate.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class PartInformationDelegate::Data
 ******************************************************************************/

QSize PartInformationDelegate::Data::calcSizeHint(
    const QStyleOptionViewItem& option) const noexcept {
  QFont f = option.font;
  f.setPointSize(f.pointSize() - 2);
  QFontMetrics fm(f);
  const QString text = getDisplayText(true);
  QSize s = fm.size(Qt::AlignCenter, text);
  if (text.isEmpty()) {
    s.setWidth(6);  // Make it a square resp. circle.
  }
  return s;
}

QString PartInformationDelegate::Data::getDisplayText(
    bool maxLen) const noexcept {
  QString s;
  if (info && (info->results == 1)) {
    s = info->getPriceStr(priceQuantity);
    if (s.isEmpty()) {
      s = info->getStatusTr();
    }
  } else if ((!info) && (progress > 0)) {
    const QStringList values = {"․", "‥", "…"};
    return maxLen ? values.last() : values[progress % values.count()];
  }
  return s;
}

bool PartInformationDelegate::Data::getColors(QBrush& background, QPen& outline,
                                              QPen& text) const noexcept {
  if (info && (info->results == 1)) {
    const QString sts = info->status.toLower();
    const std::optional<int> av = info->availability;
    if (sts == "preview") {
      // Ignore availability in this case since preview state somehow indicates
      // the part might not be available *yet*, which users probably expect.
      background = Qt::blue;
      outline = Qt::NoPen;
      text = QPen(Qt::white);
      return true;
    } else if ((sts == "obsolete") || (av && ((*av) < -5))) {
      background = Qt::red;
      outline = Qt::NoPen;
      text = QPen(Qt::white);
      return true;
    } else if (av && ((*av) < 0)) {
      background = QColor::fromRgb(0xFFA500);  // Orange
      outline = Qt::NoPen;
      text = QPen(Qt::black);
      return true;
    } else if (sts == "nrnd") {
      background = Qt::darkGray;
      outline = Qt::NoPen;
      text = QPen(Qt::white);
      return true;
    } else if ((sts.isEmpty() || (sts == "active")) && (av && ((*av) < 5))) {
      background = Qt::yellow;
      outline = Qt::NoPen;
      text = QPen(Qt::black);
      return true;
    } else if ((sts.isEmpty() && (av) && ((*av) >= 5)) ||
               ((sts == "active") && (!av))) {
      background = Qt::darkGreen;
      outline = Qt::NoPen;
      text = QPen(Qt::white);
      return true;
    } else if ((sts == "active") && (av) && ((*av) >= 5)) {
      background = Qt::green;
      outline = Qt::NoPen;
      text = QPen(Qt::black);
      return true;
    } else {
      background = Qt::white;
      outline = QPen(Qt::black);
      text = QPen(Qt::black);
      return true;
    }
  } else if ((!info) && (progress > 0)) {
    background = Qt::transparent;
    outline = QPen(Qt::transparent);
    text = QPen(Qt::gray);
    return true;
  }
  return false;
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PartInformationDelegate::PartInformationDelegate(bool fillCell,
                                                 QObject* parent) noexcept
  : QStyledItemDelegate(parent), mFillCell(fillCell) {
}

PartInformationDelegate::~PartInformationDelegate() noexcept {
}

/*******************************************************************************
 *  Inherited from QStyledItemDelegate
 ******************************************************************************/

QSize PartInformationDelegate::sizeHint(
    const QStyleOptionViewItem& option,
    const QModelIndex& index) const noexcept {
  QSize size = QStyledItemDelegate::sizeHint(option, index);

  Data data;
  if (getData(index, data)) {
    const QSize s = data.calcSizeHint(option);
    size.setWidth(size.width() + s.width() + s.height() - 2);
    size.setHeight(std::max(size.height(), s.height() + 2));
  }
  return size;
}

void PartInformationDelegate::paint(QPainter* painter,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const {
  QStyledItemDelegate::paint(painter, option, index);

  Data data;
  if (getData(index, data)) {
    QBrush bgBrush;
    QPen outlinePen;
    QPen textPen;
    if (data.getColors(bgBrush, outlinePen, textPen)) {
      const QSize textSize = data.calcSizeHint(option);
      const QSize bgSize(textSize.width() + textSize.height() - 4,
                         textSize.height());
      QRect rect(QPoint(0, 0), bgSize);
      if (mFillCell) {
        rect.setWidth(option.rect.width() - 2);
        rect.translate(option.rect.center() - rect.center());
      } else {
        rect.translate(option.rect.right() - rect.right() - 1,
                       option.rect.center().y() - rect.center().y());
      }
      painter->setBrush(bgBrush);
      painter->setPen(outlinePen);
      painter->drawRoundedRect(rect, rect.height() / 2, rect.height() / 2);

      const QString text = data.getDisplayText();
      if (!text.isEmpty()) {
        QFont f = option.font;
        f.setPointSize(f.pointSize() - 2);
        painter->setFont(f);
        painter->setPen(textPen);
        painter->drawText(rect, Qt::AlignCenter, text);
      }
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PartInformationDelegate::getData(const QModelIndex& index,
                                      Data& data) const noexcept {
  const QVariant d = index.data(Qt::UserRole);
  if (d.canConvert<Data>()) {
    data = d.value<Data>();
    return (data.info && (data.info->results == 1)) ||
        ((!data.info) && (data.progress > 0));
  } else {
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
