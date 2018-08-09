#pragma once
#include "pns_router/router/pns_router.h"

#include <QtCore>
#include <QtWidgets>

namespace librepcb {
class Uuid;
class GraphicsLayer;
namespace project {
class NetSignal;
class Board;
class BI_Base;
class BI_NetLine;
class BI_Via;
class BI_Footprint;
class BI_FootprintPad;
}  // namespace project
}  // namespace librepcb

namespace PNS {
class PNS_LIBREPCB_PARENT_ITEM {
public:
  PNS_LIBREPCB_PARENT_ITEM() {}
  PNS_LIBREPCB_PARENT_ITEM(const librepcb::project::BI_NetLine* l) : line(l) {}
  PNS_LIBREPCB_PARENT_ITEM(const librepcb::project::BI_Via* v) : via(v) {}
  PNS_LIBREPCB_PARENT_ITEM(const librepcb::project::BI_Footprint*    fpt,
                           const librepcb::project::BI_FootprintPad* p)
    : footprint(fpt), pad(p) {}

  const librepcb::project::BI_NetLine*      line      = nullptr;
  const librepcb::project::BI_Via*          via       = nullptr;
  const librepcb::project::BI_Footprint*    footprint = nullptr;
  const librepcb::project::BI_FootprintPad* pad       = nullptr;

  bool operator<(const PNS_LIBREPCB_PARENT_ITEM& other) const {
    if (line < other.line)
      return true;
    else if (line > other.line)
      return false;

    if (via < other.via)
      return true;
    else if (via > other.via)
      return false;

    if (footprint < other.footprint)
      return true;
    else if (footprint > other.footprint)
      return false;

    if (pad < other.pad)
      return true;
    else if (pad > other.pad)
      return false;

    return false;
  }
};

class PNS_LIBREPCB_IFACE : public PNS::ROUTER_IFACE {
public:
  PNS_LIBREPCB_IFACE() {}
  ~PNS_LIBREPCB_IFACE();

  void SetRouter(PNS::ROUTER* aRouter) override { m_router = aRouter; }
  void SetBoard(librepcb::project::Board* brd) { board = brd; }

  void SyncWorld(PNS::NODE* aWorld) override;
  void EraseView() override;
  void HideItem(PNS::ITEM* aItem) override;
  void DisplayItem(const PNS::ITEM* aItem, int aColor = 0,
                   int aClearance = 0) override;
  void AddItem(PNS::ITEM* aItem) override;
  void RemoveItem(PNS::ITEM* aItem) override;
  void Commit() override;

  void UpdateNet(int aNetCode) override;

  PNS::RULE_RESOLVER*   GetRuleResolver() override;
  PNS::DEBUG_DECORATOR* GetDebugDecorator() override;

  void create_debug_decorator();

  static int                    layer_to_router(const QString& l);
  static QString                layer_from_router(int l);
  librepcb::project::NetSignal* get_net_for_code(int code);
  int                           get_net_code(const librepcb::Uuid& uu);

  const PNS_LIBREPCB_PARENT_ITEM* get_parent(
      const librepcb::project::BI_NetLine* line);
  const PNS_LIBREPCB_PARENT_ITEM* get_parent(
      const librepcb::project::BI_Via* via);
  const PNS_LIBREPCB_PARENT_ITEM* get_parent(
      const librepcb::project::BI_Footprint*    fpt,
      const librepcb::project::BI_FootprintPad* pad);

  int64_t get_override_routing_offset() const {
    return override_routing_offset;
  }

  void set_override_routing_offset(int64_t o) { override_routing_offset = o; }

private:
  class PNS_LIBREPCB_RULE_RESOLVER*    m_ruleResolver   = nullptr;
  class PNS_LIBREPCB_DEBUG_DECORATOR*  m_debugDecorator = nullptr;
  QVector<QGraphicsItem*>              m_preview_items;
  QVector<librepcb::project::BI_Base*> m_hidden_items;

  librepcb::project::Board* board = nullptr;
  PNS::NODE*                m_world;
  PNS::ROUTER*              m_router;

  std::unique_ptr<PNS::SEGMENT> syncNetLine(
      const librepcb::project::BI_NetLine* line);
  std::unique_ptr<PNS::VIA>   syncVia(const librepcb::project::BI_Via* via);
  std::unique_ptr<PNS::SOLID> syncPad(
      const librepcb::project::BI_Footprint*    fpt,
      const librepcb::project::BI_FootprintPad* pad);

  std::map<librepcb::Uuid, int> net_code_map;
  std::map<int, librepcb::Uuid> net_code_map_r;
  int                           net_code_max = 0;

  int64_t override_routing_offset = -1;

  std::set<PNS_LIBREPCB_PARENT_ITEM> parents;
};
}  // namespace PNS
