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
#include "graphicsoutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(
    const GraphicsOutputJob::Content::Type& obj) {
  if (obj == GraphicsOutputJob::Content::Type::Schematic) {
    return SExpression::createToken("schematic");
  } else if (obj == GraphicsOutputJob::Content::Type::Board) {
    return SExpression::createToken("board");
  } else if (obj == GraphicsOutputJob::Content::Type::AssemblyGuide) {
    return SExpression::createToken("assembly_guide");
  } else {
    throw LogicError(__FILE__, __LINE__);
  }
}

template <>
GraphicsOutputJob::Content::Type deserialize(const SExpression& node) {
  if (node.getValue() == "schematic") {
    return GraphicsOutputJob::Content::Type::Schematic;
  } else if (node.getValue() == "board") {
    return GraphicsOutputJob::Content::Type::Board;
  } else if (node.getValue() == "assembly_guide") {
    return GraphicsOutputJob::Content::Type::AssemblyGuide;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Invalid content type: '%1'").arg(node.getValue()));
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsOutputJob::GraphicsOutputJob() noexcept
  : OutputJob(getTypeName(), Uuid::createRandom(), ElementName("PDF/Image")),
    mDocumentTitle("{{PROJECT}} - {{VERSION}}"),
    mContent(),
    mOutputPath("{{PROJECT}}_{{VERSION}}.pdf") {
}

GraphicsOutputJob::GraphicsOutputJob(const GraphicsOutputJob& other) noexcept
  : OutputJob(other),
    mDocumentTitle(other.mDocumentTitle),
    mContent(other.mContent),
    mOutputPath(other.mOutputPath) {
}

GraphicsOutputJob::GraphicsOutputJob(const SExpression& node)
  : OutputJob(node),
    mDocumentTitle(deserialize<SimpleString>(node.getChild("title/@0"))),
    mContent(),
    mOutputPath(node.getChild("output/@0").getValue()) {
  foreach (const SExpression* contentNode, node.getChildren("content")) {
    Content c(Content::Preset::None);
    c.type = deserialize<Content::Type>(contentNode->getChild("type/@0"));
    c.title = contentNode->getChild("title/@0").getValue();
    const QString pageSize = contentNode->getChild("paper/@0").getValue();
    if (pageSize != "auto") {
      c.pageSizeKey = pageSize;
    }
    foreach (const SExpression* layerNode, contentNode->getChildren("layer")) {
      c.layers.insert(layerNode->getChild("@0").getValue(),
                      deserialize<QColor>(layerNode->getChild("color/@0")));
    }
    foreach (const SExpression* optNode, contentNode->getChildren("option")) {
      c.options[optNode->getChild("@0").getValue()].append(*optNode);
    }
    c.orientation = deserialize<GraphicsExportSettings::Orientation>(
        contentNode->getChild("orientation/@0"));
    c.marginLeft =
        deserialize<UnsignedLength>(contentNode->getChild("margins/left/@0"));
    c.marginTop =
        deserialize<UnsignedLength>(contentNode->getChild("margins/top/@0"));
    c.marginRight =
        deserialize<UnsignedLength>(contentNode->getChild("margins/right/@0"));
    c.marginBottom =
        deserialize<UnsignedLength>(contentNode->getChild("margins/bottom/@0"));
    c.mirror = deserialize<bool>(contentNode->getChild("mirror/@0"));
    c.rotate = deserialize<bool>(contentNode->getChild("rotate/@0"));
    c.scale = deserialize<std::optional<UnsignedRatio>>(
        contentNode->getChild("scale/@0"));
    c.pixmapDpi = deserialize<uint>(contentNode->getChild("dpi/@0"));
    c.monochrome = deserialize<bool>(contentNode->getChild("monochrome/@0"));
    c.backgroundColor =
        deserialize<QColor>(contentNode->getChild("background/@0"));
    c.minLineWidth =
        deserialize<UnsignedLength>(contentNode->getChild("min_line_width/@0"));
    c.boards = BoardSet(*contentNode, "board");
    c.assemblyVariants = AssemblyVariantSet(*contentNode, "variant");
    mContent.append(c);
  }
}

GraphicsOutputJob::~GraphicsOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString GraphicsOutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon GraphicsOutputJob::getTypeIcon() const noexcept {
  if (mOutputPath.toLower().endsWith(".pdf")) {
    return QIcon(":/img/actions/pdf.png");
  } else if (mOutputPath.toLower().endsWith(".svg")) {
    return QIcon(":/img/actions/export_svg.png");
  } else {
    return QIcon(":/img/actions/export_pixmap.png");
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GraphicsOutputJob::setDocumentTitle(const SimpleString& title) noexcept {
  if (title != mDocumentTitle) {
    mDocumentTitle = title;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GraphicsOutputJob::setContent(const QList<Content>& content) noexcept {
  if (content != mContent) {
    mContent = content;
    onEdited.notify(Event::PropertyChanged);
  }
}

void GraphicsOutputJob::setOutputPath(const QString& path) noexcept {
  if (path != mOutputPath) {
    mOutputPath = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> GraphicsOutputJob::cloneShared() const noexcept {
  return std::make_shared<GraphicsOutputJob>(*this);
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

std::shared_ptr<GraphicsOutputJob> GraphicsOutputJob::schematicPdf() noexcept {
  std::shared_ptr<GraphicsOutputJob> obj(new GraphicsOutputJob());
  obj->setName(
      elementNameFromTr("GraphicsOutputJob", QT_TR_NOOP("Schematic PDF")));
  obj->setContent({Content(Content::Preset::Schematic)});
  obj->setOutputPath("{{PROJECT}}_{{VERSION}}_Schematic.pdf");
  return obj;
}

std::shared_ptr<GraphicsOutputJob>
    GraphicsOutputJob::boardAssemblyPdf() noexcept {
  std::shared_ptr<GraphicsOutputJob> obj(new GraphicsOutputJob());
  obj->setName(
      elementNameFromTr("GraphicsOutputJob", QT_TR_NOOP("Board Assembly PDF")));
  obj->setContent({Content(Content::Preset::BoardAssemblyTop),
                   Content(Content::Preset::BoardAssemblyBottom)});
  obj->setOutputPath("{{PROJECT}}_{{VERSION}}_Assembly.pdf");
  return obj;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsOutputJob::serializeDerived(SExpression& root) const {
  root.appendChild("title", *mDocumentTitle);
  root.ensureLineBreak();
  foreach (const Content& content, mContent) {
    SExpression& node = root.appendList("content");
    node.appendChild("type", content.type);
    node.appendChild("title", content.title);
    node.ensureLineBreak();
    node.appendChild("paper",
                     content.pageSizeKey
                         ? SExpression::createString(*content.pageSizeKey)
                         : SExpression::createToken("auto"));
    node.appendChild("orientation", content.orientation);
    node.appendChild("rotate", content.rotate);
    node.appendChild("mirror", content.mirror);
    node.appendChild("scale", content.scale);
    node.ensureLineBreak();
    SExpression& marginsNode = node.appendList("margins");
    marginsNode.appendChild("left", content.marginLeft);
    marginsNode.appendChild("top", content.marginTop);
    marginsNode.appendChild("right", content.marginRight);
    marginsNode.appendChild("bottom", content.marginBottom);
    node.ensureLineBreak();
    node.appendChild("dpi", content.pixmapDpi);
    node.appendChild("min_line_width", content.minLineWidth);
    node.appendChild("monochrome", content.monochrome);
    node.appendChild("background", content.backgroundColor);
    node.ensureLineBreak();
    for (auto it = content.layers.begin(); it != content.layers.end(); ++it) {
      SExpression& layerNode = node.appendList("layer");
      layerNode.appendChild(SExpression::createToken(it.key()));
      layerNode.appendChild("color", it.value());
      node.ensureLineBreak();
    }
    content.boards.serialize(node, "board");
    node.ensureLineBreak();
    content.assemblyVariants.serialize(node, "variant");
    foreach (const auto& optionList, content.options) {
      foreach (const auto& optionNode, optionList) {
        node.ensureLineBreak();
        node.appendChild(optionNode);
      }
    }
    node.ensureLineBreak();
    root.ensureLineBreak();
  }
  root.appendChild("output", mOutputPath);
}

bool GraphicsOutputJob::equals(const OutputJob& rhs) const noexcept {
  const GraphicsOutputJob& other = static_cast<const GraphicsOutputJob&>(rhs);
  if (mDocumentTitle != other.mDocumentTitle) return false;
  if (mContent != other.mContent) return false;
  if (mOutputPath != other.mOutputPath) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
