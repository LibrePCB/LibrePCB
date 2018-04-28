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

#ifndef LIBREPCB_GRAPHICSLAYER_H
#define LIBREPCB_GRAPHICSLAYER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <memory>
#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class IF_GraphicsLayerObserver;

/*****************************************************************************************
 *  Class GraphicsLayer
 ****************************************************************************************/

/**
 * @brief The GraphicsLayer class represents a graphical layer used in schematics and boards
 *
 * These layers are used in graphics items (QGraphicsItem) to determine their visibility
 * and colors.
 *
 * @author ubruhin
 * @date 2016-11-04
 */
class GraphicsLayer : public QObject
{
        Q_OBJECT

    public:

        // schematic layers
        //static constexpr const char* sSchematicBackground     = "sch_background";         ///< Primary: background | Secondary: grid
        //static constexpr const char* sSchematicSelection      = "sch_selection";          ///< Primary: outline    | Secondary: area
        static constexpr const char* sSchematicReferences     = "sch_references";         ///< origin crosses of symbols, texts, ...
        static constexpr const char* sSchematicSheetFrames    = "sch_scheet_frames";      ///< e.g. A4 sheet frame + text boxes
        static constexpr const char* sSchematicNetLines       = "sch_net_lines";          ///< librepcb::project::SI_NetLine
        static constexpr const char* sSchematicNetLabels      = "sch_net_labels";         ///< librepcb::project::SI_NetLabel
        static constexpr const char* sSchematicNetLabelAnchors= "sch_net_label_anchors";  ///< anchor line of librepcb::project::SI_NetLabel
        static constexpr const char* sSchematicDocumentation  = "sch_documentation";      ///< for documentation purposes, e.g. text
        static constexpr const char* sSchematicComments       = "sch_comments";           ///< for personal comments, e.g. text
        static constexpr const char* sSchematicGuide          = "sch_guide";              ///< e.g. for boxes around circuits

        // symbol layers
        static constexpr const char* sSymbolOutlines          = "sym_outlines";           ///< dark red lines of symbols
        static constexpr const char* sSymbolGrabAreas         = "sym_grab_areas";         ///< optional yellow area of symbols
        static constexpr const char* sSymbolHiddenGrabAreas   = "sym_hidden_grab_areas";  ///< hidden grab areas of symbols
        static constexpr const char* sSymbolNames             = "sym_names";              ///< text {{NAME}}
        static constexpr const char* sSymbolValues            = "sym_values";             ///< text {{VALUE}}
        static constexpr const char* sSymbolPinCirclesOpt     = "sym_pin_circles_opt";    ///< green circle of unconnected pins
        static constexpr const char* sSymbolPinCirclesReq     = "sym_pin_circles_req";    ///< red circle of unconnected pins
        static constexpr const char* sSymbolPinNames          = "sym_pin_names";          ///< name of the connected component signal
        static constexpr const char* sSymbolPinNumbers        = "sym_pin_numbers";        ///< number of the connected footprint pad

        // asymmetric board layers
        //static constexpr const char* sBoardBackground         = "brd_background";         ///< Primary: background | Secondary: grid
        //static constexpr const char* sBoardSelection          = "brd_selection";          ///< Primary: outline    | Secondary: area
        //static constexpr const char* sBoardReferences         = "brd_references";         ///< origin crosses of footprints, holes, ...
        static constexpr const char* sBoardSheetFrames        = "brd_sheet_frames";       ///< e.g. A4 sheet frame + text boxes
        static constexpr const char* sBoardOutlines           = "brd_outlines";           ///< incl. non-plated through hole milling
        static constexpr const char* sBoardMillingPth         = "brd_milling_pth";        ///< plated through hole milling
        static constexpr const char* sBoardDrillsNpth         = "brd_drills_npth";        ///< non-plated through hole drills
        static constexpr const char* sBoardPadsTht            = "brd_pads_tht";           ///< plated through hole pads
        static constexpr const char* sBoardViasTht            = "brd_vias_tht";           ///< plated through hole vias
        static constexpr const char* sBoardMeasures           = "brd_measures";           ///< measurements documentation
        static constexpr const char* sBoardAlignment          = "brd_alignment";          ///< alignment helpers in devices
        static constexpr const char* sBoardDocumentation      = "brd_documentation";      ///< for documentation purposes, e.g. text
        static constexpr const char* sBoardComments           = "brd_comments";           ///< for personal comments, e.g. text
        static constexpr const char* sBoardGuide              = "brd_guide";              ///< e.g. for boxes around circuits

