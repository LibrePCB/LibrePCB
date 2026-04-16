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
#include "../workspace/colorrole.h"
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
      BoardRenderingTop,
      BoardRenderingBottom,
    };
    enum class Type {
      Schematic,
      Board,
      BoardRendering,
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
    //  - none
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
      QSet<QString> enabledColors;
      if (preset == Preset::Schematic) {
        type = Type::Schematic;
        title = tr("Schematic");
        boards = BoardSet::set({std::nullopt});
        assemblyVariants = AssemblyVariantSet::set({std::nullopt});
        enabledColors = {
            ColorRole::schematicFrames().getId(),
            ColorRole::schematicWires().getId(),
            ColorRole::schematicNetLabels().getId(),
            ColorRole::schematicBuses().getId(),
            ColorRole::schematicBusLabels().getId(),
            ColorRole::schematicImageBorders().getId(),
            ColorRole::schematicDocumentation().getId(),
            ColorRole::schematicComments().getId(),
            ColorRole::schematicGuide().getId(),
            ColorRole::schematicOutlines().getId(),
            ColorRole::schematicGrabAreas().getId(),
            ColorRole::schematicNames().getId(),
            ColorRole::schematicValues().getId(),
            ColorRole::schematicPinLines().getId(),
            ColorRole::schematicPinNames().getId(),
            ColorRole::schematicPinNumbers().getId(),
        };
      } else if (preset != Preset::None) {
        if (preset == Preset::BoardAssemblyTop) {
          type = Type::Board;
          title = tr("Assembly Top");
          mirror = false;
        } else if (preset == Preset::BoardAssemblyBottom) {
          type = Type::Board;
          title = tr("Assembly Bottom");
          mirror = true;
        } else if (preset == Preset::BoardRenderingTop) {
          type = Type::BoardRendering;
          title = tr("Rendering Top");
          mirror = false;
        } else if (preset == Preset::BoardRenderingBottom) {
          type = Type::BoardRendering;
          title = tr("Rendering Bottom");
          mirror = true;
        } else {
          type = Type::Board;
          title = tr("Board");
          mirror = false;
        }
        boards = BoardSet::onlyDefault();
        assemblyVariants = AssemblyVariantSet::set({std::nullopt});
        if (preset == Preset::BoardRenderingTop) {
          enabledColors = {
              ColorRole::boardOutlines().getId(),
              ColorRole::boardCopperTop().getId(),
              ColorRole::boardStopMaskTop().getId(),
              ColorRole::boardLegendTop().getId(),
          };
        } else if (preset == Preset::BoardRenderingBottom) {
          enabledColors = {
              ColorRole::boardOutlines().getId(),
              ColorRole::boardCopperBot().getId(),
              ColorRole::boardStopMaskBot().getId(),
              ColorRole::boardLegendBot().getId(),
          };
        } else {
          enabledColors = {
              ColorRole::boardFrames().getId(),
              ColorRole::boardOutlines().getId(),
              ColorRole::boardPlatedCutouts().getId(),
              ColorRole::boardHoles().getId(),
              ColorRole::boardPads().getId(),
              ColorRole::boardMeasures().getId(),
              ColorRole::boardDocumentation().getId(),
              ColorRole::boardComments().getId(),
              ColorRole::boardGuide().getId(),
          };
          if (preset != Preset::BoardAssemblyBottom) {
            enabledColors += {
                ColorRole::boardLegendTop().getId(),
                ColorRole::boardDocumentationTop().getId(),
                ColorRole::boardGrabAreasTop().getId(),
                ColorRole::boardNamesTop().getId(),
                ColorRole::boardValuesTop().getId(),
            };
          }
          if (preset != Preset::BoardAssemblyTop) {
            enabledColors += {
                ColorRole::boardLegendBot().getId(),
                ColorRole::boardDocumentationBot().getId(),
                ColorRole::boardGrabAreasBot().getId(),
                ColorRole::boardNamesBot().getId(),
                ColorRole::boardValuesBot().getId(),
            };
          }
          if (preset == Preset::BoardImage) {
            enabledColors += {
                ColorRole::boardVias().getId(),
                ColorRole::boardCopperTop().getId(),
                ColorRole::boardCopperBot().getId(),
            };
          }
        }
      }
      GraphicsExportSettings defaultSettings;
      if ((preset == Preset::BoardRenderingTop) ||
          (preset == Preset::BoardRenderingBottom)) {
        defaultSettings.loadBoardRenderingColors(0);
      }
      foreach (const auto& pair, defaultSettings.getColors()) {
        if (enabledColors.contains(pair.first)) {
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
  static std::shared_ptr<GraphicsOutputJob> boardRenderingPdf() noexcept;

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
