/* AbiWord
 * Copyright (C) 1998 AbiSource, Inc.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 * 02111-1307, USA.
 */

#include <gtk/gtk.h>

#include "ut_types.h"
#include "ut_debugmsg.h"
#include "ut_assert.h"
#include "xap_ViewListener.h"
#include "ap_FrameData.h"
#include "xap_UnixFrame.h"
#include "ev_UnixToolbar.h"
#include "xav_View.h"
#include "xad_Document.h"
#include "fv_View.h"
#include "fl_DocLayout.h"
#include "pd_Document.h"
#include "gr_UnixGraphics.h"
#include "xap_Scrollbar_ViewListener.h"
#include "ap_UnixFrame.h"
#include "xap_UnixApp.h"
#include "ap_UnixTopRuler.h"
#include "ap_UnixLeftRuler.h"
#include "xap_UnixFontManager.h"
#include "ap_UnixStatusBar.h"

#ifdef ABISOURCE_LICENSED_TRADEMARKS
#include "swoosh_app_48.xpm"
#else
#include "abi_app_48.xpm"
#endif

/*****************************************************************/

#define REPLACEP(p,q)	do { if (p) delete p; p = q; } while (0)
#define ENSUREP(p)		do { UT_ASSERT(p); if (!p) goto Cleanup; } while (0)

/*****************************************************************/

void AP_UnixFrame::setZoomPercentage(UT_uint32 iZoom)
{
	_showDocument(iZoom);
}

UT_uint32 AP_UnixFrame::getZoomPercentage(void)
{
	return ((AP_FrameData*)m_pData)->m_pG->getZoomPercentage();
}

