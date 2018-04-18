/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_STROKEFONT_H
#define LIBREPCB_STROKEFONT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../alignment.h"
#include "../fileio/filepath.h"
#include "../geometry/path.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace fontobene {
struct Vertex;
struct Polyline;
struct Font;
class GlyphListCache;
class GlyphListAccessor;
}

namespace librepcb {

/*****************************************************************************************
 *  Class StrokeFont
 ****************************************************************************************/

/**
 * @brief The StrokeFont class
 */
class StrokeFont final : public QObject
{
        Q_OBJECT

    public:
        // Constructors / Destructor
        StrokeFont(const FilePath& fontFilePath) noexcept;
        StrokeFont(const StrokeFont& other) = delete;
        ~StrokeFont() noexcept;

        // Getters
        Ratio getLetterSpacing() const noexcept;
        Ratio getLineSpacing() const noexcept;

        // General Methods
        QVector<Path> stroke(const QString& text, const Length& height,
                             const Length& letterSpacing, const Length& lineSpacing,
                             const Alignment& align, Point& bottomLeft, Point& topRight) const noexcept;
        QVector<QPair<QVector<Path>, Length>> strokeLines(const QString& text,
                                                          const Length& height,
                                                          const Length& letterSpacing,
                                                          Length& width) const noexcept;
        QVector<Path> strokeLine(const QString& text, const Length& height,
                                 const Length& letterSpacing, Length& width) const noexcept;
        QVector<Path> strokeGlyph(const QChar& glyph, const Length& height,
                                  Length& spacing) const noexcept;

        // Operator Overloadings
        StrokeFont& operator=(const StrokeFont& rhs) = delete;


    private:
        void fontLoaded() noexcept;
        const fontobene::GlyphListAccessor& accessor() const noexcept;
        static QVector<Path> polylines2paths(const QVector<fontobene::Polyline>& polylines,
                                             const Length& height) noexcept;
        static Path polyline2path(const fontobene::Polyline& p, const Length& height) noexcept;
        static Vertex convertVertex(const fontobene::Vertex& v, const Length& height) noexcept;
        Length convertLength(const Length& height, qreal length) const noexcept;
        static void computeBoundingRect(const QVector<Path>& paths,
                                        Point& bottomLeft, Point& topRight) noexcept;


    private: // Data
        FilePath mFilePath;
        QFuture<fontobene::Font> mFuture;
        QFutureWatcher<fontobene::Font> mWatcher;
        mutable QScopedPointer<fontobene::Font> mFont;
        mutable QScopedPointer<fontobene::GlyphListCache> mGlyphListCache;
        mutable QScopedPointer<fontobene::GlyphListAccessor> mGlyphListAccessor;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_STROKEFONT_H
