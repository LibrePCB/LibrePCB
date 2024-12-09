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

#ifndef LIBREPCB_CORE_GRAPHICSOUTPUTJOB_H
#define LIBREPCB_CORE_GRAPHICSOUTPUTJOB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../export/graphicsexportsettings.h"
#include "../types/simplestring.h"
#include "../workspace/theme.h"
#include "outputjob.h"

#include <QtCore>
#include <QtGui>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class GraphicsOutputJob
 ******************************************************************************/

/**
 * @brief PDF/Image output job
 */
class GraphicsOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(GraphicsOutputJob)

public:
  using BoardSet = ObjectSet<std::optional<Uuid>>;
  using AssemblyVariantSet = ObjectSet<std::optional<Uuid>>;

  struct Content {
    // Types
    enum class Preset {
      None,
      Schematic,
      BoardImage,
      BoardAssemblyTop,
      BoardAssemblyBottom,
    };
    enum class Type {
      Schematic,
      Board,
      AssemblyGuide,  // Reserved for future use.
    };

    // Common options.
    Type type;
    QString title;
    std::optional<QString> pageSizeKey;  // nullopt = auto
    GraphicsExportSettings::Orientation orientation;
    UnsignedLength marginLeft;
    UnsignedLength marginTop;
    UnsignedLength marginRight;
    UnsignedLength marginBottom;
    bool rotate;
    bool mirror;
    std::optional<UnsignedRatio> scale;
    uint pixmapDpi;
    bool monochrome;
    QColor backgroundColor;
    UnsignedLength minLineWidth;
    QMap<QString, QColor> layers;
    BoardSet boards;
    AssemblyVariantSet assemblyVariants;

    // Arbitrary options for forward compatibility in case we really need to
    // add new settings in a minor release. Supported options:
    //  - realistic: If present, render boards in realistic mode
    QMap<QString, QList<SExpression>> options;

    Content(Preset preset) noexcept
      : type(Type::Schematic),
        title(),
        pageSizeKey(std::nullopt),
        orientation(GraphicsExportSettings::Orientation::Auto),
        marginLeft(10000000),  // 10mm
        marginTop(10000000),  // 10mm
        marginRight(10000000),  // 10mm
        marginBottom(10000000),  // 10mm
        rotate(false),
        mirror(false),
        scale(std::nullopt),  // Fit in page
        pixmapDpi(600),
        monochrome(false),
        backgroundColor(Qt::transparent),
        minLineWidth(100000),
        layers(),
        boards(BoardSet::set({std::nullopt})),
        assemblyVariants(AssemblyVariantSet::set({std::nullopt})),
        options() {
      QSet<QString> enabledLayers;
      if (preset == Preset::Schematic) {
        type = Type::Schematic;
        title = tr("Schematic");
        boards = BoardSet::set({std::nullopt});
        assemblyVariants = AssemblyVariantSet::set({std::nullopt});
        enabledLayers = {
            QString(Theme::Color::sSchematicFrames),
            QString(Theme::Color::sSchematicWires),
            QString(Theme::Color::sSchematicNetLabels),
            QString(Theme::Color::sSchematicDocumentation),
            QString(Theme::Color::sSchematicComments),
            QString(Theme::Color::sSchematicGuide),
            QString(Theme::Color::sSchematicOutlines),
            QString(Theme::Color::sSchematicGrabAreas),
            QString(Theme::Color::sSchematicNames),
            QString(Theme::Color::sSchematicValues),
            QString(Theme::Color::sSchematicPinLines),
            QString(Theme::Color::sSchematicPinNames),
            QString(Theme::Color::sSchematicPinNumbers),
        };
      } else if (preset != Preset::None) {
        type = Type::Board;
        if (preset == Preset::BoardAssemblyTop) {
          title = tr("Assembly Top");
          mirror = false;
        } else if (preset == Preset::BoardAssemblyBottom) {
          title = tr("Assembly Bottom");
          mirror = true;
        } else {
          title = tr("Board");
          mirror = false;
        }
        boards = BoardSet::onlyDefault();
        assemblyVariants = AssemblyVariantSet::set({std::nullopt});
        enabledLayers = {
            QString(Theme::Color::sBoardFrames),
            QString(Theme::Color::sBoardOutlines),
            QString(Theme::Color::sBoardPlatedCutouts),
            QString(Theme::Color::sBoardHoles),
            QString(Theme::Color::sBoardPads),
            QString(Theme::Color::sBoardMeasures),
            QString(Theme::Color::sBoardDocumentation),
            QString(Theme::Color::sBoardComments),
            QString(Theme::Color::sBoardGuide),
        };
        if (preset != Preset::BoardAssemblyBottom) {
          enabledLayers += {
              QString(Theme::Color::sBoardLegendTop),
              QString(Theme::Color::sBoardDocumentationTop),
              QString(Theme::Color::sBoardGrabAreasTop),
              QString(Theme::Color::sBoardNamesTop),
              QString(Theme::Color::sBoardValuesTop),
          };
        }
        if (preset != Preset::BoardAssemblyTop) {
          enabledLayers += {
              QString(Theme::Color::sBoardLegendBot),
              QString(Theme::Color::sBoardDocumentationBot),
              QString(Theme::Color::sBoardGrabAreasBot),
              QString(Theme::Color::sBoardNamesBot),
              QString(Theme::Color::sBoardValuesBot),
          };
        }
        if (preset == Preset::BoardImage) {
          enabledLayers += {
              QString(Theme::Color::sBoardVias),
              QString(Theme::Color::sBoardCopperTop),
              QString(Theme::Color::sBoardCopperBot),
          };
        }
      }
      GraphicsExportSettings defaultSettings;
      foreach (const auto& pair, defaultSettings.getColors()) {
        if (enabledLayers.contains(pair.first)) {
          layers.insert(pair.first, pair.second);
        }
      }
    }
    Content(const Content& other) = default;
    Content& operator=(const Content& rhs) = default;
    bool operator==(const Content& rhs) const noexcept {
      return (type == rhs.type)  //
          && (title == rhs.title)  //
          && (pageSizeKey == rhs.pageSizeKey)  // break
          && (orientation == rhs.orientation)  // break
          && (marginLeft == rhs.marginLeft)  // break
          && (marginTop == rhs.marginTop)  // break
          && (marginRight == rhs.marginRight)  // break
          && (marginBottom == rhs.marginBottom)  // break
          && (rotate == rhs.rotate)  // break
          && (mirror == rhs.mirror)  // break
          && (scale == rhs.scale)  // break
          && (pixmapDpi == rhs.pixmapDpi)  // break
          && (monochrome == rhs.monochrome)  // break
          && (backgroundColor == rhs.backgroundColor)  // break
          && (minLineWidth == rhs.minLineWidth)  // break
          && (layers == rhs.layers)  // break
          && (boards == rhs.boards)  // break
          && (assemblyVariants == rhs.assemblyVariants)  // break
          && (options == rhs.options)  // break
          ;
    }
    bool operator!=(const Content& rhs) const noexcept {
      return !(*this == rhs);
    }
  };

  // Constructors / Destructor
  GraphicsOutputJob(const GraphicsOutputJob& other) noexcept;
  explicit GraphicsOutputJob(const SExpression& node);
  virtual ~GraphicsOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  const SimpleString& getDocumentTitle() const noexcept {
    return mDocumentTitle;
  }
  const QList<Content> getContent() const noexcept { return mContent; }
  const QString& getOutputPath() const noexcept { return mOutputPath; }

  // Setters
  void setDocumentTitle(const SimpleString& title) noexcept;
  void setContent(const QList<Content>& content) noexcept;
  void setOutputPath(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "graphics"; }
  static QString getTypeTrStatic() noexcept { return tr("PDF/Image"); }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  GraphicsOutputJob& operator=(const GraphicsOutputJob& rhs) = delete;

  // Static Methods
  static std::shared_ptr<GraphicsOutputJob> schematicPdf() noexcept;
  static std::shared_ptr<GraphicsOutputJob> boardAssemblyPdf() noexcept;

private:  // Methods
  GraphicsOutputJob() noexcept;
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  SimpleString mDocumentTitle;
  QList<Content> mContent;
  QString mOutputPath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
