#include "WebPageDownloader.h"

#include <QtConcurrentRun>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QFile>

#include <QtNetwork/QNetworkReply>

PageFetcher::PageFetcher(QObject* parent)
	: QObject(parent)
{
	m_manager = new QNetworkAccessManager(this);
}

void PageFetcher::fetchPage(const QUrl& url, const QtCallback1<QByteArray>& callback)
{
	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::User, QVariant::fromValue(callback));

	QNetworkReply* reply = m_manager->get(request);
	connect(reply, SIGNAL(finished()), this, SLOT(requestFinished()));
}

void PageFetcher::requestFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	QtCallback1<QByteArray> callback = reply->request().attribute(QNetworkRequest::User).value<QtCallback1<QByteArray> >();
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
	callback.bindArg(fileName);
	callback.bindArg(url);
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
	if (m_pendingRequests == 0)
	{
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

