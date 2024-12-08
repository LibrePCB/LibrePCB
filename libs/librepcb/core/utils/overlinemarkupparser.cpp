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
#include "overlinemarkupparser.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QVector<std::pair<int, int>> OverlineMarkupParser::extract(
    const QString& input, QString& output) noexcept {
  output.clear();

  // Determine length of string without trailing '!'.
  int inputLength = input.length();
  while ((inputLength > 0) && (input.at(inputLength - 1) == '!')) {
    --inputLength;
  }

  // Convert remaining '!' to overlines and '!!' to '!'.
  QVector<std::pair<int, int>> spans;
  int spanStart = -1;
  for (int i = 0; i < inputLength; ++i) {
    if (QStringView(input).mid(i, 2) == QLatin1String("!!")) {
      // Substitutte '!!' by '!'.
      output.append('!');
      ++i;
    } else if (QStringView(input).mid(i, 2) == QLatin1String("!/")) {
      // Do not end overline if '/' is prefixed with '!'.
      if (spanStart < 0) {
        spanStart = output.length();
      }
      output.append('/');
      ++i;
    } else if ((spanStart < 0) && (input.at(i) == '!')) {
      // Start overline on single '!'.
      spanStart = output.length();
    } else if ((spanStart >= 0) && (input.at(i) == '!')) {
      // End overline on single '!'.
      spans.append(std::make_pair(spanStart, output.length() - spanStart));
      spanStart = -1;
    } else if ((spanStart >= 0) && (input.at(i) == '/')) {
      // End overline implicitly on '/'.
      spans.append(std::make_pair(spanStart, output.length() - spanStart));
      spanStart = -1;
      output.append(input.at(i));
    } else {
      output.append(input.at(i));
    }
  }

  // Append trailing '!' as-is.
  while (inputLength < input.length()) {
    output.append('!');
    ++inputLength;
  }

  // Finish current span.
  if (spanStart >= 0) {
    spans.append(std::make_pair(spanStart, output.length() - spanStart));
  }

  return spans;
}

QVector<QLineF> OverlineMarkupParser::calculate(
    const QString& text, const QFontMetricsF& fm, int flags,
    const QVector<std::pair<int, int>>& spans, QRectF& boundingRect) noexcept {
  QVector<QLineF> overlines;
  boundingRect = fm.boundingRect(QRectF(), flags | Qt::TextDontClip, text);
  const qreal yBase = boundingRect.top() - fm.overlinePos();
  foreach (const auto& span, spans) {
    const QRectF prefixRect = fm.boundingRect(
        QRectF(), Qt::TextDontClip | Qt::AlignBottom | Qt::AlignLeft,
        text.left(span.first));
    const QRectF suffixRect = fm.boundingRect(
        QRectF(), Qt::TextDontClip | Qt::AlignBottom | Qt::AlignLeft,
        text.mid(span.first + span.second));
    overlines.append(QLineF(
        boundingRect.left() + prefixRect.width(), yBase - prefixRect.top(),
        boundingRect.right() - suffixRect.width(), yBase - prefixRect.top()));
  }
  return overlines;
}

void OverlineMarkupParser::process(const QString& input,
                                   const QFontMetricsF& fm, int flags,
                                   QString& output, QVector<QLineF>& overlines,
                                   QRectF& boundingRect) noexcept {
  const QVector<std::pair<int, int>> spans = extract(input, output);
  overlines = calculate(output, fm, flags, spans, boundingRect);
}

qreal OverlineMarkupParser::getLineWidth(qreal heightPx) noexcept {
  return heightPx / 15;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