UT_Bool AP_UnixFrame::_showDocument(UT_uint32 iZoom)
{
	if (!m_pDoc)
	{
		UT_DEBUGMSG(("Can't show a non-existent document\n"));
		return UT_FALSE;
	}

	if (!((AP_FrameData*)m_pData))
	{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		return UT_FALSE;
	}

	GR_UnixGraphics * pG = NULL;
	FL_DocLayout * pDocLayout = NULL;
	AV_View * pView = NULL;
	AV_ScrollObj * pScrollObj = NULL;
	ap_ViewListener * pViewListener = NULL;
	AD_Document * pOldDoc = NULL;
	ap_Scrollbar_ViewListener * pScrollbarViewListener = NULL;
	AV_ListenerId lid;
	AV_ListenerId lidScrollbarViewListener;
	UT_uint32 nrToolbars;

	XAP_UnixFontManager * fontManager = ((XAP_UnixApp *) getApp())->getFontManager();
	
	pG = new GR_UnixGraphics(m_dArea->window, fontManager);
	ENSUREP(pG);
	pG->setZoomPercentage(iZoom);
	
	pDocLayout = new FL_DocLayout(static_cast<PD_Document *>(m_pDoc), pG);
	ENSUREP(pDocLayout);
  
//	pDocLayout->formatAll();

	pView = new FV_View(getApp(), this, pDocLayout);
	ENSUREP(pView);

	// The "AV_ScrollObj pScrollObj" receives
	// send{Vertical,Horizontal}ScrollEvents
	// from both the scroll-related edit methods
	// and from the UI callbacks.
	// 
	// The "ap_ViewListener pViewListener" receives
	// change notifications as the document changes.
	// This ViewListener is responsible for keeping
	// the title-bar up to date (primarily title
	// changes, dirty indicator, and window number).
	//
	// The "ap_Scrollbar_ViewListener pScrollbarViewListener"
	// receives change notifications as the doucment changes.
	// This ViewListener is responsible for recalibrating the
	// scrollbars as pages are added/removed from the document.
	//
	// Each Toolbar will also get a ViewListener so that
	// it can update toggle buttons, and other state-indicating
	// controls on it.
	//
	// TODO we ***really*** need to re-do the whole scrollbar thing.
	// TODO we have an addScrollListener() using an m_pScrollObj
	// TODO and a View-Listener, and a bunch of other widget stuff.
	// TODO and its very confusing.
	
	pScrollObj = new AV_ScrollObj(this,_scrollFuncX,_scrollFuncY);
	ENSUREP(pScrollObj);
	pViewListener = new ap_ViewListener(this);
	ENSUREP(pViewListener);
	pScrollbarViewListener = new ap_Scrollbar_ViewListener(this,pView);
	ENSUREP(pScrollbarViewListener);

	if (!pView->addListener(static_cast<AV_Listener *>(pViewListener),&lid))
		goto Cleanup;

	if (!pView->addListener(static_cast<AV_Listener *>(pScrollbarViewListener),
							&lidScrollbarViewListener))
		goto Cleanup;

	nrToolbars = m_vecToolbarLayoutNames.getItemCount();
	for (UT_uint32 k=0; k < nrToolbars; k++)
	{
		// TODO Toolbars are a frame-level item, but a view-listener is
		// TODO a view-level item.  I've bound the toolbar-view-listeners
		// TODO to the current view within this frame and have code in the
		// TODO toolbar to allow the view-listener to be rebound to a different
		// TODO view.  in the future, when we have support for multiple views
		// TODO in the frame (think splitter windows), we will need to have
		// TODO a loop like this to help change the focus when the current
		// TODO view changes.
		
		EV_UnixToolbar * pUnixToolbar = (EV_UnixToolbar *)m_vecUnixToolbars.getNthItem(k);
		pUnixToolbar->bindListenerToView(pView);
	}

	/****************************************************************
	*****************************************************************
	** If we reach this point, everything for the new document has
	** been created.  We can now safely replace the various fields
	** within the structure.  Nothing below this point should fail.
	*****************************************************************
	****************************************************************/
	
	// switch to new view, cleaning up previous settings
	if (((AP_FrameData*)m_pData)->m_pDocLayout)
	{
		pOldDoc = ((AP_FrameData*)m_pData)->m_pDocLayout->getDocument();
	}

	REPLACEP(((AP_FrameData*)m_pData)->m_pG, pG);
	REPLACEP(((AP_FrameData*)m_pData)->m_pDocLayout, pDocLayout);
	if (pOldDoc != m_pDoc)
	{
		UNREFP(pOldDoc);
	}
	REPLACEP(m_pView, pView);
	REPLACEP(m_pScrollObj, pScrollObj);
	REPLACEP(m_pViewListener, pViewListener);
	m_lid = lid;
	REPLACEP(m_pScrollbarViewListener,pScrollbarViewListener);
	m_lidScrollbarViewListener = lidScrollbarViewListener;

	m_pView->addScrollListener(m_pScrollObj);

	// Associate the new view with the existing TopRuler, LeftRuler.
	// Because of the binding to the actual on-screen widgets we do
	// not destroy and recreate the TopRuler, LeftRuler when we change
	// views, like we do for all the other objects.  We also do not
	// allocate the TopRuler, LeftRuler  here; that is done as the
	// frame is created.
	((AP_FrameData*)m_pData)->m_pTopRuler->setView(pView, iZoom);
	((AP_FrameData*)m_pData)->m_pLeftRuler->setView(pView, iZoom);
	((AP_FrameData*)m_pData)->m_pStatusBar->setView(pView);
	
	m_pView->setWindowSize(GTK_WIDGET(m_dArea)->allocation.width,
						   GTK_WIDGET(m_dArea)->allocation.height);
	setXScrollRange();
	setYScrollRange();
	updateTitle();

#if 1
	/*
	  UPDATE:  this code is back, but I'm leaving these comments as
	  an audit trail.  See bug 99.  This only happens when loading
	  a document into an empty window -- the case where a frame gets
	  reused.  TODO consider putting an expose into ap_EditMethods.cpp
	  instead of a draw() here.
	*/
	
	/*
	  I've removed this once again.  (Eric)  I replaced it with a call
	  to draw() which is now in the configure event handler in the GTK
	  section of the code.  See me if this causes problems.
	*/
	m_pView->draw();
#endif	

	((AP_FrameData*)m_pData)->m_pTopRuler->draw(NULL);
	((AP_FrameData*)m_pData)->m_pLeftRuler->draw(NULL);
	((AP_FrameData*)m_pData)->m_pStatusBar->draw();
	
	return UT_TRUE;

Cleanup:
	// clean up anything we created here
	DELETEP(pG);
	DELETEP(pDocLayout);
	DELETEP(pView);
	DELETEP(pViewListener);
	DELETEP(pScrollObj);
	DELETEP(pScrollbarViewListener);

	// change back to prior document
	UNREFP(m_pDoc);
	m_pDoc = ((AP_FrameData*)m_pData)->m_pDocLayout->getDocument();

	return UT_FALSE;
}

