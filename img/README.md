# Images

This directory contains all images which are required in the LibrePCB
application or other tools.

## Sources

Preferred sources of images (best at top):

1. [Font Awesome](https://fontawesome.com/icons) from directory
   [`libs/font-awesome/svgs`](../libs/font-awesome/svgs)
2. [Bootstrap Icons](https://icons.getbootstrap.com/) from directory
   [`libs/bootstrap-icons/icons`](../libs/bootstrap-icons/icons)
3. Not recommended: [Open Icon Library](http://openiconlibrary.sourceforge.net/)

Alternatively, they can also be self-drawn. Create SVGs which are as small
as possible in file size.

## Rules

- Only use icons licensed under a *really* free license! Don't use Icons8
  anymore as the license is not really free. Any icons in
  [`libs/font-awesome/svgs`](../libs/font-awesome/svgs) and
  [`libs/bootstrap-icons/icons`](../libs/bootstrap-icons/icons) are safe to use.
- Use SVG whenever possible. PNG (ideally 64x64) or JPEG are allowed only
  in rare exceptions (e.g. wizard watermarks).
- Reduce PNG size with this command:
  ```
  pngquant --verbose --skip-if-larger --force --ext .png --strip <FILE>
  ```
- Generally use monochrome icons with black color. Single-color icons shall
  be recolored dynamically in the UI. Only multicolor icons shall be stored
  with colors.
- Line thickness of the icons should look consistent with the rest of the UI.
  Many icons are available from different sources, but with different line
  widths (e.g. Bootstrap Icons are often thinner than Font Awesome). Choose
  the one which looks best / most consistent in the UI.
- Any icons downloaded from the Internet and added to the `img` directory
  shall be listed explicitly in [`.reuse/dep5`](../.reuse/dep5). Any self-drawn
  icons don't need to be listed as they are covered by a glob pattern.
