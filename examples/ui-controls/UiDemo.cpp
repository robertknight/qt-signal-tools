#include "QtCallbackProxy.h"

#include <QtGui/QApplication>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QSlider>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	QWidget* widget = new QWidget;
	
	QVBoxLayout* layout = new QVBoxLayout(widget);

	QPushButton* button1 = new QPushButton("Set to 10%");
	QPushButton* button2 = new QPushButton("Set to 50%");
	QPushButton* button3 = new QPushButton("Set to 80%");

	QSlider* slider = new QSlider(Qt::Horizontal);

	// create a callback object which sets the slider's value
	// and use QtCallbackProxy to invoke it when the button is
	// clicked
	QtCallback1<int> button1Callback(slider, SLOT(setValue(int)));
	button1Callback.bind(10);
	QtCallbackProxy::connectCallback(button1, SIGNAL(clicked(bool)), button1Callback);

	// setup another couple of buttons.
	// Here we use a more succinct syntax to create the callback
	QtCallbackProxy::connectCallback(button2, SIGNAL(clicked(bool)),
	  QtCallback1<int>(slider, SLOT(setValue(int))).bind(50));

	QtCallbackProxy::connectCallback(button3, SIGNAL(clicked(bool)),
	  QtCallback1<int>(slider, SLOT(setValue(int))).bind(80));

	layout->addWidget(button1);
	layout->addWidget(button2);
	layout->addWidget(button3);
	layout->addWidget(slider);
	
	layout->addStretch();

	widget->show();

	return app.exec();
}

