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

#ifndef LIBREPCB_PROJECT_SCHEMATIC_H
#define LIBREPCB_PROJECT_SCHEMATIC_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/attributes/attributeprovider.h>
#include <librepcb/common/elementname.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/fileio/transactionaldirectory.h>
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/uuid.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GridProperties;
class GraphicsView;
class GraphicsScene;

namespace project {

class Project;
class NetSignal;
class ComponentInstance;
class SI_Base;
class SI_Symbol;
class SI_SymbolPin;
class SI_NetSegment;
class SI_NetPoint;
class SI_NetLine;
class SI_NetLabel;
class SchematicSelectionQuery;

/*******************************************************************************
 *  Class Schematic
 ******************************************************************************/

/**
 * @brief The Schematic class represents one schematic page of a project and is
 * always part of a circuit
 *
 * A schematic can contain following items (see ::librepcb::project::SI_Base and
 * ::librepcb::project::SGI_Base):
 *  - netsegment:       ::librepcb::project::SI_NetSegment
 *      - netpoint:     ::librepcb::project::SI_NetPoint
 *      - netline:      ::librepcb::project::SI_NetLine
 *      - netlabel:     ::librepcb::project::SI_NetLabel
 *  - symbol:           ::librepcb::project::SI_Symbol
 *      - symbol pin:   ::librepcb::project::SI_SymbolPin
 *  - polygon:          TODO
 *  - circle:           TODO
 *  - text:             TODO
 */
class Schematic final : public QObject,
                        public AttributeProvider,
                        public SerializableObject {
  Q_OBJECT

public:
  // Types

  /**
   * @brief Z Values of all items in a schematic scene (to define the stacking
   * order)
   *
   * These values are used for QGraphicsItem::setZValue() to define the stacking
   * order of all items in a schematic QGraphicsScene. We use integer values,
   * even if the z-value of QGraphicsItem is a qreal attribute...
   *
   * Low number = background, high number = foreground
   */
  enum ItemZValue {
    ZValue_Default = 0,  ///< this is the default value (behind all other items)
    ZValue_Symbols,      ///< Z value for ::librepcb::project::SI_Symbol items
    ZValue_NetLabels,    ///< Z value for ::librepcb::project::SI_NetLabel items
    ZValue_NetLines,     ///< Z value for ::librepcb::project::SI_NetLine items
    ZValue_HiddenNetPoints,   ///< Z value for hidden
                              ///< ::librepcb::project::SI_NetPoint items
    ZValue_VisibleNetPoints,  ///< Z value for visible
                              ///< ::librepcb::project::SI_NetPoint items
  };

  // Constructors / Destructor
  Schematic()                       = delete;
  Schematic(const Schematic& other) = delete;
  Schematic(Project& project, std::unique_ptr<TransactionalDirectory> directory)
    : Schematic(project, std::move(directory), false, QString()) {}
  ~Schematic() noexcept;

  // Getters: General
  Project&              getProject() const noexcept { return mProject; }
  FilePath              getFilePath() const noexcept;
  const GridProperties& getGridProperties() const noexcept {
    return *mGridProperties;
  }
  GraphicsScene&  getGraphicsScene() const noexcept { return *mGraphicsScene; }
  bool            isEmpty() const noexcept;
  QList<SI_Base*> getItemsAtScenePos(const Point& pos) const noexcept;
  QList<SI_NetPoint*>  getNetPointsAtScenePos(const Point& pos) const noexcept;
  QList<SI_NetLine*>   getNetLinesAtScenePos(const Point& pos) const noexcept;
  QList<SI_NetLabel*>  getNetLabelsAtScenePos(const Point& pos) const noexcept;
  QList<SI_SymbolPin*> getPinsAtScenePos(const Point& pos) const noexcept;

  // Setters: General
  void setGridProperties(const GridProperties& grid) noexcept;

  // Getters: Attributes
  const Uuid&        getUuid() const noexcept { return mUuid; }
  const ElementName& getName() const noexcept { return mName; }
  const QIcon&       getIcon() const noexcept { return mIcon; }

  // Symbol Methods
  QList<SI_Symbol*> getSymbols() const noexcept { return mSymbols; }
  SI_Symbol*        getSymbolByUuid(const Uuid& uuid) const noexcept;
  void              addSymbol(SI_Symbol& symbol);
  void              removeSymbol(SI_Symbol& symbol);

  // NetSegment Methods
  SI_NetSegment* getNetSegmentByUuid(const Uuid& uuid) const noexcept;
  void           addNetSegment(SI_NetSegment& netsegment);
  void           removeNetSegment(SI_NetSegment& netsegment);

  // General Methods
  void addToProject();
  void removeFromProject();
  void save();
  void showInView(GraphicsView& view) noexcept;
  void saveViewSceneRect(const QRectF& rect) noexcept { mViewRect = rect; }
  const QRectF& restoreViewSceneRect() const noexcept { return mViewRect; }
  void          setSelectionRect(const Point& p1, const Point& p2,
                                 bool updateItems) noexcept;
  void          clearSelection() const noexcept;
  void          updateAllNetLabelAnchors() noexcept;
  void          renderToQPainter(QPainter& painter) const noexcept;
  std::unique_ptr<SchematicSelectionQuery> createSelectionQuery() const
      noexcept;

  // Inherited from AttributeProvider
  /// @copydoc librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;
  /// @copydoc librepcb::AttributeProvider::getAttributeProviderParents()
  QVector<const AttributeProvider*> getAttributeProviderParents() const
      noexcept override;

  // Operator Overloadings
  Schematic& operator=(const Schematic& rhs) = delete;
  bool operator==(const Schematic& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const Schematic& rhs) noexcept { return (this != &rhs); }

  // Static Methods
  static Schematic* create(Project&                                project,
                           std::unique_ptr<TransactionalDirectory> directory,
                           const ElementName&                      name);

signals:

  /// @copydoc AttributeProvider::attributesChanged()
  void attributesChanged() override;

private:
  Schematic(Project& project, std::unique_ptr<TransactionalDirectory> directory,
            bool create, const QString& newName);
  void updateIcon() noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // General
  Project& mProject;  ///< A reference to the Project object (from the ctor)
  std::unique_ptr<TransactionalDirectory> mDirectory;
  bool                                    mIsAddedToProject;

  QScopedPointer<GraphicsScene>  mGraphicsScene;
  QScopedPointer<GridProperties> mGridProperties;
  QRectF                         mViewRect;

  // Attributes
  Uuid        mUuid;
  ElementName mName;
  QIcon       mIcon;

  QList<SI_Symbol*>     mSymbols;
  QList<SI_NetSegment*> mNetSegments;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_SCHEMATIC_H
