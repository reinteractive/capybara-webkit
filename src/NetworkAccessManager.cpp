#include "NetworkAccessManager.h"
#include "WebPage.h"
#include <iostream>


NetworkAccessManager::NetworkAccessManager(QObject *parent):QNetworkAccessManager(parent) {
}

QNetworkReply* NetworkAccessManager::createRequest(QNetworkAccessManager::Operation operation, const QNetworkRequest &request, QIODevice * outgoingData = 0) {
  QNetworkRequest new_request(request);

  /* Since we use headers to set the authentication,
   * we can get into a state where QT caches invalid/blank
   * credentials for a page, which then overwrites the
   * Authorization header. This was reported as a bug
   * in  http://help.stillalive.com/discussions/questions/145-intermittently-failing-scripts
   *
   * Setting AuthenticationReuse to manual prevents this overwrite.
   *
   * We can only do this if a proxy is not set, since proxy-auth relies
   * on AuthenticationReuse.
   * Since we only everset a proxy to QNetworkProxy::HttpProxy we can
   * be specific about this. It also won't affect Still Alive since we
   * don't use a proxy, but it gets the tests passing at least.
   *
   * TODO: fix this once authentication caches can be cleared
   */
  if (this->proxy().type() != QNetworkProxy::HttpProxy) {
    new_request.setAttribute(QNetworkRequest::AuthenticationReuseAttribute,  QNetworkRequest::Manual);
  }

  if (this->isBlacklisted(new_request.url())) {
    return this->noOpRequest();
  } else {
    if (operation != QNetworkAccessManager::PostOperation && operation != QNetworkAccessManager::PutOperation) {
      new_request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant());
    }
    QHashIterator<QString, QString> item(m_headers);
    while (item.hasNext()) {
        item.next();
        new_request.setRawHeader(item.key().toAscii(), item.value().toAscii());
    }
    return QNetworkAccessManager::createRequest(operation, new_request, outgoingData);
  }
};

void NetworkAccessManager::addHeader(QString key, QString value) {
  m_headers.insert(key, value);
};

void NetworkAccessManager::setUrlBlacklist(QStringList urlBlacklist) {
  m_urlBlacklist.clear();

  QStringListIterator iter(urlBlacklist);
  while (iter.hasNext()) {
    m_urlBlacklist << QUrl(iter.next());
  }
};

bool NetworkAccessManager::isBlacklisted(QUrl url) {
  QListIterator<QUrl> iter(m_urlBlacklist);

  while (iter.hasNext()) {
    QUrl blacklisted = iter.next();

    if (blacklisted == url) {
      return true;
    } else if (blacklisted.path().isEmpty() && blacklisted.isParentOf(url)) {
      return true;
    }
  }

  return false;
};

QNetworkReply* NetworkAccessManager::noOpRequest() {
  return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation, QNetworkRequest(QUrl()));
};
void NetworkAccessManager::resetHeaders() {
  m_headers.clear();
};

