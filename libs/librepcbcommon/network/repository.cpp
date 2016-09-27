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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "repository.h"
#include "fileio/xmldomelement.h"
#include "network/networkrequest.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Repository::Repository(const Repository& other) noexcept :
    QObject(nullptr), mUrl(other.mUrl)
{
}

Repository::Repository(const QUrl& url) noexcept :
    QObject(nullptr), mUrl(url)
{
}

Repository::Repository(const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mUrl()
{
    mUrl = domElement.getAttribute<QUrl>("url", true); // can throw
}

Repository::~Repository() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

bool Repository::setUrl(const QUrl& url) noexcept
{
    if (url.isValid()) {
        mUrl = url;
        return true;
    } else {
        return false;
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Repository::requestRepositoryList() noexcept
{
    requestRepositoryList(mUrl.resolved(QUrl("api/v1/libraries/")));
}

XmlDomElement* Repository::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("repository"));
    root->setAttribute("url", mUrl);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Repository::requestRepositoryList(const QUrl& url) noexcept
{
    NetworkRequest* request = new NetworkRequest(url);
    request->setHeaderField("Accept", "application/json;charset=UTF-8");
    request->setHeaderField("Accept-Charset", "UTF-8");
    connect(request, &NetworkRequest::errored,
            this, &Repository::errorWhileFetchingRepositoryList, Qt::QueuedConnection);
    connect(request, &NetworkRequest::dataReceived,
            this, &Repository::requestedDataReceived, Qt::QueuedConnection);
    request->start();
}

void Repository::requestedDataReceived(const QByteArray& data) noexcept
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
        emit errorWhileFetchingRepositoryList(tr("Received JSON object is not valid."));
        return;
    }
    QJsonValue nextResultsLink = doc.object().value("next");
    if (nextResultsLink.isString()) {
        QUrl url = QUrl(nextResultsLink.toString());
        if (url.isValid()) {
            qDebug() << "Request more results from repository:" << url.toString();
            requestRepositoryList(url);
        } else {
            qWarning() << "Invalid URL in received JSON object:" << nextResultsLink.toString();
        }
    }
    QJsonValue reposVal = doc.object().value("results");
    if ((reposVal.isNull()) || (!reposVal.isArray())) {
        emit errorWhileFetchingRepositoryList(tr("Received JSON object does not contain "
                                                 "any results."));
        return;
    }
    emit repositoryListReceived(reposVal.toArray());
}

bool Repository::checkAttributesValidity() const noexcept
{
    if (!mUrl.isValid()) return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
