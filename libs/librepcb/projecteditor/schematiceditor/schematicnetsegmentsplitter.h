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

#ifndef LIBREPCB_PROJECT_EDITOR_SCHEMATICNETSEGMENTSPLITTER_H
#define LIBREPCB_PROJECT_EDITOR_SCHEMATICNETSEGMENTSPLITTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class SI_NetSegment;
class SI_NetPoint;
class SI_NetLine;
class SI_NetLabel;
class SI_NetLineAnchor;

namespace editor {

/*******************************************************************************
 *  Class SchematicNetSegmentSplitter
 ******************************************************************************/

/**
 * @brief The SchematicNetSegmentSplitter class
 */
class SchematicNetSegmentSplitter final {
public:
  // Types
  struct Segment {
    QList<SI_NetLineAnchor*> anchors;
    QList<SI_NetLine*>       netlines;
    QList<SI_NetLabel*>      netlabels;
  };

  // Constructors / Destructor
  SchematicNetSegmentSplitter() noexcept;
  SchematicNetSegmentSplitter(const SchematicNetSegmentSplitter& other) =
      delete;
  ~SchematicNetSegmentSplitter() noexcept;

  // General Methods
  void addNetLine(SI_NetLine* netline) noexcept {
    Q_ASSERT(!mNetLines.contains(netline));
    mNetLines.append(netline);
  }
  void addNetLabel(SI_NetLabel* netlabel) noexcept {
    Q_ASSERT(!mNetLabels.contains(netlabel));
    mNetLabels.append(netlabel);
  }
  QList<Segment> split() const noexcept;

  // Operator Overloadings
  SchematicNetSegmentSplitter& operator       =(
      const SchematicNetSegmentSplitter& rhs) = delete;

private:  // Methods
  void findConnectedLinesAndPoints(SI_NetLineAnchor&         anchor,
                                   QList<SI_NetLineAnchor*>& processedAnchors,
                                   QList<SI_NetLineAnchor*>& anchors,
                                   QList<SI_NetLine*>&       netlines,
                                   QList<SI_NetLine*>& availableNetLines) const
      noexcept;
  int getNearestNetSegmentOfNetLabel(const SI_NetLabel&    netlabel,
                                     const QList<Segment>& segments) const
      noexcept;
  Length getDistanceBetweenNetLabelAndNetSegment(
      const SI_NetLabel& netlabel, const Segment& netsegment) const noexcept;

private:  // Data
  QList<SI_NetLine*>  mNetLines;
  QList<SI_NetLabel*> mNetLabels;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