void AP_UnixFrame::setXScrollRange(void)
{
	int width = ((AP_FrameData*)m_pData)->m_pDocLayout->getWidth();
	int windowWidth = GTK_WIDGET(m_dArea)->allocation.width;

	int newvalue = ((m_pView) ? m_pView->getXScrollOffset() : 0);
	int newmax = width - windowWidth; /* upper - page_size */
	if (newmax <= 0)
		newvalue = 0;
	else if (newvalue > newmax)
		newvalue = newmax;

	UT_Bool bDifferentPosition = (newvalue != (int)m_pHadj->value);
	UT_Bool bDifferentLimits = ((width-windowWidth) != (m_pHadj->upper-m_pHadj->page_size));
	
	m_pHadj->value = newvalue;
	m_pHadj->lower = 0.0;
	m_pHadj->upper = (gfloat) width;
	m_pHadj->step_increment = 20.0;
	m_pHadj->page_increment = (gfloat) windowWidth;
	m_pHadj->page_size = (gfloat) windowWidth;
	gtk_signal_emit_by_name(GTK_OBJECT(m_pHadj), "changed");

	if (m_pView && (bDifferentPosition || bDifferentLimits))
		m_pView->sendHorizontalScrollEvent(newvalue, (long) m_pHadj->upper-m_pHadj->page_size);
}

void AP_UnixFrame::setYScrollRange(void)
{
	int height = ((AP_FrameData*)m_pData)->m_pDocLayout->getHeight();
	int windowHeight = GTK_WIDGET(m_dArea)->allocation.height;

	int newvalue = ((m_pView) ? m_pView->getYScrollOffset() : 0);
	int newmax = height - windowHeight;	/* upper - page_size */
	if (newmax <= 0)
		newvalue = 0;
	else if (newvalue > newmax)
		newvalue = newmax;

	UT_Bool bDifferentPosition = (newvalue != (int)m_pVadj->value);
	UT_Bool bDifferentLimits ((height-windowHeight) != (m_pVadj->upper-m_pVadj->page_size));
	
	m_pVadj->value = newvalue;
	m_pVadj->lower = 0.0;
	m_pVadj->upper = (gfloat) height;
	m_pVadj->step_increment = 20.0;
	m_pVadj->page_increment = (gfloat) windowHeight;
	m_pVadj->page_size = (gfloat) windowHeight;
	gtk_signal_emit_by_name(GTK_OBJECT(m_pVadj), "changed");

	if (m_pView && (bDifferentPosition || bDifferentLimits))
		m_pView->sendVerticalScrollEvent(newvalue, (long) m_pVadj->upper-m_pVadj->page_size);
}


AP_UnixFrame::AP_UnixFrame(XAP_UnixApp * app)
	: XAP_UnixFrame(app)
{
	// TODO
}

AP_UnixFrame::AP_UnixFrame(AP_UnixFrame * f)
	: XAP_UnixFrame(static_cast<XAP_UnixFrame *>(f))
{
	// TODO
}

AP_UnixFrame::~AP_UnixFrame(void)
{
	killFrameData();
}

UT_Bool AP_UnixFrame::initialize(void)
{
	if (!initFrameData())
		return UT_FALSE;

	if (!XAP_UnixFrame::initialize(AP_PREF_KEY_KeyBindings,AP_PREF_DEFAULT_KeyBindings,
								   AP_PREF_KEY_MenuLayout, AP_PREF_DEFAULT_MenuLayout,
								   AP_PREF_KEY_MenuLabelSet, AP_PREF_DEFAULT_MenuLabelSet,
								   AP_PREF_KEY_ToolbarLayouts, AP_PREF_DEFAULT_ToolbarLayouts,
								   AP_PREF_KEY_ToolbarLabelSet, AP_PREF_DEFAULT_ToolbarLabelSet))
		return UT_FALSE;

	_createTopLevelWindow();
	gtk_widget_show(m_wTopLevelWindow);

	return UT_TRUE;
}

