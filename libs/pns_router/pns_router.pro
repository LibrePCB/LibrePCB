TEMPLATE = lib
TARGET = pnsrouter

# Use common project definitions
include(../../common.pri)

QT -= core gui widgets

CONFIG -= qt app_bundle
CONFIG += staticlib

# suppress compiler warnings
CONFIG += warn_off

INCLUDEPATH += \
    include \

SOURCES += \
    #    common/geometry/convex_hull.cpp \
    #    common/geometry/hetriang.cpp \
    #    common/geometry/shape_poly_set.cpp \
    common/geometry/geometry_utils.cpp \
    common/geometry/seg.cpp \
    common/geometry/shape.cpp \
    common/geometry/shape_arc.cpp \
    common/geometry/shape_collisions.cpp \
    common/geometry/shape_file_io.cpp \
    common/geometry/shape_line_chain.cpp \
    common/math/math_util.cpp \
    router/pns_algo_base.cpp \
    router/pns_diff_pair.cpp \
    router/pns_diff_pair_placer.cpp \
    router/pns_dp_meander_placer.cpp \
    router/pns_dragger.cpp \
    router/pns_item.cpp \
    router/pns_itemset.cpp \
    router/pns_line.cpp \
    router/pns_line_placer.cpp \
    router/pns_logger.cpp \
    router/pns_meander.cpp \
    router/pns_meander_placer.cpp \
    router/pns_meander_placer_base.cpp \
    router/pns_meander_skew_placer.cpp \
    router/pns_node.cpp \
    router/pns_optimizer.cpp \
    router/pns_router.cpp \
    router/pns_routing_settings.cpp \
    router/pns_shove.cpp \
    router/pns_sizes_settings.cpp \
    router/pns_solid.cpp \
    router/pns_topology.cpp \
    router/pns_utils.cpp \
    router/pns_via.cpp \
    router/pns_walkaround.cpp \
    router/time_limit.cpp \
    wx_compat.cpp \

HEADERS += \
    include/core/optional.h \
    include/math/math_util.h \
    include/math/matrix3x3.h \
    include/math/box2.h \
    include/math/vector2d.h \
    include/layers_id_colors_and_visibility.h \
    include/geometry/shape_simple.h \
    include/geometry/shape_rect.h \
    include/geometry/shape_line_chain.h \
    include/geometry/shape.h \
    include/geometry/shape_segment.h \
    include/geometry/poly_grid_partition.h \
    include/geometry/shape_arc.h \
    include/geometry/rtree.h \
    include/geometry/shape_file_io.h \
    include/geometry/shape_poly_set.h \
    include/geometry/seg.h \
    include/geometry/geometry_utils.h \
    include/geometry/shape_index.h \
    include/geometry/direction45.h \
    include/geometry/shape_index_list.h \
    include/geometry/shape_circle.h \
    include/geometry/convex_hull.h \
    router/pns_meander_placer_base.h \
    router/pns_logger.h \
    router/time_limit.h \
    router/pns_optimizer.h \
    router/pns_via.h \
    router/pns_shove.h \
    router/pns_meander_skew_placer.h \
    router/pns_solid.h \
    router/pns_layerset.h \
    router/pns_node.h \
    router/pns_line.h \
    router/pns_algo_base.h \
    router/pns_diff_pair.h \
    router/pns_utils.h \
    router/pns_walkaround.h \
    router/pns_sizes_settings.h \
    router/pns_topology.h \
    router/pns_router.h \
    router/ranged_num.h \
    router/pns_item.h \
    router/pns_line_placer.h \
    router/pns_index.h \
    router/pns_diff_pair_placer.h \
    router/pns_routing_settings.h \
    router/pns_dp_meander_placer.h \
    router/pns_meander.h \
    router/pns_placement_algo.h \
    router/pns_segment.h \
    router/pns_debug_decorator.h \
    router/pns_itemset.h \
    router/pns_joint.h \
    router/pns_dragger.h \
    router/range.h \
    router/pns_meander_placer.h \
    wx_compat.h \
    class_track.h \
