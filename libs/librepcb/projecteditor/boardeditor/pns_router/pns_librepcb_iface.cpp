#include "pns_librepcb_iface.h"
//#include "board/board.hpp"
//#include "board/board_layers.hpp"
//#include "board/via_padstack_provider.hpp"
//#include "canvas/canvas_gl.hpp"
#include "clipper/clipper.hpp"
#include "geometry/shape_simple.h"
#include "pns_router/router/pns_debug_decorator.h"
#include "pns_router/router/pns_solid.h"
#include "pns_router/router/pns_topology.h"
#include "pns_router/router/pns_via.h"
//#include "util/util.hpp"

#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/primitivecirclegraphicsitem.h>
#include <librepcb/common/graphics/primitivepathgraphicsitem.h>
#include <librepcb/common/utils/clipperhelpers.h>
#include <librepcb/library/pkg/footprintpad.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/project.h>

namespace PNS {

int PNS_LIBREPCB_IFACE::layer_to_router(const QString& l) {
  if (l == librepcb::GraphicsLayer::sTopCopper)
    return F_Cu;
  else if (l == librepcb::GraphicsLayer::sBotCopper)
    return B_Cu;
  else if (l == librepcb::GraphicsLayer::getInnerLayerName(1))
    return In1_Cu;
  else if (l == librepcb::GraphicsLayer::getInnerLayerName(2))
    return In2_Cu;
  else if (l == librepcb::GraphicsLayer::getInnerLayerName(3))
    return In3_Cu;
  else if (l == librepcb::GraphicsLayer::getInnerLayerName(4))
    return In4_Cu;
  else
    return UNDEFINED_LAYER;
}

QString PNS_LIBREPCB_IFACE::layer_from_router(int l) {
  switch (l) {
    case F_Cu:
      return librepcb::GraphicsLayer::sTopCopper;
    case B_Cu:
      return librepcb::GraphicsLayer::sBotCopper;
    case In1_Cu:
      return librepcb::GraphicsLayer::getInnerLayerName(1);
    case In2_Cu:
      return librepcb::GraphicsLayer::getInnerLayerName(2);
    case In3_Cu:
      return librepcb::GraphicsLayer::getInnerLayerName(3);
    case In4_Cu:
      return librepcb::GraphicsLayer::getInnerLayerName(4);
    default:
      Q_ASSERT(false);
      return QString();
  }
}

class PNS_LIBREPCB_RULE_RESOLVER : public PNS::RULE_RESOLVER {
public:
  PNS_LIBREPCB_RULE_RESOLVER(const librepcb::project::Board* aBoard,
                             PNS::ROUTER*                    aRouter);
  virtual ~PNS_LIBREPCB_RULE_RESOLVER();

  virtual int  Clearance(const PNS::ITEM* aA,
                         const PNS::ITEM* aB) const override;
  virtual int  Clearance(int aNetCode) const override;
  virtual int  DpCoupledNet(int aNet) override;
  virtual int  DpNetPolarity(int aNet) override;
  virtual bool DpNetPair(PNS::ITEM* aItem, int& aNetP, int& aNetN) override;

private:
  PNS::ROUTER*                    m_router;
  const librepcb::project::Board* m_board;
  PNS_LIBREPCB_IFACE*             m_iface = nullptr;

