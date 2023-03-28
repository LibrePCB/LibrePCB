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
#include "pickplacecsvwriter.h"

#include "../application.h"
#include "../fileio/csvfile.h"
#include "pickplacedata.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PickPlaceCsvWriter::PickPlaceCsvWriter(const PickPlaceData& data) noexcept
  : mData(data), mBoardSide(BoardSide::BOTH), mIncludeMetadataComment(true) {
}

PickPlaceCsvWriter::~PickPlaceCsvWriter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<CsvFile> PickPlaceCsvWriter::generateCsv() const {
  // Names for all mount types.
  static QVector<PickPlaceDataItem::Type> types = {
      PickPlaceDataItem::Type::Tht,   PickPlaceDataItem::Type::Smt,
      PickPlaceDataItem::Type::Mixed, PickPlaceDataItem::Type::Fiducial,
      PickPlaceDataItem::Type::Other,
  };
  static QStringList typeNames = {
      "THT", "SMT", "THT+SMT", "Fiducial", "Other",
  };
  auto getTypeName = [](PickPlaceDataItem::Type type) {
    return typeNames.value(types.indexOf(type), "Other");
  };

  std::shared_ptr<CsvFile> file(new CsvFile());

  // Optionally add some metadata to to the CSV as a help for readers.
  if (mIncludeMetadataComment) {
    QString comment =
        QString(
            "Pick&Place Position Data File\n"
            "\n"
            "Project Name:        %1\n"
            "Project Version:     %2\n"
            "Board Name:          %3\n"
            "Generation Software: LibrePCB %4\n"
            "Generation Date:     %5\n"
            "Unit:                mm\n"
            "Rotation:            Degrees CCW\n"
            "Board Side:          %6\n"
            "Supported Types:     %7")
            .arg(mData.getProjectName())
            .arg(mData.getProjectVersion())
            .arg(mData.getBoardName())
            .arg(Application::getVersion())
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
            .arg(boardSideToString(mBoardSide))
            .arg(typeNames.join(", "));
    file->setComment(comment);
  }

  // Don't translate the CSV header to make pick&place files independent of the
  // user's language.
  file->setHeader({"Designator", "Value", "Device", "Package", "Position X",
                   "Position Y", "Rotation", "Side", "Type"});

  foreach (const PickPlaceDataItem& item, mData.getItems()) {
    if (isOnBoardSide(item, mBoardSide)) {
      QStringList values;
      values += item.getDesignator();
      values += item.getValue();
      values += item.getDeviceName();
      values += item.getPackageName();
      values += item.getPosition().getX().toMmString();
      values += item.getPosition().getY().toMmString();
      values += item.getRotation().mappedTo0_360deg().toDegString();
      values += item.getBoardSide() == PickPlaceDataItem::BoardSide::Top
          ? "Top"
          : "Bottom";
      values += getTypeName(item.getType());
      file->addValue(values);  // can throw
    }
  }

  return file;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PickPlaceCsvWriter::isOnBoardSide(const PickPlaceDataItem& item,
                                       BoardSide side) noexcept {
  switch (side) {
    case BoardSide::TOP:
      return (item.getBoardSide() == PickPlaceDataItem::BoardSide::Top);
    case BoardSide::BOTTOM:
      return (item.getBoardSide() == PickPlaceDataItem::BoardSide::Bottom);
    default:
      return true;
  }
}

QString PickPlaceCsvWriter::boardSideToString(BoardSide side) noexcept {
  switch (side) {
    case BoardSide::TOP:
      return "Top";
    case BoardSide::BOTTOM:
      return "Bottom";
    case BoardSide::BOTH:
      return "Top + Bottom";
    default:
      return "Unknown";
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