/*****************************************************************/

UT_Bool AP_UnixFrame::initFrameData(void)
{
	UT_ASSERT(!((AP_FrameData*)m_pData));

	AP_FrameData* pData = new AP_FrameData();

	m_pData = (void*)pData;
	return (pData ? UT_TRUE : UT_FALSE);
}

void AP_UnixFrame::killFrameData(void)
{
	AP_FrameData* pData = (AP_FrameData*) m_pData;
	DELETEP(pData);
	m_pData = NULL;
}

UT_Bool AP_UnixFrame::_loadDocument(const char * szFilename, IEFileType ieft)
{
	// are we replacing another document?
	if (m_pDoc)
	{
		// yep.  first make sure it's OK to discard it, 
		// TODO: query user if dirty...
	}

	// load a document into the current frame.
	// if no filename, create a new document.

	AD_Document * pNewDoc = new PD_Document();
	UT_ASSERT(pNewDoc);
	
	if (!szFilename || !*szFilename)
	{
		pNewDoc->newDocument();
		m_iUntitled = _getNextUntitledNumber();
		goto ReplaceDocument;
	}

	if (pNewDoc->readFromFile(szFilename, ieft))
		goto ReplaceDocument;
	
	UT_DEBUGMSG(("ap_Frame: could not open the file [%s]\n",szFilename));
	UNREFP(pNewDoc);
	return UT_FALSE;

ReplaceDocument:
	getApp()->forgetClones(this);

	// NOTE: prior document is discarded in _showDocument()
	m_pDoc = pNewDoc;
	return UT_TRUE;
}
	
XAP_Frame * AP_UnixFrame::cloneFrame(void)
{
	AP_UnixFrame * pClone = new AP_UnixFrame(this);
	ENSUREP(pClone);

	if (!pClone->initialize())
		goto Cleanup;

	if (!pClone->_showDocument())
		goto Cleanup;

	pClone->show();

	return pClone;

Cleanup:
	// clean up anything we created here
	if (pClone)
	{
		m_pUnixApp->forgetFrame(pClone);
		delete pClone;
	}

	return NULL;
}

UT_Bool AP_UnixFrame::loadDocument(const char * szFilename, int ieft)
{
	UT_Bool bUpdateClones;
	UT_Vector vClones;
	XAP_App * pApp = getApp();

	bUpdateClones = (getViewNumber() > 0);
	if (bUpdateClones)
	{
		pApp->getClones(&vClones, this);
	}

	if (! _loadDocument(szFilename, (IEFileType) ieft))
	{
		// we could not load the document.
		// we cannot complain to the user here, we don't know
		// if the app is fully up yet.  we force our caller
		// to deal with the problem.
		return UT_FALSE;
	}

	pApp->rememberFrame(this);
	if (bUpdateClones)
	{
		for (UT_uint32 i = 0; i < vClones.getItemCount(); i++)
		{
			AP_UnixFrame * pFrame = (AP_UnixFrame *) vClones.getNthItem(i);
			if(pFrame != this)
			{
				pFrame->_replaceDocument(m_pDoc);
				pApp->rememberFrame(pFrame, this);
			}
		}
	}

	return _showDocument();
}

void AP_UnixFrame::_scrollFuncY(void * pData, UT_sint32 yoff, UT_sint32 /*yrange*/)
{
	// this is a static callback function and doesn't have a 'this' pointer.
	
	AP_UnixFrame * pUnixFrame = static_cast<AP_UnixFrame *>(pData);
	AV_View * pView = pUnixFrame->getCurrentView();
	
	// we've been notified (via sendVerticalScrollEvent()) of a scroll (probably
	// a keyboard motion).  push the new values into the scrollbar widgets
	// (with clamping).  then cause the view to scroll.
	
	gfloat yoffNew = (gfloat)yoff;
	gfloat yoffMax = pUnixFrame->m_pVadj->upper - pUnixFrame->m_pVadj->page_size;
	if (yoffMax <= 0)
		yoffNew = 0;
	else if (yoffNew > yoffMax)
		yoffNew = yoffMax;
	gtk_adjustment_set_value(GTK_ADJUSTMENT(pUnixFrame->m_pVadj),yoffNew);
	pView->setYScrollOffset((UT_sint32)yoffNew);
}