  bool m_useDpGap = false;
};

PNS_LIBREPCB_RULE_RESOLVER::PNS_LIBREPCB_RULE_RESOLVER(
    const librepcb::project::Board* aBoard, PNS::ROUTER* aRouter)
  : m_router(aRouter), m_board(aBoard) {
  PNS::NODE* world = m_router->GetWorld();

  PNS::TOPOLOGY topo(world);

  m_iface = static_cast<PNS_LIBREPCB_IFACE*>(m_router->GetInterface());
}

PNS_LIBREPCB_RULE_RESOLVER::~PNS_LIBREPCB_RULE_RESOLVER() {
}

// static const PNS_LIBREPCB_PARENT_ITEM parent_dummy_outline;

int PNS_LIBREPCB_RULE_RESOLVER::Clearance(const PNS::ITEM* /*aA*/,
                                          const PNS::ITEM* /*aB*/) const {
  return librepcb::Length::fromMm(0.1).toNm();  // TODO
}

int PNS_LIBREPCB_RULE_RESOLVER::Clearance(int /*aNetCode*/) const {
  // only used for display purposes, can return dummy value
  return .1e6;
}

int PNS_LIBREPCB_RULE_RESOLVER::DpCoupledNet(int /*aNet*/) {
  return -1;  // TODO
}

int PNS_LIBREPCB_RULE_RESOLVER::DpNetPolarity(int /*aNet*/) {
  return -1;  // TODO
}

bool PNS_LIBREPCB_RULE_RESOLVER::DpNetPair(PNS::ITEM* /*aItem*/, int& /*aNetP*/,
                                           int& /*aNetN*/) {
  return false;  // TODO
}

class PNS_LIBREPCB_DEBUG_DECORATOR : public PNS::DEBUG_DECORATOR {
public:
  PNS_LIBREPCB_DEBUG_DECORATOR() : PNS::DEBUG_DECORATOR() {}

  ~PNS_LIBREPCB_DEBUG_DECORATOR() { Clear(); }

