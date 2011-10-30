#include "smoothwebview.h"
#include <QPainter>
#include <QFontMetrics>
#include <QSizeF>
#include <QRectF>
#include <QGraphicsSceneEvent>
#include <QApplication>
#include <QGraphicsGridLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QTimer>
#include <QMenu>
#include <QWebFrame>
#include <QGraphicsEffect>

#include <KLocale>
#include <KRun>
#include <KConfigDialog>

#include <plasma/svg.h>
#include <plasma/theme.h>

#include <stdio.h>

#include "ui_smoothwebviewconfig.h"

class InvertGraphicsEffect : public QGraphicsEffect
{
	protected:
		virtual void draw(QPainter* painter)
		{
			QPoint offset;
			
			QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset);
			
			QImage img = pixmap.toImage();
			
			if(img.format() != QImage::Format_ARGB32_Premultiplied && img.format() != QImage::Format_RGB32)
			{
				img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
			}
			
			img.invertPixels();
			
			painter->drawImage(offset, img);
		}
};


SmoothWebView::SmoothWebView(QObject *parent, const QVariantList &args)
	: Plasma::Applet(parent, args)
{
	setBackgroundHints(DefaultBackground);
	
	setHasConfigurationInterface(true);
	resize(200, 200);
	
	m_timer = new QTimer(this);
	m_leaveTimer = new QTimer(this);
}


SmoothWebView::~SmoothWebView()
{
	if(hasFailedToLaunch())
	{
		// Do some cleanup here
	}
	else
	{
		// Save settings
	}
}

void SmoothWebView::init()
{
	setAspectRatioMode(Plasma::IgnoreAspectRatio);
	
	m_webView = new KWebView();
	
	// Proxy widget
	m_proxy = new QGraphicsProxyWidget(this);
	m_proxy->setWidget(m_webView);
	m_proxy->installSceneEventFilter(this);
	
	// Layout
	QGraphicsGridLayout *layout = new QGraphicsGridLayout(this);
	setLayout(layout);
	layout->addItem(m_proxy, 0, 0);
	
	// WebView configuration
	m_url = config().readEntry("url", KUrl("http://www.kde.org"));
	m_webView->load(m_url);
	m_webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
	
	connect(m_webView->page(), SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
	connect(m_webView->page(), SIGNAL(loadStarted()), this, SLOT(blockUpdate()));
	connect(m_webView->page(), SIGNAL(loadFinished(bool)), this, SLOT(unblockUpdate()));
	
	m_timer->setInterval(config().readEntry("interval", 30) * 1000);
	m_timer->start();
	m_doRefresh = config().readEntry("doRefresh", true);
	updateTimerConnection();
	
	m_leaveTimer->setInterval(333);
	connect(m_leaveTimer, SIGNAL(timeout()), this, SLOT(checkLeave()));
	
	setAssociatedApplication("kfmclient %u");
	KUrl::List list;
	list.append(config().readEntry("associatedUrl", KUrl("http://www.google.de")));
	setAssociatedApplicationUrls(list);
	
	m_invertColors = config().readEntry("invertColors", false);
	m_usePlasmaBackground = config().readEntry("usePlasmaBackground", false);
	updateColors();
	
	m_hideScrollBars = config().readEntry("hideScrollBars", false);
	updateScrollBars();
	
	setCacheMode(NoCache);
}

void SmoothWebView::blockUpdate()
{
	m_webView->setUpdatesEnabled(false);
}

void SmoothWebView::unblockUpdate()
{
	QTimer::singleShot(200, this, SLOT(unblockUpdateReal()));
}

void SmoothWebView::unblockUpdateReal()
{
	m_webView->setUpdatesEnabled(true);
	m_webView->update();
}

void SmoothWebView::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	m_timer->stop();
	unblockUpdateReal();
	
	Plasma::Applet::hoverEnterEvent(event);
}

void SmoothWebView::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	m_leaveTimer->start();
	
	Plasma::Applet::hoverLeaveEvent(event);
}

void SmoothWebView::checkLeave()
{
	if(!isUnderMouse())
	{
		m_timer->start();
	}
	
	m_leaveTimer->stop();
}


bool SmoothWebView::sceneEventFilter(QGraphicsItem* item, QEvent* event)
{
	if(event->type() == QEvent::GraphicsSceneHoverMove)
	{
		m_proxy->setCursor(m_webView->cursor());
		
		m_leaveTimer->stop();
	}
	else if(event->type() == QEvent::GraphicsSceneContextMenu)
	{
		QGraphicsSceneContextMenuEvent *cevent = static_cast<QGraphicsSceneContextMenuEvent*>(event);
		
		// Let's see if JavaScript etc. wants the event
		QContextMenuEvent fakeEvent(QContextMenuEvent::Reason(cevent->reason()), cevent->pos().toPoint());
		
		if(m_webView->page()->swallowContextMenuEvent(&fakeEvent))
			return true;
		
		// Create a custom context menu just for us
		m_webView->page()->updatePositionDependentActions(cevent->pos().toPoint());
		
		// And get it...
		QMenu *contextMenu = m_webView->page()->createStandardContextMenu();
		
		if(contextMenu)
		{
			// Problem is, the QMenu already has a QGraphicsWidgetProxy, and we can't delete it.
			// So we need to copy the actions to our own menu...
			QMenu myContextMenu;
			
			foreach(QAction *action, contextMenu->actions())
			{
				myContextMenu.addAction(action);
			}
			
			myContextMenu.exec(cevent->screenPos());
			
			delete contextMenu;
		}
		
		return true;
	}
	
	return Applet::sceneEventFilter(item, event);
}

