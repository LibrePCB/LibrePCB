# Images

This directory contains all images which are required in the LibrePCB
application or other tools.

- Most images are from the
  [Open Icon Library](http://openiconlibrary.sourceforge.net/)
- Some images are from https://www.iconfinder.com/
- Some images are from https://icons8.com/ (we may use those icons under the
  free license since we attribute icons8 in the about dialog)

## Adding New Images

- Only use icons licensed under a *really* free license! Don't use Icons8
  anymore as the license is not really free.
- For toolbar icons, use 64x64 (preferred) or 48x48 PNGs.
- Reduce PNG size with this command:

      pngquant --verbose --skip-if-larger --force --ext .png --strip <FILE>

- List all new icons with their license in [`.reuse/dep5`](../.reuse/dep5).
