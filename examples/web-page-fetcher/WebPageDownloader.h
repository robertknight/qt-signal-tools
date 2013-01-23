#pragma once

#include "QtCallback.h"

#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>

class PageFetcher : public QObject
{
	Q_OBJECT

	public:
		PageFetcher(QObject* parent = 0);

		/** Fetch the content of a URL and invoke a given callback when completed. */
		void fetchPage(const QUrl& url, const QtCallback1<QByteArray>& callback);

	private Q_SLOTS:
		void requestFinished(QNetworkReply*, const QtCallback1<QByteArray>& callback);

	private:
		QNetworkAccessManager* m_manager;
};

Q_DECLARE_METATYPE(QtCallback1<QByteArray>);

/** Downloads web pages and saves them to a file.
 *  This is acts as a client of the PageFetcher class.
 */
class WebPageDownloader : public QObject
{
	Q_OBJECT

	public:
		WebPageDownloader();

		void savePage(const QUrl& url, const QString& fileName);

	Q_SIGNALS:
		void finished();

	private Q_SLOTS:
		void fetchedPage(const QString& fileName, const QUrl& url, const QByteArray& content);

	private:
		PageFetcher* m_pageFetcher;

		int m_pendingRequests;
};