void SmoothWebView::linkClicked(const QUrl& url)
{
	new KRun(url, 0);
}

void SmoothWebView::createConfigurationInterface(KConfigDialog* parent)
{
	QWidget *w = new QWidget(parent);
	
	m_configUi.setupUi(w);
	
	m_configUi.intervalSpinBox->setValue(m_timer->interval() / 1000);
	m_configUi.refreshCheckBox->setChecked(m_doRefresh);
	m_configUi.url->setUrl(m_url);
	m_configUi.associatedUrl->setUrl(associatedApplicationUrls()[0]);
	m_configUi.invertColorsCheckBox->setChecked(m_invertColors);
	m_configUi.plasmaBackgroundCheckbox->setChecked(m_usePlasmaBackground);
	m_configUi.hideScrollBarsCheckBox->setChecked(m_hideScrollBars);
	
	parent->addPage(w, i18n("General"), icon());
	
	connect(parent, SIGNAL(accepted()), this, SLOT(configAccepted()));
	
	Plasma::Applet::createConfigurationInterface(parent);
}

void SmoothWebView::configAccepted()
{
	bool changed = false;
	
	if(m_configUi.intervalSpinBox->value() != m_timer->interval() / 1000)
	{
		m_timer->setInterval(m_configUi.intervalSpinBox->value() * 1000);
		config().writeEntry("interval", m_timer->interval() / 1000);
		changed = true;
	}
	
	if(m_configUi.refreshCheckBox->isChecked() != m_doRefresh)
	{
		m_doRefresh = m_configUi.refreshCheckBox->isChecked();
		updateTimerConnection();
		config().writeEntry("doRefresh", m_doRefresh);
		changed = true;
	}
	
	if(m_configUi.url->url() != m_url)
	{
		m_url = m_configUi.url->url();
		m_webView->load(m_url);
		config().writeEntry("url", m_url);
		changed = true;
	}
	
	if(m_configUi.associatedUrl->url() != associatedApplicationUrls()[0])
	{
		KUrl url = m_configUi.associatedUrl->url();
		KUrl::List list;
		list.append(url);
		setAssociatedApplicationUrls(list);
		config().writeEntry("associatedUrl", url);
		changed = true;
	}
	
	if(   m_configUi.invertColorsCheckBox->isChecked() != m_invertColors
	   || m_configUi.plasmaBackgroundCheckbox->isChecked() != m_usePlasmaBackground)
	{
		m_invertColors = m_configUi.invertColorsCheckBox->isChecked();
		m_usePlasmaBackground = m_configUi.plasmaBackgroundCheckbox->isChecked();
		updateColors();
		config().writeEntry("invertColors", m_invertColors);
		config().writeEntry("usePlasmaBackground", m_usePlasmaBackground);
		changed = true;
	}
	
	if(m_configUi.hideScrollBarsCheckBox->isChecked() != m_hideScrollBars)
	{
		m_hideScrollBars = m_configUi.hideScrollBarsCheckBox->isChecked();
		updateScrollBars();
		config().writeEntry("hideScrollBars", m_hideScrollBars);
		changed = true;
	}
	
	if(changed)
	{
		emit configNeedsSaving();
	}
}

void SmoothWebView::updateTimerConnection()
{
	if(m_doRefresh)
		connect(m_timer, SIGNAL(timeout()), m_webView, SLOT(reload()), Qt::UniqueConnection);
	else
		disconnect(m_timer, SIGNAL(timeout()), m_webView, SLOT(reload()));
}

void SmoothWebView::updateColors()
{
	if(m_invertColors)
	{
		m_proxy->setGraphicsEffect(new InvertGraphicsEffect());
	}
	else
	{
		m_proxy->setGraphicsEffect(0);
	}
	
	if(!m_invertColors && m_usePlasmaBackground)
	{
		printf("Changing palette...\n");
		QPalette p = m_webView->page()->palette();
		p.setBrush(QPalette::Base, Qt::transparent);
		m_webView->setPalette(p);
	}
	else
	{
		m_webView->page()->setPalette(palette());
	}
}

void SmoothWebView::updateScrollBars()
{
	QWebFrame *frame = m_webView->page()->mainFrame();
	
	if(m_hideScrollBars)
	{
		frame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
		frame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
	}
	else
	{
		frame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAsNeeded);
		frame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAsNeeded);
	}
}

#include "smoothwebview.moc"
