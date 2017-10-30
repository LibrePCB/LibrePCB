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

#ifndef LIBREPCB_SMARTSEXPRFILE_H
#define LIBREPCB_SMARTSEXPRFILE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <memory>
#include <QtCore>
#include "smartfile.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class SExpression;

/*****************************************************************************************
 *  Class SmartSExprFile
 ****************************************************************************************/

/**
 * @brief The SmartSExprFile class represents an S-Expressions file and provides methods
 *        to load/save DOM trees (#DomDocument)
 *
 * With #parseFileAndBuildDomTree() the file can be parsed and a DOM tree is created.
 * With #save() the DOM tree can be saved back to the S-Expressions file.
 *
 * @note See class #SmartFile for more information.
 *
 * @author ubruhin
 * @date 2014-08-13
 */
class SmartSExprFile final : public SmartFile
{
        Q_DECLARE_TR_FUNCTIONS(SmartSExprFile)

    public:

        // Constructors / Destructor
        SmartSExprFile() = delete;
        SmartSExprFile(const SmartSExprFile& other) = delete;

        /**
         * @brief The constructor to open an existing S-Expressions file
         *
         * This constructor tries to open an existing file and throws an exception if an
         * error occurs.
         *
         * @param filepath  See SmartFile#SmartFile()
         * @param restore   See SmartFile#SmartFile()
         * @param readOnly  See SmartFile#SmartFile()
         *
         * @throw Exception If the specified text file could not be opened successful, an
         *                  exception will be thrown.
         */
        SmartSExprFile(const FilePath& filepath, bool restore, bool readOnly) :
            SmartSExprFile(filepath, restore, readOnly, false) {}

        /**
         * @copydoc SmartFile#~SmartFile()
         */
        ~SmartSExprFile() noexcept;


        // General Methods

        /**
         * @brief Open and parse the S-Expressions file and build the whole DOM tree
         *
         * @return  A pointer to the created DOM tree. The caller takes the ownership of
         *          the DOM document.
         */
        SExpression parseFileAndBuildDomTree() const;

        /**
         * @brief Write the S-Expressions DOM tree to the file system
         *
         * @param domDocument   The DOM document to save
         * @param toOriginal    Specifies whether the original or the backup file should
         *                      be overwritten/created.
         *
         * @throw Exception If an error occurs
         */
        void save(const SExpression& domDocument, bool toOriginal);


        // Operator Overloadings
        SmartSExprFile& operator=(const SmartSExprFile& rhs) = delete;


        // Static Methods

        /**
         * @brief Create a new S-Expressions file
         *
         * @note    This method will NOT immediately create the file! The file will be
         *          created after calling #save().
         *
         * @param filepath  The filepath to the file to create (always to the original file,
         *                  not to the backup file with "~" at the end of the filename!)
         *
         * @return The #SmartSExprFile object of the created file
         *
         * @throw Exception If an error occurs
         */
        static SmartSExprFile* create(const FilePath &filepath);


    private: // Methods

        /**
         * @brief Constructor to create or open a S-Expressions file
         *
         * @param filepath          See SmartFile#SmartFile()
         * @param restore           See SmartFile#SmartFile()
         * @param readOnly          See SmartFile#SmartFile()
         * @param create            See SmartFile#SmartFile()
         *
         * @throw Exception See SmartFile#SmartFile()
         */
        SmartSExprFile(const FilePath& filepath, bool restore, bool readOnly, bool create);

};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_SMARTSEXPRFILE_H
