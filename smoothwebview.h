#ifndef SMOOTHWEBVIEW_HEADER
#define SMOOTHWEBVIEW_HEADER


#include <QGraphicsProxyWidget>

#include <KIcon>

#include <Plasma/Applet>
#include <Plasma/Svg>

#include <KWebView>

#include "ui_smoothwebviewconfig.h"

class SmoothWebView : public Plasma::Applet
{
	Q_OBJECT
	public:
		SmoothWebView(QObject *parent, const QVariantList &args);
		~SmoothWebView();
		
		void init();
		
		virtual void createConfigurationInterface(KConfigDialog* parent);
	protected:
		virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
		
		virtual bool sceneEventFilter(QGraphicsItem* watched, QEvent* event);
	public slots:
		void blockUpdate();
		void unblockUpdate();
		void unblockUpdateReal();
		void checkLeave();
		
		void linkClicked(const QUrl &url);
		
		void configAccepted();
		void updateTimerConnection();
		void updateColors();
		void updateScrollBars();
	private:
		KWebView *m_webView;
		QGraphicsProxyWidget *m_proxy;
		QTimer *m_timer;
		QTimer *m_leaveTimer;
		Ui_SmoothWebViewConfig m_configUi;
		
		bool m_doRefresh;
		KUrl m_url;
		bool m_hideScrollBars;
		bool m_invertColors;
		bool m_usePlasmaBackground;
};
 
K_EXPORT_PLASMA_APPLET(smoothwebview, SmoothWebView)
#endif