        // symmetric board layers
        static constexpr const char* sTopPlacement            = "top_placement";          ///< placement information (e.g. outline) of devices
        static constexpr const char* sBotPlacement            = "bot_placement";          ///< placement information (e.g. outline) of devices
        static constexpr const char* sTopDocumentation        = "top_documentation";      ///< like placement layers, but not for silk screen
        static constexpr const char* sBotDocumentation        = "bot_documentation";      ///< like placement layers, but not for silk screen
        static constexpr const char* sTopGrabAreas            = "top_grab_areas";         ///< area where devices can be dragged
        static constexpr const char* sBotGrabAreas            = "bot_grab_areas";         ///< area where devices can be dragged
        static constexpr const char* sTopHiddenGrabAreas      = "top_hidden_grab_areas";  ///< hidden area where devices can be dragged
        static constexpr const char* sBotHiddenGrabAreas      = "bot_hidden_grab_areas";  ///< hidden area where devices can be dragged
        static constexpr const char* sTopReferences           = "top_references";         ///< origin crosses of devices
        static constexpr const char* sBotReferences           = "bot_references";         ///< origin crosses of devices
        static constexpr const char* sTopNames                = "top_names";              ///< text, may be used for silk screen
        static constexpr const char* sBotNames                = "bot_names";              ///< text, may be used for silk screen
        static constexpr const char* sTopValues               = "top_values";             ///< text, may be used for silk screen
        static constexpr const char* sBotValues               = "bot_values";             ///< text, may be used for silk screen
        static constexpr const char* sTopCourtyard            = "top_courtyard";          ///< area required to mount devices
        static constexpr const char* sBotCourtyard            = "bot_courtyard";          ///< area required to mount devices
        static constexpr const char* sTopStopMask             = "top_stop_mask";          ///< areas over smt pads
        static constexpr const char* sBotStopMask             = "bot_stop_mask";          ///< areas over smt pads
        static constexpr const char* sTopSolderPaste          = "top_solder_paste";       ///< areas over smt pads
        static constexpr const char* sBotSolderPaste          = "bot_solder_paste";       ///< areas over smt pads
        static constexpr const char* sTopFinish               = "top_finish";             ///< areas of special surface treatments
        static constexpr const char* sBotFinish               = "bot_finish";             ///< areas of special surface treatments
        static constexpr const char* sTopGlue                 = "top_glue";               ///< adhesive for fixing devices
        static constexpr const char* sBotGlue                 = "bot_glue";               ///< adhesive for fixing devices

        // copper layers
        static constexpr const char* sTopCopper               = "top_cu";
        //static constexpr const char* sInnerCopper#            = "in#_cu";
        static constexpr const char* sBotCopper               = "bot_cu";

#ifdef QT_DEBUG
        // debug layers
        static constexpr const char* sDebugGraphicsItemsBoundingRects       = "dbg_GraphicsItemsBoundingRects";
        static constexpr const char* sDebugGraphicsItemsTextsBoundingRects  = "dbg_GraphicsItemsTextsBoundingRects";
        static constexpr const char* sDebugSymbolPinNetSignalNames          = "dbg_SymbolPinNetSignalNames";
        static constexpr const char* sDebugNetLinesNetSignalNames           = "dbg_NetLinesNetSignalNames";
        static constexpr const char* sDebugInvisibleNetPoints               = "dbg_InvisibleNetPoints";
        static constexpr const char* sDebugComponentSymbolsCounts           = "dbg_ComponentSymbolsCounts";
#endif

        // Constructors / Destructor
        GraphicsLayer() = delete;
        GraphicsLayer(const GraphicsLayer& other) noexcept;
        explicit GraphicsLayer(const QString& name) noexcept;
        virtual ~GraphicsLayer() noexcept;

        // Getters
        const QString& getName() const noexcept {return mName;}
        const QString& getNameTr() const noexcept {return mNameTr;}
        const QColor& getColor(bool highlighted = false) const noexcept {
            return highlighted ? mColorHighlighted : mColor;
        }
        bool getVisible() const noexcept {return mIsVisible;}
        bool isEnabled() const noexcept {return mIsEnabled;}
        bool isVisible() const noexcept {return mIsEnabled && mIsVisible;}
        bool isTopLayer() const noexcept {return isTopLayer(mName);}
        bool isBottomLayer() const noexcept {return isBottomLayer(mName);}
        bool isInnerLayer() const noexcept {return isInnerLayer(mName);}
        bool isCopperLayer() const noexcept {return isCopperLayer(mName);}
        int getInnerLayerNumber() const noexcept {return getInnerLayerNumber(mName);}
        QString getMirroredLayerName() const noexcept {return getMirroredLayerName(mName);}
        QString getGrabAreaLayerName() const noexcept {return getGrabAreaLayerName(mName);}