void AP_UnixFrame::_scrollFuncX(void * pData, UT_sint32 xoff, UT_sint32 /*xrange*/)
{
	// this is a static callback function and doesn't have a 'this' pointer.
	
	AP_UnixFrame * pUnixFrame = static_cast<AP_UnixFrame *>(pData);
	AV_View * pView = pUnixFrame->getCurrentView();
	
	// we've been notified (via sendScrollEvent()) of a scroll (probably
	// a keyboard motion).  push the new values into the scrollbar widgets
	// (with clamping).  then cause the view to scroll.

	gfloat xoffNew = (gfloat)xoff;
	gfloat xoffMax = pUnixFrame->m_pHadj->upper - pUnixFrame->m_pHadj->page_size;
	if (xoffMax <= 0)
		xoffNew = 0;
	else if (xoffNew > xoffMax)
		xoffNew = xoffMax;
	gtk_adjustment_set_value(GTK_ADJUSTMENT(pUnixFrame->m_pHadj),xoffNew);
	pView->setXScrollOffset((UT_sint32)xoffNew);
}

GtkWidget * AP_UnixFrame::_createDocumentWindow(void)
{
	GtkWidget * wSunkenBox;

	// create the top ruler
	AP_UnixTopRuler * pUnixTopRuler = new AP_UnixTopRuler(this);
	UT_ASSERT(pUnixTopRuler);
	m_topRuler = pUnixTopRuler->createWidget();
	((AP_FrameData*)m_pData)->m_pTopRuler = pUnixTopRuler;

	// create the left ruler
	AP_UnixLeftRuler * pUnixLeftRuler = new AP_UnixLeftRuler(this);
	UT_ASSERT(pUnixLeftRuler);
	m_leftRuler = pUnixLeftRuler->createWidget();
	((AP_FrameData*)m_pData)->m_pLeftRuler = pUnixLeftRuler;

	// get the width from the left ruler and stuff it into the top ruler.
	
	pUnixTopRuler->setOffsetLeftRuler(pUnixLeftRuler->getWidth());
	
	// set up for scroll bars.
	m_pHadj = (GtkAdjustment*) gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	gtk_object_set_user_data(GTK_OBJECT(m_pHadj),this);
	m_hScroll = gtk_hscrollbar_new(m_pHadj);
	gtk_object_set_user_data(GTK_OBJECT(m_hScroll),this);

	gtk_signal_connect(GTK_OBJECT(m_pHadj), "value_changed", GTK_SIGNAL_FUNC(_fe::hScrollChanged), NULL);

	m_pVadj = (GtkAdjustment*) gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	gtk_object_set_user_data(GTK_OBJECT(m_pVadj),this);
	m_vScroll = gtk_vscrollbar_new(m_pVadj);
	gtk_object_set_user_data(GTK_OBJECT(m_vScroll),this);

	gtk_signal_connect(GTK_OBJECT(m_pVadj), "value_changed", GTK_SIGNAL_FUNC(_fe::vScrollChanged), NULL);

	// we don't want either scrollbar grabbing events from us
	GTK_WIDGET_UNSET_FLAGS(m_hScroll, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS(m_vScroll, GTK_CAN_FOCUS);

	// create a drawing area in the for our document window.
	m_dArea = gtk_drawing_area_new();
	
	gtk_object_set_user_data(GTK_OBJECT(m_dArea),this);
	gtk_widget_set_events(GTK_WIDGET(m_dArea), (GDK_EXPOSURE_MASK |
												GDK_BUTTON_PRESS_MASK |
												GDK_POINTER_MOTION_MASK |
												GDK_BUTTON_RELEASE_MASK |
												GDK_KEY_PRESS_MASK |
												GDK_KEY_RELEASE_MASK));

	gtk_signal_connect(GTK_OBJECT(m_dArea), "expose_event",
					   GTK_SIGNAL_FUNC(_fe::expose), NULL);
  
	gtk_signal_connect(GTK_OBJECT(m_dArea), "button_press_event",
					   GTK_SIGNAL_FUNC(_fe::button_press_event), NULL);

	gtk_signal_connect(GTK_OBJECT(m_dArea), "button_release_event",
					   GTK_SIGNAL_FUNC(_fe::button_release_event), NULL);

	gtk_signal_connect(GTK_OBJECT(m_dArea), "motion_notify_event",
					   GTK_SIGNAL_FUNC(_fe::motion_notify_event), NULL);
  
	gtk_signal_connect(GTK_OBJECT(m_dArea), "configure_event",
					   GTK_SIGNAL_FUNC(_fe::configure_event), NULL);

	// create a table for scroll bars, rulers, and drawing area

	m_table = gtk_table_new(3, 3, FALSE);
	gtk_object_set_user_data(GTK_OBJECT(m_table),this);

	// arrange the widgets within our table.
	
	gtk_table_attach(GTK_TABLE(m_table), m_topRuler, 0, 2, 0, 1,
					 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
					 (GtkAttachOptions)(GTK_FILL),
					 0,0);

	gtk_table_attach(GTK_TABLE(m_table), m_leftRuler, 0, 1, 1, 2,
					 (GtkAttachOptions)(GTK_FILL),
					 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
					 0,0);

	gtk_table_attach(GTK_TABLE(m_table), m_dArea,   1, 2, 1, 2,
					 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					 0, 0); 
	gtk_table_attach(GTK_TABLE(m_table), m_hScroll, 0, 2, 2, 3,
					 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					 (GtkAttachOptions) (GTK_FILL),
					 0, 0);
	gtk_table_attach(GTK_TABLE(m_table), m_vScroll, 2, 3, 0, 2,
					 (GtkAttachOptions) (GTK_FILL),
					 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					 0, 0);

	// create a 3d box and put the table in it, so that we
	// get a sunken in look.
	wSunkenBox = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(wSunkenBox), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(wSunkenBox), m_table);

	gtk_widget_show(m_hScroll);
	gtk_widget_show(m_vScroll);
	gtk_widget_show(m_dArea);
	gtk_widget_show(m_table);

	return wSunkenBox;
}

