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

#ifndef LIBREPCB_EDITOR_SCHEMATICSELECTIONQUERY_H
#define LIBREPCB_EDITOR_SCHEMATICSELECTIONQUERY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SI_NetLabel;
class SI_NetLine;
class SI_NetPoint;
class SI_NetSegment;
class SI_Polygon;
class SI_Symbol;
class SI_SymbolPin;
class SI_Text;
class Uuid;

namespace editor {

class SchematicGraphicsScene;

/*******************************************************************************
 *  Class SchematicSelectionQuery
 ******************************************************************************/

/**
 * @brief The SchematicSelectionQuery class
 */
class SchematicSelectionQuery final : public QObject {
  Q_OBJECT

public:
  // Types
  struct NetSegmentItems {
    QSet<SI_NetPoint*> netpoints;
    QSet<SI_NetLine*> netlines;
    QSet<SI_NetLabel*> netlabels;
  };

  // Constructors / Destructor
  SchematicSelectionQuery() = delete;
  SchematicSelectionQuery(const SchematicSelectionQuery& other) = delete;
  explicit SchematicSelectionQuery(SchematicGraphicsScene& scene,
                                   QObject* parent = nullptr);
  ~SchematicSelectionQuery() noexcept;

  // Getters
  const QSet<SI_Symbol*>& getSymbols() const noexcept { return mResultSymbols; }
  const QSet<SI_NetPoint*>& getNetPoints() const noexcept {
    return mResultNetPoints;
  }
  const QSet<SI_NetLine*>& getNetLines() const noexcept {
    return mResultNetLines;
  }
  const QSet<SI_NetLabel*>& getNetLabels() const noexcept {
    return mResultNetLabels;
  }
  const QSet<SI_Polygon*>& getPolygons() const noexcept {
    return mResultPolygons;
  }
  const QSet<SI_Text*>& getTexts() const noexcept { return mResultTexts; }

  /**
   * @brief Get net points, net lines and net labels grouped by net segment
   *
   * Same as #getNetPoints(), #getNetLines() and #getNetLabels(), but grouped
   * by their corresponding net segments. Only net segments containing selected
   * items are returned.
   *
   * @return List of net segments containing the selected items
   */
  QHash<SI_NetSegment*, NetSegmentItems> getNetSegmentItems() const noexcept;
  int getResultCount() const noexcept;
  bool isResultEmpty() const noexcept { return (getResultCount() == 0); }

  // General Methods
  void addSelectedSymbols() noexcept;
  void addSelectedNetPoints() noexcept;
  void addSelectedNetLines() noexcept;
  void addSelectedNetLabels() noexcept;
  void addSelectedPolygons() noexcept;
  void addSelectedSchematicTexts() noexcept;
  void addSelectedSymbolTexts() noexcept;
  /**
   * @brief Add net points of the selected net lines
   *
   * @param onlyIfAllNetLinesSelected   If true, net points are added only if
   *                                    *all* connected net lines are selected.
   *                                    If false, net points are added if at
   *                                    least one of the connected net lines
   *                                    is selected.
   */
  void addNetPointsOfNetLines(bool onlyIfAllNetLinesSelected = false) noexcept;
  void addNetLinesOfSymbolPins() noexcept;

  // Operator Overloadings
  SchematicSelectionQuery& operator=(const SchematicSelectionQuery& rhs) =
      delete;

private:  // Data
  SchematicGraphicsScene& mScene;

  // query result
  QSet<SI_Symbol*> mResultSymbols;
  QSet<SI_NetPoint*> mResultNetPoints;
  QSet<SI_NetLine*> mResultNetLines;
  QSet<SI_NetLabel*> mResultNetLabels;
  QSet<SI_Polygon*> mResultPolygons;
  QSet<SI_Text*> mResultTexts;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
