/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef PROJECT_GENERICCOMPONENTINSTANCE_H
#define PROJECT_GENERICCOMPONENTINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QDomElement>
#include "../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Circuit;
class GenCompSignalInstance;
class SymbolInstance;
}

namespace library {
class GenericComponent;
class GenCompSymbVar;
}

/*****************************************************************************************
 *  Class GenericComponentInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The GenericComponentInstance class
 */
class GenericComponentInstance : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GenericComponentInstance(Circuit& circuit, const QDomElement& domElement)
                                          throw (Exception);
        ~GenericComponentInstance() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        unsigned int getUsedSymbolsCount() const noexcept {return mSymbolInstances.count();}
        GenCompSignalInstance* getSignalInstance(const QUuid& signalUuid) const noexcept {return mSignals.value(signalUuid);}
        const library::GenericComponent& getGenComp() const noexcept {return *mGenComp;}
        const library::GenCompSymbVar& getSymbolVariant() const noexcept {return *mGenCompSymbVar;}


        // Setters

        /**
         * @brief Set the name of this generic component instance in the circuit
         *
         * @warning You have to check if there is no other component with the same name in
         *          the whole circuit! This method will not check if the name is unique.
         * @warning This method must always be called from inside an UndoCommand!
         *
         * @param name  The new name of this component in the circuit (must not be empty)
         *
         * @throw Exception If the new name is invalid, an exception will be thrown
         */
        void setName(const QString& name) throw (Exception);


        // General Methods
        void addToCircuit(bool addNode, QDomElement& parent) throw (Exception);
        void removeFromCircuit(bool removeNode, QDomElement& parent) throw (Exception);
        void registerSymbolInstance(const QUuid& itemUuid, const QUuid& symbolUuid,
                                    const SymbolInstance* instance) throw (Exception);
        void unregisterSymbolInstance(const QUuid& itemUuid, const SymbolInstance* symbol)
                                      throw (Exception);

        // Static Methods
        static GenericComponentInstance* create(Circuit& circuit, QDomDocument& doc,
                                                const library::GenericComponent& genComp,
                                                const library::GenCompSymbVar& symbVar,
                                                const QString& name) throw (Exception);


    private:

        // make some methods inaccessible...
        GenericComponentInstance();
        GenericComponentInstance(const GenericComponentInstance& other);
        GenericComponentInstance& operator=(const GenericComponentInstance& rhs);


        // General
        Circuit& mCircuit;
        QDomElement mDomElement;
        bool mAddedToCircuit;


        // Attributes

        /// @brief The unique UUID of this component instance in the circuit
        QUuid mUuid;

        /// @brief The unique name of this component instance in the circuit (e.g. "R42")
        QString mName;

        /// @brief Pointer to the generic component in the project's library
        const library::GenericComponent* mGenComp;

        /// @brief Pointer to the used symbol variant of #mGenComp
        const library::GenCompSymbVar* mGenCompSymbVar;

        /// @brief All signal instances (Key: generic component signal UUID)
        QHash<QUuid, GenCompSignalInstance*> mSignals;

        /**
         * @brief All registered symbol instances (must be empty if this generic component
         *        instance is not added to circuit)
         *
         * - Key:   UUID of the symbol variant item (library#GenCompSymbVarItem)
         * - Value: Pointer to the registered symbol instance
         *
         * @see #registerSymbolInstance(), #unregisterSymbolInstance()
         */
        QHash<QUuid, const SymbolInstance*> mSymbolInstances;
};

} // namespace project

#endif // PROJECT_GENERICCOMPONENTINSTANCE_H