  //    void AddPoint(VECTOR2I aP, int aColor) override
  //    {
  //        SHAPE_LINE_CHAIN l;
  //
  //        l.Append(aP - VECTOR2I(-50000, -50000));
  //        l.Append(aP + VECTOR2I(-50000, -50000));
  //
  //        AddLine(l, aColor, 10000);
  //
  //        l.Clear();
  //        l.Append(aP - VECTOR2I(50000, -50000));
  //        l.Append(aP + VECTOR2I(50000, -50000));
  //
  //        AddLine(l, aColor, 10000);
  //    }
  //
  //    void AddBox(BOX2I aB, int aColor) override
  //    {
  //        SHAPE_LINE_CHAIN l;
  //
  //        VECTOR2I o = aB.GetOrigin();
  //        VECTOR2I s = aB.GetSize();
  //
  //        l.Append(o);
  //        l.Append(o.x + s.x, o.y);
  //        l.Append(o.x + s.x, o.y + s.y);
  //        l.Append(o.x, o.y + s.y);
  //        l.Append(o);
  //
  //        AddLine(l, aColor, 10000);
  //    }
  //
  //    void AddSegment(SEG aS, int aColor) override
  //    {
  //        SHAPE_LINE_CHAIN l;
  //
  //        l.Append(aS.A);
  //        l.Append(aS.B);
  //
  //        AddLine(l, aColor, 10000);
  //    }
  //
  //    void AddDirections(VECTOR2D aP, int aMask, int aColor) override
  //    {
  //        BOX2I b(aP - VECTOR2I(10000, 10000), VECTOR2I(20000, 20000));
  //
  //        AddBox(b, aColor);
  //        for (int i = 0; i < 8; i++) {
  //            if ((1 << i) & aMask) {
  //                VECTOR2I v =
  //                DIRECTION_45((DIRECTION_45::Directions)i).ToVector() *
  //                100000; AddSegment(SEG(aP, aP + v), aColor);
  //            }
  //        }
  //    }
  //
  //    void AddLine(const SHAPE_LINE_CHAIN &aLine, int aType, int aWidth)
  //    override
  //    {
  //        auto npts = aLine.PointCount();
  //        std::deque<librepcb::Coordi> pts;
  //        for (int i = 0; i < npts; i++) {
  //            auto pt = aLine.CPoint(i);
  //            pts.emplace_back(pt.x, pt.y);
  //        }
  //        lines.insert(m_canvas->add_line(pts, aWidth,
  //        librepcb::ColorP::YELLOW, 10000));
  //    }
  //
  //    void Clear() override
  //    {
  //        std::cout << "debug clear" << std::endl;
  //        for (auto &li : lines) {
  //            m_canvas->remove_obj(li);
  //        }
  //        lines.clear();
  //    }
  //
  // private:
  //    std::set<librepcb::ObjectRef> lines;
};

int PNS_LIBREPCB_IFACE::get_net_code(const librepcb::Uuid& uu) {
  if (net_code_map.count(uu)) {
    return net_code_map.at(uu);
  } else {
    net_code_max++;
    net_code_map.emplace(uu, net_code_max);
    net_code_map_r.emplace(net_code_max, uu);
    return net_code_max;
  }
}

librepcb::project::NetSignal* PNS_LIBREPCB_IFACE::get_net_for_code(int code) {
  if (code == PNS::ITEM::UnusedNet) return nullptr;
  if (!net_code_map_r.count(code)) return nullptr;
  return board->getProject().getCircuit().getNetSignalByUuid(
      net_code_map_r.at(code));
}

const PNS_LIBREPCB_PARENT_ITEM* PNS_LIBREPCB_IFACE::get_parent(
    const librepcb::project::BI_NetLine* line) {
  PNS_LIBREPCB_PARENT_ITEM it(line);
  return &*parents.insert(it).first;
}

const PNS_LIBREPCB_PARENT_ITEM* PNS_LIBREPCB_IFACE::get_parent(
    const librepcb::project::BI_Via* via) {
  PNS_LIBREPCB_PARENT_ITEM it(via);
  return &*parents.insert(it).first;
}

const PNS_LIBREPCB_PARENT_ITEM* PNS_LIBREPCB_IFACE::get_parent(
    const librepcb::project::BI_Footprint*    fpt,
    const librepcb::project::BI_FootprintPad* pad) {
  PNS_LIBREPCB_PARENT_ITEM it(fpt, pad);
  return &*parents.insert(it).first;
}

std::unique_ptr<PNS::SEGMENT> PNS_LIBREPCB_IFACE::syncNetLine(
    const librepcb::project::BI_NetLine* line) {
  int net = get_net_code(line->getNetSignalOfNetSegment().getUuid());
  librepcb::Point               from = line->getStartPoint().getPosition();
  librepcb::Point               to   = line->getEndPoint().getPosition();
  std::unique_ptr<PNS::SEGMENT> segment(
      new PNS::SEGMENT(SEG(from.getX().toNm(), from.getY().toNm(),
                           to.getX().toNm(), to.getY().toNm()),
                       net));

  segment->SetWidth(line->getWidth()->toNm());
  segment->SetLayer(layer_to_router(line->getLayer().getName()));
  segment->SetParent(get_parent(line));

  return segment;
}

std::unique_ptr<PNS::VIA> PNS_LIBREPCB_IFACE::syncVia(
    const librepcb::project::BI_Via* via) {
  librepcb::Point pos = via->getPosition();
  int             net = get_net_code(via->getNetSignalOfNetSegment().getUuid());
  std::unique_ptr<PNS::VIA> pvia(new PNS::VIA(
      VECTOR2I(pos.getX().toNm(), pos.getY().toNm()),
      LAYER_RANGE(layer_to_router(librepcb::GraphicsLayer::sTopCopper),
                  layer_to_router(librepcb::GraphicsLayer::sBotCopper)),
      via->getSize()->toNm(), via->getDrillDiameter()->toNm(), net,
      VIA_THROUGH));

  pvia->SetParent(get_parent(via));
  return pvia;
}

std::unique_ptr<PNS::SOLID> PNS_LIBREPCB_IFACE::syncPad(
    const librepcb::project::BI_Footprint*    fpt,
    const librepcb::project::BI_FootprintPad* pad) {
  librepcb::Point             pos = pad->getPosition();
  std::unique_ptr<PNS::SOLID> solid(new PNS::SOLID);
  if (pad->getCompSigInstNetSignal()) {
    solid->SetNet(get_net_code(pad->getCompSigInstNetSignal()->getUuid()));
  }
  if (pad->getLayerName() == librepcb::GraphicsLayer::sTopCopper) {
    solid->SetLayer(layer_to_router(librepcb::GraphicsLayer::sTopCopper));
  } else if (pad->getLayerName() == librepcb::GraphicsLayer::sBotCopper) {
    solid->SetLayer(layer_to_router(librepcb::GraphicsLayer::sBotCopper));
  } else {
    solid->SetLayers(
        LAYER_RANGE(layer_to_router(librepcb::GraphicsLayer::sTopCopper),
                    layer_to_router(librepcb::GraphicsLayer::sBotCopper)));
  }
  solid->SetOffset(VECTOR2I(0, 0));
  solid->SetPos(VECTOR2I(pos.getX().toNm(), pos.getY().toNm()));
  SHAPE_SIMPLE*    shape = new SHAPE_SIMPLE();
  ClipperLib::Path path  = librepcb::ClipperHelpers::convert(
      pad->getSceneOutline(), librepcb::PositiveLength(5000));
  foreach (const auto& v, path) { shape->Append(v.X, v.Y); }
  solid->SetShape(shape);
  solid->SetParent(get_parent(fpt, pad));
  return solid;
}

void PNS_LIBREPCB_IFACE::SyncWorld(PNS::NODE* aWorld) {
  std::cout << "!!!sync world" << std::endl;
  if (!board) {
    wxLogTrace("PNS", "No board attached, aborting sync.");
    return;
  }
  parents.clear();

  foreach (const librepcb::project::BI_NetSegment* segment,
           board->getNetSegments()) {
    foreach (const librepcb::project::BI_NetLine* netline,
             segment->getNetLines()) {
      auto seg = syncNetLine(netline);
      if (seg) {
        aWorld->Add(std::move(seg));
      }
    }

    foreach (const librepcb::project::BI_Via* via, segment->getVias()) {
      auto v = syncVia(via);
      if (v) {
        aWorld->Add(std::move(v));
      }
    }
  }

  foreach (const librepcb::project::BI_Device* device,
           board->getDeviceInstances()) {
    foreach (const librepcb::project::BI_FootprintPad* pad,
             device->getFootprint().getPads()) {
      auto p = syncPad(&device->getFootprint(), pad);
      if (p) {
        aWorld->Add(std::move(p));
      }
    }
  }

  int worstClearance = librepcb::Length::fromMm(1).toNm();  // TODO

  delete m_ruleResolver;
  m_ruleResolver = new PNS_LIBREPCB_RULE_RESOLVER(board, m_router);

  aWorld->SetRuleResolver(m_ruleResolver);
  aWorld->SetMaxClearance(4 * worstClearance);
}

void PNS_LIBREPCB_IFACE::EraseView() {
  std::cout << "iface erase view" << std::endl;

  for (const auto& it : m_preview_items) {
    board->getGraphicsScene().removeItem(*it);
    delete it;
  }
  m_preview_items.clear();

  foreach (librepcb::project::BI_Base* item, m_hidden_items) {
    item->setVisible(true);
  }
  m_hidden_items.clear();

  if (m_debugDecorator) m_debugDecorator->Clear();
}

void PNS_LIBREPCB_IFACE::DisplayItem(const PNS::ITEM* aItem, int /*aColor*/,
                                     int /*aClearance*/) {
  wxLogTrace("PNS", "DisplayItem %p %s", aItem, aItem->KindStr().c_str());
  if (aItem->Kind() == PNS::ITEM::LINE_T) {
    auto                     line_item = dynamic_cast<const PNS::LINE*>(aItem);
    QString                  layerName = layer_from_router(line_item->Layer());
    librepcb::UnsignedLength width(line_item->Width());
    librepcb::Path           path;
    for (int i = 0; i < line_item->PointCount(); i++) {
      auto pt = line_item->CPoint(i);
      path.addVertex(librepcb::Point(pt.x, pt.y));
    }
    librepcb::PrimitivePathGraphicsItem* gItem =
        new librepcb::PrimitivePathGraphicsItem();
    gItem->setPath(path.toQPainterPathPx());
    gItem->setLineWidth(width);
    gItem->setLineLayer(board->getLayerStack().getLayer(layerName));
    board->getGraphicsScene().addItem(*gItem);
    gItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    gItem->setSelected(true);
    m_preview_items.append(gItem);
  } else if (aItem->Kind() == PNS::ITEM::SEGMENT_T) {
    auto    seg_item  = dynamic_cast<const PNS::SEGMENT*>(aItem);
    auto    seg       = seg_item->Seg();
    QString layerName = layer_from_router(seg_item->Layer());
    librepcb::UnsignedLength width(seg_item->Width());
    librepcb::Path           path;
    path.addVertex(librepcb::Point(seg.A.x, seg.A.y));
    path.addVertex(librepcb::Point(seg.B.x, seg.B.y));
    librepcb::PrimitivePathGraphicsItem* gItem =
        new librepcb::PrimitivePathGraphicsItem();
    gItem->setPath(path.toQPainterPathPx());
    gItem->setLineWidth(width);
    gItem->setLineLayer(board->getLayerStack().getLayer(layerName));
    board->getGraphicsScene().addItem(*gItem);
    m_preview_items.append(gItem);
  } else if (aItem->Kind() == PNS::ITEM::VIA_T) {
    auto                     via_item = dynamic_cast<const PNS::VIA*>(aItem);
    librepcb::Point          pos(via_item->Pos().x, via_item->Pos().y);
    librepcb::UnsignedLength diameter(via_item->Diameter());
    // QString layerStart = layer_from_router(via_item->Layers().Start());
    // QString layerEnd = layer_from_router(via_item->Layers().End());
    librepcb::PrimitiveCircleGraphicsItem* gItem =
        new librepcb::PrimitiveCircleGraphicsItem();
    gItem->setPos(pos.toPxQPointF());
    gItem->setDiameter(diameter);
    gItem->setLineLayer(board->getLayerStack().getLayer(
        librepcb::GraphicsLayer::sBoardViasTht));
    gItem->setFillLayer(board->getLayerStack().getLayer(
        librepcb::GraphicsLayer::sBoardViasTht));
    gItem->setZValue(librepcb::project::Board::ZValue_Vias);
    board->getGraphicsScene().addItem(*gItem);
    m_preview_items.append(gItem);
  } else {
    assert(false);
  }
}

void PNS_LIBREPCB_IFACE::HideItem(PNS::ITEM* aItem) {
  std::cout << "iface hide item" << std::endl;
  const PNS_LIBREPCB_PARENT_ITEM* parent = aItem->Parent();
  if (parent) {
    if (parent->line) {
      librepcb::project::BI_NetLine* l =
          const_cast<librepcb::project::BI_NetLine*>(parent->line);
      l->setVisible(false);
      m_hidden_items.append(l);
    } else if (parent->via) {
      librepcb::project::BI_Via* v =
          const_cast<librepcb::project::BI_Via*>(parent->via);
      v->setVisible(false);
      m_hidden_items.append(v);
    }
  }
}

void PNS_LIBREPCB_IFACE::RemoveItem(PNS::ITEM* aItem) {
  auto parent = aItem->Parent();
  std::cout << "!!!iface remove item " << parent << " " << aItem->KindStr()
            << std::endl;
  if (parent) {
    // if (parent->track) {
    //    board->tracks.erase(parent->track->uuid);
    //}
    // else if (parent->via) {
    //    board->vias.erase(parent->via->uuid);
    //}
  }
}

void PNS_LIBREPCB_IFACE::AddItem(PNS::ITEM* aItem) {
  std::cout << "!!!iface add item" << std::endl;
  switch (aItem->Kind()) {
    case PNS::ITEM::SEGMENT_T: {
      PNS::SEGMENT*  seg = static_cast<PNS::SEGMENT*>(aItem);
      const SEG&     s   = seg->Seg();
      librepcb::Uuid uu  = librepcb::Uuid::createRandom();
      // auto track = &board->tracks.emplace(uu, uu).first->second;
      librepcb::Point  from(s.A.x, s.A.y);
      librepcb::Point  to(s.B.x, s.B.y);
      librepcb::Length width(seg->Width());

      QString layerName = layer_from_router(seg->Layer());
      // track->layer = layer;

      // auto connect = [this, &layer](librepcb::Track::Connection &conn, const
      // librepcb::Coordi &c) {
      //    auto p = find_pad(layer, c);
      //    if (p.first) {
      //        conn.connect(p.first, p.second);
      //    }
      //    else {
      //        auto j = find_junction(layer, c);
      //        if (j) {
      //            conn.connect(j);
      //        }
      //        else {
      //            auto juu = librepcb::UUID::random();
      //            auto ju = &board->junctions.emplace(juu, juu).first->second;
      //            ju->layer = layer;
      //            ju->position = c;
      //            conn.connect(ju);
      //        }
      //    }
      //
      //};
      // connect(track->from, from);
      // connect(track->to, to);
      // track->width_from_rules = m_router->Sizes().WidthFromRules();
      // aItem->SetParent(get_parent(track));
    } break;

    case PNS::ITEM::VIA_T: {
      //        PNS::VIA *pvia = static_cast<PNS::VIA *>(aItem);
      //        auto uu = librepcb::UUID::random();
      //        auto net = get_net_for_code(pvia->Net());
      //        auto padstack =
      //        vpp->get_padstack(rules->get_via_padstack_uuid(net)); if
      //        (padstack) {
      //            auto ps = rules->get_via_parameter_set(net);
      //            auto via = &board->vias
      //                                .emplace(std::piecewise_construct,
      //                                std::forward_as_tuple(uu),
      //                                         std::forward_as_tuple(uu,
      //                                         padstack))
      //                                .first->second;
      //            via->parameter_set = ps;
      //
      //            librepcb::Coordi c(pvia->Pos().x, pvia->Pos().y);
      //            auto j = find_junction(10000, c);
      //            if (j) {
      //                via->junction = j;
      //            }
      //            else {
      //                auto juu = librepcb::UUID::random();
      //                auto ju = &board->junctions.emplace(juu,
      //                juu).first->second; ju->position = c; via->junction =
      //                ju;
      //            }
      //            via->junction->has_via = true;
      //        }
    } break;

    default:
      std::cout << "!!!unhandled add " << aItem->KindStr() << std::endl;
  }
}

void PNS_LIBREPCB_IFACE::Commit() {
  EraseView();
}

void PNS_LIBREPCB_IFACE::UpdateNet(int aNetCode) {
  qDebug() << "PNS_LIBREPCB_IFACE::UpdateNet(" << aNetCode << ")";
}

PNS::RULE_RESOLVER* PNS_LIBREPCB_IFACE::GetRuleResolver() {
  return m_ruleResolver;
}

void PNS_LIBREPCB_IFACE::create_debug_decorator() {
  if (!m_debugDecorator) {
    m_debugDecorator = new PNS_LIBREPCB_DEBUG_DECORATOR();
  }
}

PNS::DEBUG_DECORATOR* PNS_LIBREPCB_IFACE::GetDebugDecorator() {
  return m_debugDecorator;
}

PNS_LIBREPCB_IFACE::~PNS_LIBREPCB_IFACE() {
  delete m_ruleResolver;
  delete m_debugDecorator;
}
}  // namespace PNS
