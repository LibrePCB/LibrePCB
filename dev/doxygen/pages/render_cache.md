Render Cache {#doc_render_cache}
================================

[TOC]

The rendering cache accelerates the rendering of schematic and board views. This section
outlines code details and operation.


# Layers {#doc_render_cache_layers}

Every frame consists of three layers, drawn in this order:

1. **Background:** Background fill and grid.
2. **Content:** The scene items (traces, pads, text, etc.).
3. **Foreground:** Overlays such as cursors and rulers.

Each layer can be rendered on its own with
@ref librepcb::editor::GraphicsScene::renderBackground(),
@ref librepcb::editor::GraphicsScene::renderContent() and
@ref librepcb::editor::GraphicsScene::renderForeground(). All of them use
the same scene-to-device transform.

Only the Content layer is cached.  The Background and Foreground layers are re-drawn
each frame.

![Render Layers Diagram](graphics_scene_layers.svg)
