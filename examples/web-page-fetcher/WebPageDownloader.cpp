#include "WebPageDownloader.h"

#include "QtSignalForwarder.h"

#include <QtConcurrentRun>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QFile>

#include <QtNetwork/QNetworkReply>

Q_DECLARE_METATYPE(QNetworkReply*)

PageFetcher::PageFetcher(QObject* parent)
	: QObject(parent)
{
	m_manager = new QNetworkAccessManager(this);
}

void PageFetcher::fetchPage(const QUrl& url, const QtCallback1<QByteArray>& callback)
{
	QNetworkReply* reply = m_manager->get(QNetworkRequest(url));

	QtCallback finishedCallback(this, SLOT(requestFinished(QNetworkReply*,QtCallback1<QByteArray>)));
	finishedCallback.bind(reply);
	finishedCallback.bind(callback);

	QtSignalForwarder::connect(reply, SIGNAL(finished()), finishedCallback);
}

void PageFetcher::requestFinished(QNetworkReply* reply, const QtCallback1<QByteArray>& callback)
{
	reply->deleteLater();
	callback.invoke(reply->readAll());
}

WebPageDownloader::WebPageDownloader()
	: m_pendingRequests(0)
{
	m_pageFetcher = new PageFetcher(this);
}

void WebPageDownloader::savePage(const QUrl& url, const QString& fileName)
{
	QtCallback1<QByteArray> callback(this, SLOT(fetchedPage(QString,QUrl,QByteArray)));
	callback.bind(fileName);
	callback.bind(url);
	m_pageFetcher->fetchPage(url, callback);

	++m_pendingRequests;
}

void WebPageDownloader::fetchedPage(const QString& fileName, const QUrl& url, const QByteArray& content)
{
	qDebug() << "Saved" << url.toString() << "to" << fileName;

	QFile file(fileName);
	file.open(QIODevice::WriteOnly);
	file.write(content);
	file.close();

	--m_pendingRequests;
	if (m_pendingRequests == 0) {
		emit finished();
	}
}

int main(int argc, char** argv)
{
	QCoreApplication app(argc,argv);

	WebPageDownloader client;
	client.savePage(QUrl("http://www.mendeley.com"), "mendeley.html");
	client.savePage(QUrl("http://www.google.com"), "google.html");

	QObject::connect(&client, SIGNAL(finished()), &app, SLOT(quit()));

	return app.exec();
}

