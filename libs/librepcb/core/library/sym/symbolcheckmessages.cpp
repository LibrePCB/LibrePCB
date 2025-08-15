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
#include "symbolcheckmessages.h"

#include "../../geometry/text.h"
#include "../../types/layer.h"
#include "../../utils/toolbox.h"
#include "symbolpin.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  MsgDuplicatePinName
 ******************************************************************************/

MsgDuplicatePinName::MsgDuplicatePinName(const SymbolPin& pin) noexcept
  : RuleCheckMessage(
        Severity::Error, tr("Duplicate pin name: '%1'").arg(*pin.getName()),
        tr("All symbol pins must have unique names, otherwise they cannot be "
           "distinguished later in the component editor. If your part has "
           "several pins with same functionality (e.g. multiple GND pins), you "
           "should add only one of these pins to the symbol. The assignment to "
           "multiple leads should be done in the device editor instead."),
        "duplicate_pin_name") {
  mApproval->appendChild("name", *pin.getName());
}

/*******************************************************************************
 *  MsgInvalidImageFile
 ******************************************************************************/

MsgInvalidImageFile::MsgInvalidImageFile(const QString& fileName, Error error,
                                         const QString& details) noexcept
  : RuleCheckMessage(Severity::Error, buildMessagePattern(error).arg(fileName),
                     buildDescription(details), "invalid_image_file") {
  mApproval->appendChild("file", fileName);
}

QString MsgInvalidImageFile::buildMessagePattern(Error error) noexcept {
  QHash<Error, QString> translations = {
      {Error::FileMissing, tr("Missing image file: '%1'")},
      {Error::FileReadError, QString("Failed to read image file: '%1'")},
      {Error::UnsupportedFormat, tr("Unsupported image format: '%1'")},
      {Error::ImageLoadError, tr("Invalid image file: '%1'")},
  };
  return translations.value(error, "Unknown image error: %1");
}

QString MsgInvalidImageFile::buildDescription(const QString& details) noexcept {
  QString s =
      tr("The referenced file of an image does either not exist in the symbol "
         "or is not a valid image file. Try removing and re-adding the image "
         "from the symbol.");
  if (!details.isEmpty()) {
    s += "\n\n" % tr("Details:") % " " % details;
  }
  return s;
}

/*******************************************************************************
 *  MsgMissingSymbolName
 ******************************************************************************/

MsgMissingSymbolName::MsgMissingSymbolName() noexcept
  : RuleCheckMessage(
        Severity::Warning, tr("Missing text: '%1'").arg("{{NAME}}"),
        tr("Most symbols should have a text element for the component's name, "
           "otherwise you won't see that name in the schematics. There are "
           "only a few exceptions (e.g. a schematic frame) which don't need a "
           "name, for those you can ignore this message."),
        "missing_name_text") {
}

/*******************************************************************************
 *  MsgMissingSymbolValue
 ******************************************************************************/

MsgMissingSymbolValue::MsgMissingSymbolValue() noexcept
  : RuleCheckMessage(
        Severity::Warning, tr("Missing text: '%1'").arg("{{VALUE}}"),
        tr("Most symbols should have a text element for the component's value, "
           "otherwise you won't see that value in the schematics. There are "
           "only a few exceptions (e.g. a schematic frame) which don't need a "
           "value, for those you can ignore this message."),
        "missing_value_text") {
}

/*******************************************************************************
 *  MsgNonFunctionalSymbolPinInversionSign
 ******************************************************************************/

MsgNonFunctionalSymbolPinInversionSign::MsgNonFunctionalSymbolPinInversionSign(
    std::shared_ptr<const SymbolPin> pin) noexcept
  : RuleCheckMessage(
        Severity::Hint,
        tr("Non-functional inversion sign: '%1'").arg(*pin->getName()),
        tr("The pin name seems to start with an inversion sign, but LibrePCB "
           "uses a different sign to indicate inversion.\n\nIt's recommended "
           "to prefix inverted pin names with '%1', regardless of the "
           "inversion sign used in the parts datasheet.")
            .arg("!"),
        "nonfunctional_inversion_sign"),
    mPin(pin) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("pin", pin->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgSymbolOriginNotInCenter
 ******************************************************************************/

MsgSymbolOriginNotInCenter::MsgSymbolOriginNotInCenter(
    const Point& center) noexcept
  : RuleCheckMessage(
        Severity::Hint, tr("Origin not in center"),
        tr("Generally the origin (0, 0) should be in the center of the symbol "
           "body (roughly, mapped to grid). It's not recommended to have it at "
           "pin-1 coordinate, top-left or something like that.\n\nIt looks "
           "like this rule is not followed in this symbol. However, for "
           "irregular symbol shapes this warning may not be justified. In such "
           "cases, just approve it."),
        "origin_not_in_center"),
    mCenter(center) {
}

/*******************************************************************************
 *  MsgOverlappingSymbolPins
 ******************************************************************************/

MsgOverlappingSymbolPins::MsgOverlappingSymbolPins(
    QVector<std::shared_ptr<const SymbolPin>> pins) noexcept
  : RuleCheckMessage(
        Severity::Error, buildMessage(pins),
        tr("There are multiple pins at the same position. This is not allowed "
           "because you cannot connect wires to these pins in the schematic "
           "editor."),
        "overlapping_pins"),
    mPins(pins) {
  QVector<std::shared_ptr<const SymbolPin>> sortedPins = pins;
  std::sort(sortedPins.begin(), sortedPins.end(),
            [](const std::shared_ptr<const SymbolPin>& a,
               const std::shared_ptr<const SymbolPin>& b) {
              return a->getUuid() < b->getUuid();
            });
  foreach (const auto& pin, pins) {
    mApproval->ensureLineBreak();
    mApproval->appendChild("pin", pin->getUuid());
  }
  mApproval->ensureLineBreak();
}

QString MsgOverlappingSymbolPins::buildMessage(
    const QVector<std::shared_ptr<const SymbolPin>>& pins) noexcept {
  QStringList pinNames;
  foreach (const auto& pin, pins) {
    pinNames.append("'" % pin->getName() % "'");
  }
  Toolbox::sortNumeric(pinNames, Qt::CaseInsensitive, false);
  return tr("Overlapping pins: %1").arg(pinNames.join(", "));
}

/*******************************************************************************
 *  MsgSymbolPinNotOnGrid
 ******************************************************************************/

MsgSymbolPinNotOnGrid::MsgSymbolPinNotOnGrid(
    std::shared_ptr<const SymbolPin> pin,
    const PositiveLength& gridInterval) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Pin not on %1mm grid: '%2'")
            .arg(gridInterval->toMmString(), *pin->getName()),
        tr("Every pin must be placed exactly on the %1mm grid, "
           "otherwise it cannot be connected in the schematic editor.")
            .arg(gridInterval->toMmString()),
        "pin_not_on_grid"),
    mPin(pin),
    mGridInterval(gridInterval) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("pin", pin->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  MsgWrongSymbolTextLayer
 ******************************************************************************/

MsgWrongSymbolTextLayer::MsgWrongSymbolTextLayer(
    std::shared_ptr<const Text> text, const Layer& expectedLayer) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Layer of '%1' is not '%2'")
            .arg(text->getText(), expectedLayer.getNameTr()),
        tr("The text element '%1' should normally be on layer '%2'.")
            .arg(text->getText(), expectedLayer.getNameTr()),
        "unusual_text_layer"),
    mText(text),
    mExpectedLayer(&expectedLayer) {
  mApproval->ensureLineBreak();
  mApproval->appendChild("text", text->getUuid());
  mApproval->ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