void AP_UnixFrame::translateDocumentToScreen(UT_sint32 &x, UT_sint32 &y)
{
	// translate the given document mouse coordinates into absolute screen coordinates.

	Window child;
	gint tx;
	gint ty;
  
	GdkWindowPrivate * priv = (GdkWindowPrivate*) m_dArea->window;
	if (!priv->destroyed)
		XTranslateCoordinates (priv->xdisplay, priv->xwindow, gdk_root_window, x, y, &tx, &ty, &child);
  
	x = tx;
	y = ty;
}

GtkWidget * AP_UnixFrame::_createStatusBarWindow(void)
{
	AP_UnixStatusBar * pUnixStatusBar = new AP_UnixStatusBar(this);
	UT_ASSERT(pUnixStatusBar);

	((AP_FrameData *)m_pData)->m_pStatusBar = pUnixStatusBar;
	
	GtkWidget * w = pUnixStatusBar->createWidget();

	return w;
}

void AP_UnixFrame::setStatusMessage(const char * szMsg)
{
	((AP_FrameData *)m_pData)->m_pStatusBar->setStatusMessage(szMsg);
}

void AP_UnixFrame::_setWindowIcon(void)
{
	// attach program icon to window
	GtkWidget * window = getTopLevelWindow();
	UT_ASSERT(window);

	// create a pixmap from our included data
	GdkBitmap * mask;
	GdkPixmap * pixmap = gdk_pixmap_create_from_xpm_d(window->window,
													  &mask,
													  NULL,
													  abi_app_48_xpm);
	UT_ASSERT(pixmap && mask);
		
	gdk_window_set_icon(window->window, NULL, pixmap, mask);
	gdk_window_set_icon_name(window->window, "AbiWord Application Icon");
}

UT_Bool AP_UnixFrame::_replaceDocument(AD_Document * pDoc)
{
	// NOTE: prior document is discarded in _showDocument()
	m_pDoc = REFP(pDoc);

	return _showDocument();
}
