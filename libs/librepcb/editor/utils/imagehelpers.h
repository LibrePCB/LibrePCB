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

#ifndef LIBREPCB_EDITOR_IMAGEHELPERS_H
#define LIBREPCB_EDITOR_IMAGEHELPERS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/fileproofname.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class TransactionalDirectory;

namespace editor {

/*******************************************************************************
 *  Class ImageHelpers
 ******************************************************************************/

/**
 * @brief Various editor helper functions for working with ::librepcb::Image
 *
 * Intended to share code between various editors resp. their state machines.
 */
class ImageHelpers final {
  Q_DECLARE_TR_FUNCTIONS(ImageHelpers)

public:
  // Types
  enum class Target { Symbol, Project };

  // Constructors / Destructor
  ImageHelpers() = delete;
  ImageHelpers(const ImageHelpers& other) = delete;
  ~ImageHelpers() = delete;

  // Operator Overloadings
  ImageHelpers& operator=(const ImageHelpers& rhs) = delete;

  // Static Methods

  /**
   * @brief Find a file with specific content in a directory
   *
   * Intended to reuse image files already existing in a directory.
   *
   * @param dir     The directory to search for files.
   * @param data    The content of the file to search for.
   * @return The filename if a file was found, `std::nullopt` otherwise.
   */
  static std::optional<FileProofName> findExistingFile(
      const TransactionalDirectory& dir, const QByteArray& data);

  /**
   * @brief Build a valid filename for an image file to be created
   *
   * This method builds a filename that is a valid ::librepcb::FileProofName
   * for a file which doesn't exist yet in a given directory by guarantee. If
   * there are filename conflicts, a number will be appended to make it unique.
   * Intended to be used to determine a valid filename before adding a new
   * file to a directory.
   *
   * @param dir           The directory where a new file shall be created.
   * @param nameUserInput The desired file basename (untrusted input accepted).
   *                      If empty or completely invalid, a fallback name will
   *                      be used instead.
   * @param extension     The image file extension. Must be one of
   *                      ::librepcb::Image::getSupportedExtensions()
   *                      (case sensitive), otherwise an exception is thrown!
   * @return A guaranteed valid filename of a non-existent file.
   */
  static FileProofName getUnusedFileName(const TransactionalDirectory& dir,
                                         QString nameUserInput,
                                         const QString& extension);

  /**
   * @brief Execute "open file" dialog to choose an image
   *
   * @note  If neccessary, this method does convert the selected image file
   *        to a supported image format. However, if the file extension is
   *        one of the supported formats, this method does not open or validate
   *        the file. Always call ::librepcb::Image::tryLoad() afterwards to
   *        do so.
   *
   * @param data        Content of the chosen image (may have been converted
   *                    into a supported image format).
   * @param format      Image format of `data` (e.g. "png").
   * @param basename    Basename of the chosen file (untrusted user input).
   * @param settingsKey The dialog saves the chosen file under this QSettings
   *                    key and preselects the file when opening the next time.
   *                    Set to something like "schematic_editor/add_image/file".
   * @retval true       An image has been chosen (and converted if neccessary).
   * @retval false      The user canceled the file dialog.
   * @throws Exception  If a format conversion was needed and it failed.
   */
  static bool execImageChooserDialog(QByteArray& data, QString& format,
                                     QString& basename,
                                     const QString& settingsKey);

  /**
   * @brief Determine the filename of an image to be added to a directory
   *
   * This may show a blocking input dialog where the user can enter a file name.
   *
   * @param dir      Directory where the file is asked to be added (this method
   *                 does *not* actually add it).
   * @param target   The target type that the directory represents. Only used
   *                 for a UI translation string.
   * @param data     The image data asked to be added.
   * @param format   The image format of `data`.
   * @param basename The desired basename of the image (untrusted user input).
   * @param exists   Output variable whether the file already existed in `dir`
   *                 or not.
   * @return The filename of the new file to be created, or `std::nullopt` if
   *         the user aborted the input dialog.
   */
  static std::optional<FileProofName> findExistingOrAskForNewImageFileName(
      const TransactionalDirectory& dir, Target target, const QByteArray& data,
      const QString& format, const QString& basename, bool& exists);

  /**
   * @brief Check if the clipboard contains any kind of image
   *
   * If the clipboard contains a *filepath* to an image file (rather than the
   * image itself), it is also taken into account (i.e. `true` returned) since
   * #getImageFromClipboard() is able to load it.
   *
   * Call this method for a lightweight check if theres is any data
   * #getImageFromClipboard() is able to load.
   *
   * @retval true    The clipboard contains an image.
   * @retval false   There's no image in the clipboard.
   */
  static bool isImageInClipboard() noexcept;

  /**
   * @brief Try to get the image from the clipboard
   *
   * If the clipboard contains a *filepath* to an image file (rather than the
   * image itself), this method tries to load the image from that file.
   *
   * @note  If neccessary, this method does convert the image to a supported
   *        image format. However, if the clipboard already contains one of
   *        the supported formats, this method does not validate the file.
   *        Always call ::librepcb::Image::tryLoad() afterwards to do so.
   *
   * @param data        Content of the image (may have been converted
   *                    into a supported image format).
   * @param format      Image format of `data` (e.g. "png").
   * @param basename    Basename of the image (untrusted user input).
   * @retval true       Successfully fetched the image.
   * @retval false      No image in the clipboard, or failed to fetch it.
   */
  static bool getImageFromClipboard(QByteArray& data, QString& format,
                                    QString& basename) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
