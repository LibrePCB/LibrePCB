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

#ifndef LIBREPCB_WORKSPACE_LIBRARYDOWNLOAD_H
#define LIBREPCB_WORKSPACE_LIBRARYDOWNLOAD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/fileio/filepath.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class FileDownload;

namespace workspace {

/*****************************************************************************************
 *  Class LibraryDownload
 ****************************************************************************************/

/**
 * @brief The LibraryDownload class
 *
 * @author ubruhin
 * @date 2016-10-01
 */
class LibraryDownload final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        LibraryDownload() = delete;
        LibraryDownload(const LibraryDownload& other) = delete;
        LibraryDownload(const QUrl& urlToZip, const FilePath& destDir) noexcept;
        ~LibraryDownload() noexcept;

        // Getters
        const FilePath& getDestinationDir() const noexcept {return mDestDir;}

        // Setters

        /**
         * @copydoc librepcb::NetworkRequestBase::setExpectedReplyContentSize()
         */
        void setExpectedZipFileSize(qint64 bytes) noexcept;

        /**
         * @copydoc librepcb::NetworkRequestBase::setExpectedChecksum()
         */
        void setExpectedChecksum(QCryptographicHash::Algorithm algorithm,
                                 const QByteArray& checksum) noexcept;

        // Operator Overloadings
        LibraryDownload& operator=(const LibraryDownload& rhs) = delete;


    public slots:

        /**
         * @brief Start downloading the library
         */
        void start() noexcept;

        /**
         * @brief Abort downloading the library
         */
        void abort() noexcept;


    signals:

        void progressState(const QString& status);
        void progressPercent(int percent);
        void finished(bool success, const QString& errMsg);
        void abortRequested(); // internal signal!


    private: // Methods

        void downloadErrored(const QString& errMsg) noexcept;
        void downloadAborted() noexcept;
        void downloadSucceeded() noexcept;
        FilePath getPathToLibDir() noexcept;


    private: // Data

        QScopedPointer<FileDownload> mFileDownload;
        FilePath mDestDir;
        FilePath mTempDestDir;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WORKSPACE_LIBRARYDOWNLOAD_H
