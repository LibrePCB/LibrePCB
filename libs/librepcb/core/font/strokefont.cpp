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
#include "strokefont.h"

#include <fontobene-qt5/font.h>
#include <fontobene-qt5/glyphlistaccessor.h>

#include <QtConcurrent/QtConcurrent>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

namespace fb = fontobene;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

StrokeFont::StrokeFont(const FilePath& fontFilePath,
                       const QByteArray& content) noexcept
  : QObject(nullptr), mFilePath(fontFilePath) {
  // load the font in another thread because it takes some time to load it
  qDebug() << "Start loading stroke font " << mFilePath.toNative()
           << "in worker thread...";
  mFuture = QtConcurrent::run([content]() {
    QTextStream s(content);
    return fb::Font(s);
  });
  connect(&mWatcher, &QFutureWatcher<fb::Font>::finished, this,
          &StrokeFont::fontLoaded);
  mWatcher.setFuture(mFuture);
}

StrokeFont::~StrokeFont() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Ratio StrokeFont::getLetterSpacing() const noexcept {
  accessor();  // block until the font is loaded.
  return Ratio::fromNormalized(mFont->header.letterSpacing / 9);
}

Ratio StrokeFont::getLineSpacing() const noexcept {
  accessor();  // block until the font is loaded.
  return Ratio::fromNormalized(mFont->header.lineSpacing / 9);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QVector<Path> StrokeFont::stroke(const QString& text,
                                 const PositiveLength& height,
                                 const Length& letterSpacing,
                                 const Length& lineSpacing,
                                 const Alignment& align, Point& bottomLeft,
                                 Point& topRight) const noexcept {
  accessor();  // block until the font is loaded. TODO: abort instead of
               // waiting?
  QVector<Path> paths;
  Length totalWidth;
  QVector<QPair<QVector<Path>, Length>> lines =
      strokeLines(text, height, letterSpacing, totalWidth);
  Length totalHeight = height + lineSpacing * (lines.count() - 1);
  for (int i = 0; i < lines.count(); ++i) {
    Point pos(0, 0);
    if (align.getH() == HAlign::left()) {
      pos.setX(Length(0));
    } else if (align.getH() == HAlign::right()) {
      pos.setX((totalWidth - lines.at(i).second) - totalWidth);
    } else {
      pos.setX(lines.at(i).second / -2);
    }
    if (align.getV() == VAlign::bottom()) {
      pos.setY(lineSpacing * (lines.count() - i - 1));
    } else if (align.getV() == VAlign::top()) {
      pos.setY(-height - lineSpacing * i);
    } else {
      Length h = lineSpacing * (lines.count() - i - 1);
      pos.setY(h - (totalHeight / 2));
    }
    foreach (const Path& p, lines.at(i).first) {
      paths.append(p.translated(pos));
    }
  }

  if (align.getH() == HAlign::left()) {
    bottomLeft.setX(0);
    topRight.setX(totalWidth);
  } else if (align.getH() == HAlign::right()) {
    bottomLeft.setX(-totalWidth);
    topRight.setX(0);
  } else {
    bottomLeft.setX(-totalWidth / 2);
    topRight.setX(totalWidth / 2);
  }
  if (align.getV() == VAlign::bottom()) {
    bottomLeft.setY(0);
    topRight.setY(totalHeight);
  } else if (align.getV() == VAlign::top()) {
    bottomLeft.setY(-totalHeight);
    topRight.setY(0);
  } else {
    bottomLeft.setY(-totalHeight / 2);
    topRight.setY(totalHeight / 2);
  }

  return paths;
}

QVector<QPair<QVector<Path>, Length>> StrokeFont::strokeLines(
    const QString& text, const PositiveLength& height,
    const Length& letterSpacing, Length& width) const noexcept {
  QVector<QPair<QVector<Path>, Length>> result;
  foreach (const QString& line, text.split('\n')) {
    QPair<QVector<Path>, Length> pair;
    pair.first = strokeLine(line, height, letterSpacing, pair.second);
    result.append(pair);
    if (pair.second > width) width = pair.second;
  }
  return result;
}

QVector<Path> StrokeFont::strokeLine(const QString& text,
                                     const PositiveLength& height,
                                     const Length& letterSpacing,
                                     Length& width) const noexcept {
  QVector<Path> paths;
  Length offset = 0;
  width = 0;  // same as offset, but without last letter spacing
  for (int i = 0; i < text.length(); ++i) {
    Length glyphSpacing;
    QVector<Path> glyphPaths = strokeGlyph(text.at(i), height, glyphSpacing);
    if (!glyphPaths.isEmpty()) {
      Point bottomLeft, topRight;
      computeBoundingRect(glyphPaths, bottomLeft, topRight);
      Length shift =
          (i == 0) ? -bottomLeft.getX() : 0;  // left-align first character
      foreach (const Path& p, glyphPaths) {
        paths.append(p.translated(Point(offset + shift, Length(0))));
      }
      width = offset + topRight.getX() +
          shift;  // do *not* count glyph spacing as width!
      offset = width + glyphSpacing + letterSpacing;
    } else if (glyphSpacing != 0) {
      // it's a whitespace-only glyph -> count additional glyph spacing as width
      width = offset + glyphSpacing;
      offset = width + letterSpacing;
    }
  }
  return paths;
}

QVector<Path> StrokeFont::strokeGlyph(const QChar& glyph,
                                      const PositiveLength& height,
                                      Length& spacing) const noexcept {
  try {
    qreal glyphSpacing = 0;
    QVector<fb::Polyline> polylines =
        accessor().getAllPolylinesOfGlyph(glyph.unicode(),
                                          &glyphSpacing);  // can throw
    spacing = convertLength(height, glyphSpacing);
    return polylines2paths(polylines, height);
  } catch (const fb::Exception& e) {
    qWarning().nospace() << "Failed to load stroke font glyph " << glyph << ".";
    spacing = 0;
    return QVector<Path>();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void StrokeFont::fontLoaded() noexcept {
  accessor();  // trigger the message about loading succeeded or failed
}

const fb::GlyphListAccessor& StrokeFont::accessor() const noexcept {
  if (!mFont) {
    try {
      mFont.reset(new fb::Font(mFuture.result()));  // can throw
      qDebug() << "Successfully loaded stroke font" << mFilePath.toNative()
               << "with" << mFont->glyphs.count() << "glyphs.";
    } catch (const fb::Exception& e) {
      mFont.reset(new fb::Font());
      qCritical().nospace() << "Failed to load stroke font "
                            << mFilePath.toNative() << ": " << e.msg();
    }

    mGlyphListCache.reset(new fb::GlyphListCache(mFont->glyphs));
    mGlyphListCache->setReplacementGlyph(
        0xFFFD);  // U+FFFD REPLACEMENT CHARACTER
    mGlyphListCache->addReplacements(
        {0x00B5, 0x03BC});  // MICRO SIGN <-> GREEK SMALL LETTER MU
    mGlyphListCache->addReplacements(
        {0x2126, 0x03A9});  // OHM SIGN <-> GREEK CAPITAL LETTER OMEGA

    mGlyphListAccessor.reset(new fb::GlyphListAccessor(*mGlyphListCache));
  }
  return *mGlyphListAccessor;
}

QVector<Path> StrokeFont::polylines2paths(
    const QVector<fb::Polyline>& polylines,
    const PositiveLength& height) noexcept {
  QVector<Path> paths;
  foreach (const fb::Polyline& p, polylines) {
    if (p.isEmpty()) continue;
    paths.append(polyline2path(p, height));
  }
  return paths;
}

Path StrokeFont::polyline2path(const fb::Polyline& p,
                               const PositiveLength& height) noexcept {
  Path path;
  foreach (const fb::Vertex& v, p) {
    path.addVertex(convertVertex(v, height));
  }
  return path;
}

Vertex StrokeFont::convertVertex(const fb::Vertex& v,
                                 const PositiveLength& height) noexcept {
  return Vertex(
      Point::fromMm(v.scaledX(height->toMm()), v.scaledY(height->toMm())),
      Angle::fromDeg(v.scaledBulge(180)));
}

Length StrokeFont::convertLength(const PositiveLength& height,
                                 qreal length) const noexcept {
  return Length(height->toNm() * length / 9);
}

void StrokeFont::computeBoundingRect(const QVector<Path>& paths,
                                     Point& bottomLeft,
                                     Point& topRight) noexcept {
  QRectF rect = Path::toQPainterPathPx(paths, false).boundingRect();
  bottomLeft = Point::fromPx(rect.bottomLeft());
  topRight = Point::fromPx(rect.topRight());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
