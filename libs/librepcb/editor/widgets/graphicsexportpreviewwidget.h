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

#ifndef LIBREPCB_EDITOR_GRAPHICSEXPORTPREVIEWWIDGET_H
#define LIBREPCB_EDITOR_GRAPHICSEXPORTPREVIEWWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class GraphicsExportWidget
 ******************************************************************************/

/**
 * @brief Like QGraphicsExportWidget, just better
 *
 * Main differences to QGraphicsExportWidget:
 *
 * - Does not enforce a cumbersome software architecture, making this widget
 *   easier to integrate (passing QImage objects instead of implementing
 *   a callback which draws on a QPrinter)
 * - Supports per-page size, i.e. each page can have a different size
 * - Draws page margins and page numbers
 * - Less configurable, supports only the configuration we need for LibrePCB
 */
class GraphicsExportWidget final : public QWidget {
  Q_OBJECT

  class PageItem final : public QGraphicsItem {
  public:
    // Constructors / Destructor
    PageItem() = delete;
    PageItem(bool showPageNumber, bool showResolution, int number) noexcept;
    PageItem(const PageItem& other) = delete;
    ~PageItem() noexcept;

    // General Methods
    void setContent(const QSize& pageSize, const QRectF margins,
                    std::shared_ptr<QPicture> picture) noexcept;
    QRectF boundingRect() const noexcept override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) noexcept override;

    // Operator Overloadings
    PageItem& operator=(const PageItem& rhs) = delete;

  private:  // Methods
    QSize getSize() const noexcept;
    QString getResolution() const noexcept;

  private:  // Data
    bool mShowPageNumbers;
    bool mShowResolution;
    int mNumber;
    QSize mSize;
    QRectF mMargins;
    std::shared_ptr<QPicture> mPicture;
  };

public:
  // Constructors / Destructor
  GraphicsExportWidget(const GraphicsExportWidget& other) = delete;
  explicit GraphicsExportWidget(QWidget* parent = nullptr) noexcept;
  ~GraphicsExportWidget() noexcept;

  // General Methods
  void setShowPageNumbers(bool show) noexcept;
  void setShowResolution(bool show) noexcept;
  void setNumberOfPages(int number) noexcept;
  void setPageContent(int index, const QSize& pageSize, const QRectF margins,
                      std::shared_ptr<QPicture> picture) noexcept;

  // Operator Overloadings
  GraphicsExportWidget& operator=(const GraphicsExportWidget& rhs) = delete;

protected:
  void resizeEvent(QResizeEvent* e) noexcept override;
  void showEvent(QShowEvent* e) noexcept override;

private:  // Methods
  void updateScale() noexcept;
  void updateItemPositions() noexcept;

private:  // Data
  QScopedPointer<QGraphicsView> mView;
  QScopedPointer<QGraphicsScene> mScene;
  QVector<std::shared_ptr<PageItem> > mItems;
  bool mShowPageNumbers;
  bool mShowResolution;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