        // Setters
        void setColor(const QColor& color) noexcept;
        void setColorHighlighted(const QColor& color) noexcept;
        void setVisible(bool visible) noexcept;
        void setEnabled(bool enable) noexcept;

        // General Methods
        void registerObserver(IF_GraphicsLayerObserver& object) const noexcept;
        void unregisterObserver(IF_GraphicsLayerObserver& object) const noexcept;

        // Operator Overloadings
        GraphicsLayer& operator=(const GraphicsLayer& rhs) = delete;

        // Static Methods
        static int getInnerLayerCount() noexcept {return 62;} // some random number... ;)
        static bool isTopLayer(const QString& name) noexcept;
        static bool isBottomLayer(const QString& name) noexcept;
        static bool isInnerLayer(const QString& name) noexcept;
        static bool isCopperLayer(const QString& name) noexcept;
        static QString getInnerLayerName(int number) noexcept;
        static int getInnerLayerNumber(const QString& name) noexcept;
        static QString getMirroredLayerName(const QString& name) noexcept;
        static QString getGrabAreaLayerName(const QString& outlineLayerName) noexcept;
        static const QStringList& getSchematicGeometryElementLayerNames() noexcept;
        static const QStringList& getBoardGeometryElementLayerNames() noexcept;
        static void getDefaultValues(const QString& name, QString& nameTr, QColor& color,
                                     QColor& colorHl, bool& visible) noexcept;


    signals:
        void attributesChanged();


    protected: // Data
        QString mName;              ///< Unique name which is used for serialization
        QString mNameTr;            ///< Layer name (translated into the user's language)
        QColor mColor;              ///< Color of graphics items on that layer
        QColor mColorHighlighted;   ///< Color of hightlighted graphics items on that layer
        bool mIsVisible;            ///< Visibility of graphics items on that layer
        bool mIsEnabled;            ///< Visibility/availability of the layer itself
        mutable QSet<IF_GraphicsLayerObserver*> mObservers; ///< A list of all observer objects
};

/*****************************************************************************************
 *  Interface IF_GraphicsLayerObserver
 ****************************************************************************************/

/**
 * @brief The IF_GraphicsLayerOblayerHighlightColorChangedserver class defines an interface for classes which
 *        can receive updates from graphics layer attributes
 */
class IF_GraphicsLayerObserver
{
    public:
        virtual ~IF_GraphicsLayerObserver() {}

        virtual void layerColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept = 0;
        virtual void layerHighlightColorChanged(const GraphicsLayer& layer, const QColor& newColor) noexcept = 0;
        virtual void layerVisibleChanged(const GraphicsLayer& layer, bool newVisible) noexcept = 0;
        virtual void layerEnabledChanged(const GraphicsLayer& layer, bool newEnabled) noexcept = 0;
        virtual void layerDestroyed(const GraphicsLayer& layer) noexcept = 0;
};

/*****************************************************************************************
 *  Interface IF_GraphicsLayerProvider
 ****************************************************************************************/

/**
 * @brief The IF_GraphicsLayerProvider class defines an interface for classes which
 *        provide layers
 */
class IF_GraphicsLayerProvider
{
    public:
        virtual ~IF_GraphicsLayerProvider() {}

        virtual GraphicsLayer* getLayer(const QString& name) const noexcept = 0;
        virtual QList<GraphicsLayer*> getAllLayers() const noexcept = 0;

        GraphicsLayer* getGrabAreaLayer(const QString outlineLayerName) const noexcept {
            return getLayer(GraphicsLayer::getGrabAreaLayerName(outlineLayerName));
        }

        QList<GraphicsLayer*> getSchematicGeometryElementLayers() const noexcept {
            return getLayers(GraphicsLayer::getSchematicGeometryElementLayerNames());
        }

        QList<GraphicsLayer*> getBoardGeometryElementLayers() const noexcept {
            return getLayers(GraphicsLayer::getBoardGeometryElementLayerNames());
        }

        QList<GraphicsLayer*> getLayers(const QStringList& layerNames) const noexcept {
            QList<GraphicsLayer*> layers;
            foreach (const QString& name, layerNames) {
                GraphicsLayer* layer = getLayer(name);
                if (layer) layers.append(layer);
            }
            return layers;
        }
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_GRAPHICSLAYER_H
