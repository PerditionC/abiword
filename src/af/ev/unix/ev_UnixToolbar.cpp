/* AbiSource Program Utilities
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
#include <string.h>
#include <stdlib.h>
#include "ut_assert.h"
#include "ut_debugmsg.h"
#include "ut_string.h"
#include "ev_UnixToolbar.h"
#include "xap_Types.h"
#include "xap_UnixApp.h"
#include "xap_UnixFrame.h"
#include "ev_Toolbar_Actions.h"
#include "ev_Toolbar_Layouts.h"
#include "ev_Toolbar_Labels.h"
#include "ev_Toolbar_Control.h"
#include "ev_EditEventMapper.h"
#include "xap_UnixToolbar_Icons.h"
#include "ev_UnixToolbar_ViewListener.h"
#include "xav_View.h"
#include "xap_Prefs.h"
#include "xap_EncodingManager.h"


/*****************************************************************/
#define COMBO_BUF_LEN 256

static GtkTargetEntry      s_AbiTBTargets[] = {{"abi-toolbars",0,0}};


/**
 * toolbar_append_with_eventbox
 *
 * Borrowed code from gnumeric (src/gnumeric-util.c)
 *
 * @toolbar               toolbar
 * @widget                widget to insert
 * @tooltip_text          tooltip text
 * @tooltip_private_text  longer tooltip text
 *
 * Packs widget in an eventbox and adds the eventbox to toolbar.
 * This lets a windowless widget (e.g. combo box) have tooltips.
 **/
static GtkWidget *
toolbar_append_with_eventbox (GtkToolbar *toolbar, GtkWidget  *widget,
			      const char *tooltip_text,
			      const char *tooltip_private_text)
{
	GtkWidget *eventbox;

	UT_ASSERT(GTK_IS_TOOLBAR (toolbar));
	UT_ASSERT(widget != NULL);

	/* An event box to receive events - this is a requirement for having
           tooltips */
	eventbox = gtk_event_box_new ();
	gtk_widget_show (widget);
	gtk_container_add (GTK_CONTAINER (eventbox), widget);
	gtk_widget_show (eventbox);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), eventbox,
				   tooltip_text, tooltip_private_text);
	return eventbox;
}

class _wd								// a private little class to help
{										// us remember all the widgets that
public:									// we create...
	_wd(EV_UnixToolbar * pUnixToolbar, XAP_Toolbar_Id id, GtkWidget * widget = NULL)
	{
		m_pUnixToolbar = pUnixToolbar;
		m_id = id;
		m_widget = widget;
		m_blockSignal = false;
		m_comboEntryBuffer[0] = 0;
	};
	
	~_wd(void)
	{
	};

	static void s_callback(GtkWidget * /* widget */, gpointer user_data)
	{
		// this is a static callback method and does not have a 'this' pointer.
		// map the user_data into an object and dispatch the event.
	
		_wd * wd = (_wd *) user_data;
		UT_ASSERT(wd);
		GdkEvent * event = gtk_get_current_event();
		wd->m_pUnixToolbar->setCurrentEvent(event);
		UT_DEBUGMSG(("SEVIOR: in s_callback \n"));
		if (!wd->m_blockSignal)
			wd->m_pUnixToolbar->toolbarEvent(wd, 0, 0);
	};


	static void s_drag_begin(GtkWidget  *widget,
							GdkDragContext     *context)
	{
		_wd * wd = (_wd *) gtk_object_get_data(GTK_OBJECT(widget),"wd_pointer");
		UT_DEBUGMSG(("SEVIOR: Begin drag at icon id %d \n",wd->m_id));
		XAP_Frame * pFrame = static_cast<XAP_Frame *>(wd->m_pUnixToolbar->getFrame());
	    EV_Toolbar * pTBsrc = (EV_Toolbar *) wd->m_pUnixToolbar;
		pFrame->dragBegin(wd->m_id,pTBsrc);
	};


	static void s_drag_drop(GtkWidget  *widget,
							GdkDragContext     *context,
							gint x, gint y, guint time )
	{
		_wd * wd = (_wd *) gtk_object_get_data(GTK_OBJECT(widget),"wd_pointer");
		GtkWidget * src = gtk_drag_get_source_widget(context);
		_wd * wdSrc = (_wd *)  gtk_object_get_data(GTK_OBJECT(src),"wd_pointer");
		UT_DEBUGMSG(("SEVIOR: Drop at icon id %d source icon %d \n",wd->m_id,wdSrc->m_id));
		
		XAP_Frame * pFrame = static_cast<XAP_Frame *>(wd->m_pUnixToolbar->getFrame());
	    EV_Toolbar * pTBdest = (EV_Toolbar *) wd->m_pUnixToolbar;
	    EV_Toolbar * pTBsrc = (EV_Toolbar *) wdSrc->m_pUnixToolbar;
		pFrame->dragDropToIcon(wdSrc->m_id,wd->m_id,pTBsrc,pTBdest);
	};

	static void s_drag_drop_toolbar(GtkWidget  *widget,
							GdkDragContext     *context,
							gint x, gint y, guint time, gpointer pTB)
	{
		GtkWidget * src = gtk_drag_get_source_widget(context);
		_wd * wdSrc = (_wd *)  gtk_object_get_data(GTK_OBJECT(src),"wd_pointer");
		UT_DEBUGMSG(("SEVIOR: Drop  icon on toolbar source icon %d \n",wdSrc->m_id));
		XAP_Frame * pFrame = static_cast<XAP_Frame *>(wdSrc->m_pUnixToolbar->getFrame());
	    EV_Toolbar * pTBsrc = (EV_Toolbar *) wdSrc->m_pUnixToolbar;
	    EV_Toolbar * pTBdest = (EV_Toolbar *) pTB;
		pFrame->dragDropToTB(wdSrc->m_id,pTBsrc,pTBdest);
	};

	static void s_drag_end(GtkWidget  *widget,
							GdkDragContext     *context)
	{
		_wd * wd = (_wd *) gtk_object_get_data(GTK_OBJECT(widget),"wd_pointer");
		UT_DEBUGMSG(("SEVIOR: End drag of icon id %d \n",wd->m_id));
		XAP_Frame * pFrame = static_cast<XAP_Frame *>(wd->m_pUnixToolbar->getFrame());
		pFrame->dragEnd(wd->m_id);
	};

	static void s_ColorCallback(GtkWidget * /* widget */, gpointer user_data)
	{
		// this is a static callback method and does not have a 'this' pointer.
		// map the user_data into an object and dispatch the event.

//
// This is hardwired to popup the color picker dialog
//	
		_wd * wd = (_wd *) user_data;
		UT_ASSERT(wd);

		if (!wd->m_blockSignal)
		{
			XAP_Toolbar_Id id = wd->m_id;
			XAP_UnixApp * pUnixApp = wd->m_pUnixToolbar->getApp();
			const EV_EditMethodContainer * pEMC = pUnixApp->getEditMethodContainer();
			UT_ASSERT(pEMC);
			EV_EditMethod * pEM = NULL;
			UT_DEBUGMSG(("SEVIOR: toolbar ID %d forground ID number %d \n",id,EV_TBIT_ColorFore));
			AV_View * pView = wd->m_pUnixToolbar->getFrame()->getCurrentView();

			if(id ==  AP_TOOLBAR_ID_COLOR_FORE)
				pEM = pEMC->findEditMethodByName("dlgColorPickerFore");
			else
			    pEM = pEMC->findEditMethodByName("dlgColorPickerBack");
			wd->m_pUnixToolbar->invokeToolbarMethod(pView,pEM,NULL,0);
		}				
	};

	// TODO: should this move out of wd?  It's convenient here; maybe I'll make
	// a microclass for combo boxes.
	static void s_combo_changed(GtkWidget * widget, gpointer user_data)
	{
		_wd * wd = (_wd *) user_data;
		UT_ASSERT(wd);

		// only act if the widget has been shown and embedded in the toolbar
		if (wd->m_widget)
		{
			// if the popwin is still shown, this is a copy run and widget has a ->parent
			if (widget->parent)
			{
				// block is only honored here
				if (!wd->m_blockSignal)
				{
					gchar * buffer = gtk_entry_get_text(GTK_ENTRY(widget));
					UT_uint32 length = strlen(buffer);
					UT_ASSERT(length > 0);
					UT_ASSERT(length < 1024);
					strcpy(wd->m_comboEntryBuffer, buffer);
				}
			}
			else // widget has no ->parent, so use the buffer's results
			{

				// TODO : do a real conversion to UT_UCSChar or figure out the casting

				// Don't do changes for empty combo texts, or when the
				// combo box selection has been aborted (by pressing
				// outside the box - this appears to be recognizable
				// by the HAS_GRAB flag being set: I don't know if
				// that's the correct way to detect this case. 
				// jskov 2001.12.09)
				if (UT_strcmp(wd->m_comboEntryBuffer, "") 
					&& !(GTK_OBJECT_FLAGS(widget) & GTK_HAS_GRAB))
				{
					UT_UCSChar * text = (UT_UCSChar *) 
					    (wd->m_id == AP_TOOLBAR_ID_FMT_SIZE ? 
					    XAP_EncodingManager::fontsizes_mapping.lookupByTarget(wd->m_comboEntryBuffer) :
					    wd->m_comboEntryBuffer);
					
					UT_ASSERT(text);					
					wd->m_pUnixToolbar->toolbarEvent(wd, text, strlen((char*)text));
				}
			}
		}
			
	};

	// unblock when the menu goes away
	static void s_combo_hide(GtkWidget * widget, gpointer user_data)
	{
		UT_ASSERT(user_data);

		// manually force an update
		s_combo_changed(widget, user_data);
	}



	EV_UnixToolbar *	m_pUnixToolbar;
	XAP_Toolbar_Id		m_id;
	GtkWidget *			m_widget;
	bool				m_blockSignal;
	char 				m_comboEntryBuffer[1024];
};
/*****************************************************************/

EV_UnixToolbar::EV_UnixToolbar(XAP_UnixApp * pUnixApp, XAP_UnixFrame * pUnixFrame,
							   const char * szToolbarLayoutName,
							   const char * szToolbarLabelSetName)
	: EV_Toolbar(pUnixApp->getEditMethodContainer(),
				 szToolbarLayoutName,
				 szToolbarLabelSetName)
{
	m_pUnixApp = pUnixApp;
	m_pUnixFrame = pUnixFrame;
	m_pViewListener = 0;
	m_wToolbar = 0;
	m_lid = 0;							// view listener id
}

EV_UnixToolbar::~EV_UnixToolbar(void)
{
	UT_VECTOR_PURGEALL(_wd *,m_vecToolbarWidgets);
	_releaseListener();
}

bool EV_UnixToolbar::toolbarEvent(_wd * wd,
									 UT_UCSChar * pData,
									 UT_uint32 dataLength)

{
	// user selected something from this toolbar.
	// invoke the appropriate function.
	// return true iff handled.

	XAP_Toolbar_Id id = wd->m_id;

	const EV_Toolbar_ActionSet * pToolbarActionSet = m_pUnixApp->getToolbarActionSet();
	UT_ASSERT(pToolbarActionSet);

	const EV_Toolbar_Action * pAction = pToolbarActionSet->getAction(id);
	UT_ASSERT(pAction);

	AV_View * pView = m_pUnixFrame->getCurrentView();

	// make sure we ignore presses on "down" group buttons
	if (pAction->getItemType() == EV_TBIT_GroupButton)
	{
		const char * szState = 0;
		EV_Toolbar_ItemState tis = pAction->getToolbarItemState(pView,&szState);

		if (EV_TIS_ShouldBeToggled(tis))
		{
			// if this assert fires, you got a click while the button is down
			// if your widget set won't let you prevent this, handle it here

			UT_ASSERT(wd && wd->m_widget);
			
			// Block the signal, throw the button back up/down
			bool wasBlocked = wd->m_blockSignal;
			wd->m_blockSignal = true;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wd->m_widget),
										 !GTK_TOGGLE_BUTTON(wd->m_widget)->active);
			//gtk_toggle_button_toggled(item);
			wd->m_blockSignal = wasBlocked;

			// can safely ignore this event
			return true;
		}
	}

	const char * szMethodName = pAction->getMethodName();
	if (!szMethodName)
		return false;
	
	const EV_EditMethodContainer * pEMC = m_pUnixApp->getEditMethodContainer();
	UT_ASSERT(pEMC);

	EV_EditMethod * pEM = pEMC->findEditMethodByName(szMethodName);
	UT_ASSERT(pEM);						// make sure it's bound to something

	invokeToolbarMethod(pView,pEM,pData,dataLength);
	return true;
}


/*!
 * This method destroys the container widget here and returns the position in
 * the overall vbox container.
 */
UT_sint32 EV_UnixToolbar::destroy(void)
{
	GtkWidget * wVBox = m_pUnixFrame->getVBoxWidget();
	UT_sint32  pos = 0;
//
// Code gratutiously stolen from gtkbox.c
//
	GList *list = NULL;
	bool bFound = false;
	for( list = GTK_BOX(wVBox)->children; !bFound && list; list = list->next)
	{
		GtkBoxChild * child = (GtkBoxChild *) list->data;
		if(child->widget == m_wHandleBox)
		{
			bFound = true;
			break;
		}
		pos++;
	}
	UT_ASSERT(bFound);
	if(!bFound)
	{
		pos = -1;
	}
//
// Now remove the view listener
//
	AV_View * pView = getFrame()->getCurrentView();
	pView->removeListener(m_lid);
	_releaseListener();
//
// Finally destroy the old toolbar widget
//
	gtk_widget_destroy(m_wHandleBox);
	return pos;
}

/*!
 * This method rebuilds the toolbar and places it in the position it previously
 * occupied.
 */
void EV_UnixToolbar::rebuildToolbar(UT_sint32 oldpos)
{
  //
  // Build the toolbar, place it in a handlebox at an arbitary place on the 
  // the frame.
  //
    synthesize();
	GtkWidget * wVBox = m_pUnixFrame->getVBoxWidget();
	gtk_box_reorder_child(GTK_BOX(wVBox), m_wHandleBox,oldpos);
//
// bind  view listener
//
	AV_View * pView = getFrame()->getCurrentView();
	bindListenerToView(pView);
}


bool EV_UnixToolbar::synthesize(void)
{
	// create a GTK toolbar from the info provided.

	const EV_Toolbar_ActionSet * pToolbarActionSet = m_pUnixApp->getToolbarActionSet();
	UT_ASSERT(pToolbarActionSet);

	XAP_Toolbar_ControlFactory * pFactory = m_pUnixApp->getControlFactory();
	UT_ASSERT(pFactory);
	
	UT_uint32 nrLabelItemsInLayout = m_pToolbarLayout->getLayoutItemCount();
	UT_ASSERT(nrLabelItemsInLayout > 0);

	GtkWidget * wTLW = m_pUnixFrame->getTopLevelWindow();
	GtkWidget * wVBox = m_pUnixFrame->getVBoxWidget();

	m_wHandleBox = gtk_handle_box_new();
	UT_ASSERT(m_wHandleBox);

	////////////////////////////////////////////////////////////////
	// get toolbar button appearance from the preferences
	////////////////////////////////////////////////////////////////
	
	const XML_Char * szValue = NULL;
	m_pUnixApp->getPrefsValue(XAP_PREF_KEY_ToolbarAppearance,&szValue);
	UT_ASSERT((szValue) && (*szValue));
	
	GtkToolbarStyle style = GTK_TOOLBAR_ICONS;
	if (UT_XML_stricmp(szValue,"icon")==0)
		style = GTK_TOOLBAR_ICONS;
	else if (UT_XML_stricmp(szValue,"text")==0)
		style = GTK_TOOLBAR_TEXT;
	else if (UT_XML_stricmp(szValue,"both")==0)
		style = GTK_TOOLBAR_BOTH;
	
	m_wToolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, style);
	UT_ASSERT(m_wToolbar);
	
	gtk_toolbar_set_button_relief(GTK_TOOLBAR(m_wToolbar), GTK_RELIEF_NONE);
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(m_wToolbar), TRUE);
	gtk_toolbar_set_space_size(GTK_TOOLBAR(m_wToolbar), 10);
	gtk_toolbar_set_space_style(GTK_TOOLBAR (m_wToolbar), GTK_TOOLBAR_SPACE_LINE);
//
// Make the toolbar a destination for drops
//
	gtk_drag_dest_set(m_wToolbar,(GtkDestDefaults) GTK_DEST_DEFAULT_ALL,
					  s_AbiTBTargets,1,
					  GDK_ACTION_COPY);
	gtk_signal_connect(GTK_OBJECT(m_wToolbar),"drag_drop",GTK_SIGNAL_FUNC(_wd::s_drag_drop_toolbar),this);

	for (UT_uint32 k=0; (k < nrLabelItemsInLayout); k++)
	{
		EV_Toolbar_LayoutItem * pLayoutItem = m_pToolbarLayout->getLayoutItem(k);
		UT_ASSERT(pLayoutItem);

		XAP_Toolbar_Id id = pLayoutItem->getToolbarId();
		EV_Toolbar_Action * pAction = pToolbarActionSet->getAction(id);
		UT_ASSERT(pAction);
		EV_Toolbar_Label * pLabel = m_pToolbarLabelSet->getLabel(id);
		UT_ASSERT(pLabel);

		switch (pLayoutItem->getToolbarLayoutFlags())
		{
		case EV_TLF_Normal:
		{
			_wd * wd = new _wd(this,id);
			UT_ASSERT(wd);

			const char * szToolTip = pLabel->getToolTip();
			if (!szToolTip || !*szToolTip)
				szToolTip = pLabel->getStatusMsg();

			switch (pAction->getItemType())
			{
			case EV_TBIT_PushButton:
			{
				UT_ASSERT(UT_stricmp(pLabel->getIconName(),"NoIcon")!=0);
				GtkWidget * wPixmap;
				bool bFoundIcon =
					m_pUnixToolbarIcons->getPixmapForIcon(wTLW->window,
														  &wTLW->style->bg[GTK_STATE_NORMAL],
														  pLabel->getIconName(),
														  &wPixmap);
				UT_ASSERT(bFoundIcon);

				wd->m_widget = gtk_toolbar_append_item(GTK_TOOLBAR(m_wToolbar),
													   pLabel->getToolbarLabel(),
													   szToolTip,(const char *)NULL,
													   wPixmap,
													   GTK_SIGNAL_FUNC(_wd::s_callback),
													   wd);
				GtkWidget * wwd = wd->m_widget;
				gtk_object_set_data(GTK_OBJECT(wwd),
									"wd_pointer",
									wd);
				gtk_drag_source_set(wwd,GDK_BUTTON3_MASK,
									s_AbiTBTargets,1,
									GDK_ACTION_COPY);
				GdkColormap * ClrMap = gtk_widget_get_colormap (wwd);
				GdkPixmap * pixmap = GTK_PIXMAP(wPixmap)->pixmap;
			//	GdkBitmap * bitmap = GTK_PIXMAP(wPixmap)->mask;
				gtk_drag_source_set_icon(wwd,ClrMap ,pixmap,NULL);
				gtk_drag_dest_set(wwd, GTK_DEST_DEFAULT_ALL,
									s_AbiTBTargets,1,
									GDK_ACTION_COPY);
				gtk_signal_connect(GTK_OBJECT(wd->m_widget),"drag_begin",GTK_SIGNAL_FUNC(_wd::s_drag_begin), wd);
				gtk_signal_connect(GTK_OBJECT(wd->m_widget),"drag_drop",GTK_SIGNAL_FUNC(_wd::s_drag_drop), wd);
				gtk_signal_connect(GTK_OBJECT(wd->m_widget),"drag_end",GTK_SIGNAL_FUNC(_wd::s_drag_end), wd);

			}
			break;

			case EV_TBIT_ToggleButton:
			case EV_TBIT_GroupButton:
				{
					UT_ASSERT(UT_stricmp(pLabel->getIconName(),"NoIcon")!=0);
					GtkWidget * wPixmap;
					bool bFoundIcon =
						m_pUnixToolbarIcons->getPixmapForIcon(wTLW->window,
															  &wTLW->style->bg[GTK_STATE_NORMAL],
															  pLabel->getIconName(),
															  &wPixmap);
					UT_ASSERT(bFoundIcon);

					wd->m_widget = gtk_toolbar_append_element(GTK_TOOLBAR(m_wToolbar),
															  GTK_TOOLBAR_CHILD_TOGGLEBUTTON,
															  (GtkWidget *)NULL,
															  pLabel->getToolbarLabel(),
															  szToolTip,(const char *)NULL,
															  wPixmap,
															  GTK_SIGNAL_FUNC(_wd::s_callback),
															  wd);
//
// Add in a right drag method
//
				GtkWidget * wwd = wd->m_widget;
				gtk_object_set_data(GTK_OBJECT(wwd),
									"wd_pointer",
									wd);
				gtk_drag_source_set(wwd,GDK_BUTTON3_MASK,
									s_AbiTBTargets,1,
									GDK_ACTION_COPY);
				GdkColormap * ClrMap = gtk_widget_get_colormap (wwd);
				GdkPixmap * pixmap = GTK_PIXMAP(wPixmap)->pixmap;
//				GdkBitmap * bitmap = GTK_PIXMAP(wPixmap)->mask;
				gtk_drag_source_set_icon(wwd,ClrMap ,pixmap,NULL);
				gtk_drag_dest_set(wwd,(GtkDestDefaults) GTK_DEST_DEFAULT_ALL,
									s_AbiTBTargets,1,
									GDK_ACTION_COPY);
				gtk_signal_connect(GTK_OBJECT(wd->m_widget),"drag_begin",GTK_SIGNAL_FUNC(_wd::s_drag_begin), wd);
				gtk_signal_connect(GTK_OBJECT(wd->m_widget),"drag_drop",GTK_SIGNAL_FUNC(_wd::s_drag_drop), wd);
				gtk_signal_connect(GTK_OBJECT(wd->m_widget),"drag_end",GTK_SIGNAL_FUNC(_wd::s_drag_end), wd);
				}
				break;

			case EV_TBIT_EditText:
				break;
					
			case EV_TBIT_DropDown:
				break;
					
			case EV_TBIT_ComboBox:
			{
				EV_Toolbar_Control * pControl = pFactory->getControl(this, id);
				UT_ASSERT(pControl);

				// default, shouldn't be used for well-defined controls
				int iWidth = 100;

				if (pControl)
				{
					iWidth = pControl->getPixelWidth();
				}

				GtkWidget * comboBox = gtk_combo_new();
				UT_ASSERT(comboBox);

				// Combo boxes flash on 8-bit displays unless you set its colormap
				// to agree with what we're using elsewhere (gdk_rgb's version)
				gtk_widget_set_colormap(comboBox, gdk_rgb_get_cmap());
				
				// set the size of the entry to set the total combo size
				gtk_widget_set_usize(GTK_COMBO(comboBox)->entry, iWidth, 0);

				// the entry is read-only for now
				gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(comboBox)->entry), FALSE);
										 
				// handle popup events, so we can block our signals until the popdown
				GtkWidget * popwin = GTK_WIDGET(GTK_COMBO(comboBox)->popwin);
				UT_ASSERT(popwin);
// we don't use this
#if 0
				gtk_signal_connect(GTK_OBJECT(popwin),
								   "show",
								   GTK_SIGNAL_FUNC(_wd::s_combo_show),
								   wd);
#endif
				gtk_signal_connect(GTK_OBJECT(popwin),
								   "hide",
								   GTK_SIGNAL_FUNC(_wd::s_combo_hide),
								   wd);

				// take away the ability to gain focus
//				gtk_signal_connect(GTK_OBJECT(GTK_COMBO(comboBox)->entry),
//								   "focus_in_event",
//								   GTK_SIGNAL_FUNC(_wd::s_combo_focus_in),
//								   wd);
//				gtk_signal_connect(GTK_OBJECT(comboBox),
//								   "key_press_event",
//								   GTK_SIGNAL_FUNC(_wd::s_combo_key_press),
//								   wd);
//				gtk_signal_connect(GTK_OBJECT(GTK_COMBO(comboBox)->entry),
//								   "key_press_event",
//								   GTK_SIGNAL_FUNC(_wd::s_combo_key_press),
//								   wd);
				
				// handle changes in content
				GtkEntry * blah = GTK_ENTRY(GTK_COMBO(comboBox)->entry);
				GtkEditable * yuck = GTK_EDITABLE(blah);
				gtk_signal_connect(GTK_OBJECT(&yuck->widget),
								   "changed",
								   GTK_SIGNAL_FUNC(_wd::s_combo_changed),
								   wd);
				
				// populate it
				if (pControl)
				{
					pControl->populate();

					const UT_Vector * v = pControl->getContents();
					UT_ASSERT(v);

					if (v)
					{
						UT_uint32 items = v->getItemCount();
						for (UT_uint32 m=0; m < items; m++)
						{
							char * sz = (char *)v->getNthItem(m);
							GtkWidget * li = gtk_list_item_new_with_label(sz);
							gtk_widget_show(li);
							gtk_container_add (GTK_CONTAINER(GTK_COMBO(comboBox)->list), li);
						}
					}
				}
 
				// give a final show
				gtk_widget_show(comboBox);

				// stick it in the toolbar
				//	gtk_toolbar_append_widget(GTK_TOOLBAR(m_wToolbar),
				//						  comboBox,
				//						  szToolTip,
				//						  (const char *) NULL);
				GtkWidget * evBox = toolbar_append_with_eventbox(GTK_TOOLBAR(m_wToolbar),
									     comboBox,
									     szToolTip,
									     (const char *)NULL);
				wd->m_widget = comboBox;
//
// Add in a right drag method
//
				GtkWidget * wwd = wd->m_widget;
				gtk_object_set_data(GTK_OBJECT(wwd),
									"wd_pointer",
									wd);
				gtk_drag_source_set(evBox,GDK_BUTTON3_MASK,
									s_AbiTBTargets,1,
									GDK_ACTION_COPY);
//				GdkColormap * ClrMap = gtk_widget_get_colormap (wwd);
//				GdkPixmap * pixmap = GTK_PIXMAP(wPixmap)->pixmap;
//				GdkBitmap * bitmap = GTK_PIXMAP(wPixmap)->mask;
//				gtk_drag_source_set_icon(wwd,ClrMap ,pixmap,bitmap);
				gtk_drag_dest_set(evBox,(GtkDestDefaults) GTK_DEST_DEFAULT_ALL,
									s_AbiTBTargets,1,
									GDK_ACTION_COPY);
				gtk_signal_connect(GTK_OBJECT(evBox),"drag_begin",GTK_SIGNAL_FUNC(_wd::s_drag_begin), wd);
				gtk_signal_connect(GTK_OBJECT(evBox),"drag_drop",GTK_SIGNAL_FUNC(_wd::s_drag_drop), wd);
				gtk_signal_connect(GTK_OBJECT(evBox),"drag_end",GTK_SIGNAL_FUNC(_wd::s_drag_end), wd);

				// for now, we never repopulate, so can just toss it
				DELETEP(pControl);
			}
			break;

			case EV_TBIT_ColorFore:
			case EV_TBIT_ColorBack:
			{
				UT_ASSERT(UT_stricmp(pLabel->getIconName(),"NoIcon")!=0);
				GtkWidget * wPixmap;
				bool bFoundIcon =
					m_pUnixToolbarIcons->getPixmapForIcon(wTLW->window,
														  &wTLW->style->bg[GTK_STATE_NORMAL],
														  pLabel->getIconName(),
														  &wPixmap);
				UT_ASSERT(bFoundIcon);

				wd->m_widget = gtk_toolbar_append_item(GTK_TOOLBAR(m_wToolbar),
													   pLabel->getToolbarLabel(),
													   szToolTip,(const char *)NULL,
													   wPixmap,
													   GTK_SIGNAL_FUNC(_wd::s_ColorCallback),
													   wd);
//
// Add in a right drag method
//
				GtkWidget * wwd = wd->m_widget;
				gtk_object_set_data(GTK_OBJECT(wwd),
									"wd_pointer",
									wd);
				gtk_drag_source_set(wwd,GDK_BUTTON3_MASK,
									s_AbiTBTargets,1,
									GDK_ACTION_COPY);
				GdkColormap * ClrMap = gtk_widget_get_colormap (wwd);
				GdkPixmap * pixmap = GTK_PIXMAP(wPixmap)->pixmap;
				GdkBitmap * bitmap = GTK_PIXMAP(wPixmap)->mask;
				gtk_drag_dest_set(wwd,(GtkDestDefaults) GTK_DEST_DEFAULT_ALL,
									s_AbiTBTargets,1,
									GDK_ACTION_COPY);
				gtk_drag_source_set_icon(wwd,ClrMap ,pixmap,bitmap);
				gtk_signal_connect(GTK_OBJECT(wd->m_widget),"drag_begin",GTK_SIGNAL_FUNC(_wd::s_drag_begin), wd);
				gtk_signal_connect(GTK_OBJECT(wd->m_widget),"drag_drop",GTK_SIGNAL_FUNC(_wd::s_drag_drop), wd);
				gtk_signal_connect(GTK_OBJECT(wd->m_widget),"drag_end",GTK_SIGNAL_FUNC(_wd::s_drag_end), wd);

			}
			break;
				
			case EV_TBIT_StaticLabel:
				// TODO do these...
				break;
					
			case EV_TBIT_Spacer:
				break;
					
			case EV_TBIT_BOGUS:
			default:
//				UT_ASSERT(0);
				break;
			}
		// add item after bindings to catch widget returned to us
			m_vecToolbarWidgets.addItem(wd);
		}
		break;
			
		case EV_TLF_Spacer:
		{
			// Append to the vector even if spacer, to sync up with refresh
			// which expects each item in the layout to have a place in the
			// vector.
			_wd * wd = new _wd(this,id);
			UT_ASSERT(wd);
			m_vecToolbarWidgets.addItem(wd);

			gtk_toolbar_append_space(GTK_TOOLBAR(m_wToolbar));
			break;
		}
		
		default:
			UT_ASSERT(0);
		}
	}

	// show the complete thing
	gtk_widget_show(m_wToolbar);
//	gtk_widget_add_events(m_wToolbar,GDK_ALL_EVENTS_MASK);

	// an arbitrary padding to make our document not run into our buttons
	gtk_container_set_border_width(GTK_CONTAINER(m_wToolbar), 2);

	// pack it in a handle box
	gtk_container_add(GTK_CONTAINER(m_wHandleBox), m_wToolbar);
	gtk_widget_show(m_wHandleBox);
	
	// put it in the vbox
	gtk_box_pack_start(GTK_BOX(wVBox), m_wHandleBox, FALSE, FALSE, 0);

	return true;
}

void EV_UnixToolbar::_releaseListener(void)
{
	if (!m_pViewListener)
		return;
	DELETEP(m_pViewListener);
	m_pViewListener = 0;
	m_lid = 0;
}
	
bool EV_UnixToolbar::bindListenerToView(AV_View * pView)
{
	_releaseListener();
	
	m_pViewListener = new EV_UnixToolbar_ViewListener(this,pView);
	UT_ASSERT(m_pViewListener);

	bool bResult = pView->addListener(static_cast<AV_Listener *>(m_pViewListener),&m_lid);
	UT_ASSERT(bResult);

	if(pView->isDocumentPresent())
	{
		refreshToolbar(pView, AV_CHG_ALL);
	}
	return true;
}

bool EV_UnixToolbar::refreshToolbar(AV_View * pView, AV_ChangeMask mask)
{
	// make the toolbar reflect the current state of the document
	// at the current insertion point or selection.
	
	const EV_Toolbar_ActionSet * pToolbarActionSet = m_pUnixApp->getToolbarActionSet();
	UT_ASSERT(pToolbarActionSet);
	
	UT_uint32 nrLabelItemsInLayout = m_pToolbarLayout->getLayoutItemCount();
	for (UT_uint32 k=0; (k < nrLabelItemsInLayout); k++)
	{
		EV_Toolbar_LayoutItem * pLayoutItem = m_pToolbarLayout->getLayoutItem(k);
		UT_ASSERT(pLayoutItem);

		XAP_Toolbar_Id id = pLayoutItem->getToolbarId();
		EV_Toolbar_Action * pAction = pToolbarActionSet->getAction(id);
		UT_ASSERT(pAction);

		AV_ChangeMask maskOfInterest = pAction->getChangeMaskOfInterest();
		if ((maskOfInterest & mask) == 0)					// if this item doesn't care about
			continue;										// changes of this type, skip it...

		switch (pLayoutItem->getToolbarLayoutFlags())
		{
		case EV_TLF_Normal:
			{
				const char * szState = 0;
				EV_Toolbar_ItemState tis = pAction->getToolbarItemState(pView,&szState);

				switch (pAction->getItemType())
				{
				case EV_TBIT_PushButton:
				{
					bool bGrayed = EV_TIS_ShouldBeGray(tis);

					_wd * wd = (_wd *) m_vecToolbarWidgets.getNthItem(k);
					UT_ASSERT(wd);
					GtkButton * item = GTK_BUTTON(wd->m_widget);
					UT_ASSERT(item);
						
					// Disable/enable toolbar item
					gtk_widget_set_sensitive(GTK_WIDGET(item), !bGrayed);
     					
					//UT_DEBUGMSG(("refreshToolbar: PushButton [%s] is %s\n",
					//			 m_pToolbarLabelSet->getLabel(id)->getToolbarLabel(),
					//			 ((bGrayed) ? "disabled" : "enabled")));
				}
				break;
			
				case EV_TBIT_ToggleButton:
				case EV_TBIT_GroupButton:
				{
					bool bGrayed = EV_TIS_ShouldBeGray(tis);
					bool bToggled = EV_TIS_ShouldBeToggled(tis);

					_wd * wd = (_wd *) m_vecToolbarWidgets.getNthItem(k);
					UT_ASSERT(wd);
					GtkToggleButton * item = GTK_TOGGLE_BUTTON(wd->m_widget);
					UT_ASSERT(item);
						
					// Block the signal, throw the toggle event
					bool wasBlocked = wd->m_blockSignal;
					wd->m_blockSignal = true;
					gtk_toggle_button_set_active(item, bToggled);
					//gtk_toggle_button_toggled(item);
					wd->m_blockSignal = wasBlocked;
						
					// Disable/enable toolbar item
					gtk_widget_set_sensitive(GTK_WIDGET(item), !bGrayed);
						
					//UT_DEBUGMSG(("refreshToolbar: ToggleButton [%s] is %s and %s\n",
					//			 m_pToolbarLabelSet->getLabel(id)->getToolbarLabel(),
					//			 ((bGrayed) ? "disabled" : "enabled"),
					//			 ((bToggled) ? "pressed" : "not pressed")));
				}
				break;

				case EV_TBIT_EditText:
					break;
				case EV_TBIT_DropDown:
					break;
				case EV_TBIT_ComboBox:
				{
					bool bGrayed = EV_TIS_ShouldBeGray(tis);
					//bool bString = EV_TIS_ShouldUseString(tis);
					
					_wd * wd = (_wd *) m_vecToolbarWidgets.getNthItem(k);
					UT_ASSERT(wd);
					GtkCombo * item = GTK_COMBO(wd->m_widget);
					UT_ASSERT(item);
					// Disable/enable toolbar item
					gtk_widget_set_sensitive(GTK_WIDGET(item), !bGrayed);

					// Block the signal, set the contents
					bool wasBlocked = wd->m_blockSignal;
					wd->m_blockSignal = true;
					if (szState) {
					  if ( wd->m_id==AP_TOOLBAR_ID_FMT_SIZE )
					    {
					      const char * fsz = XAP_EncodingManager::fontsizes_mapping.lookupBySource(szState);
					      if ( fsz )
						gtk_entry_set_text(GTK_ENTRY(item->entry), fsz);
					      else
						gtk_entry_set_text(GTK_ENTRY(item->entry), "");
					    }
					  else
					    gtk_entry_set_text(GTK_ENTRY(item->entry), szState);
					} 
					else {
						gtk_entry_set_text(GTK_ENTRY(item->entry), "");
					}					

					wd->m_blockSignal = wasBlocked;
					
					//UT_DEBUGMSG(("refreshToolbar: ComboBox [%s] is %s and %s\n",
					//			 m_pToolbarLabelSet->getLabel(id)->getToolbarLabel(),
					//			 ((bGrayed) ? "disabled" : "enabled"),
					//			 ((bString) ? szState : "no state")));
				}
				break;

                case EV_TBIT_ColorFore:
                case EV_TBIT_ColorBack:
                {
					bool bGrayed = EV_TIS_ShouldBeGray(tis);
					
					_wd * wd = (_wd *) m_vecToolbarWidgets.getNthItem(k);
					UT_ASSERT(wd);
					UT_ASSERT(wd->m_widget);
					gtk_widget_set_sensitive(GTK_WIDGET(wd->m_widget), !bGrayed);   // Disable/enable toolbar item
                }
				break;

				case EV_TBIT_StaticLabel:
					break;
				case EV_TBIT_Spacer:
					break;
				case EV_TBIT_BOGUS:
					break;
				default:
					UT_ASSERT(0);
					break;
				}
			}
			break;
			
		case EV_TLF_Spacer:
			break;
			
		default:
			UT_ASSERT(0);
			break;
		}
	}

	return true;
}

XAP_UnixApp * EV_UnixToolbar::getApp(void)
{
	return m_pUnixApp;
}

XAP_UnixFrame * EV_UnixToolbar::getFrame(void)
{
	return m_pUnixFrame;
}

void EV_UnixToolbar::show(void)
{
	if (m_wToolbar)
		gtk_widget_show (m_wToolbar->parent);
}

void EV_UnixToolbar::hide(void)
{
	if (m_wToolbar)
		gtk_widget_hide (m_wToolbar->parent);
}

/*!
 * This method examines the current document and repopulates the Styles
 * Combo box with what is in the document. It returns false if no styles 
 * combo box was found. True if it all worked.
 */
bool EV_UnixToolbar::repopulateStyles(void)
{
//
// First off find the Styles combobox in a toolbar somewhere
//
	UT_uint32 count = m_pToolbarLayout->getLayoutItemCount();
	UT_uint32 i =0;
	EV_Toolbar_LayoutItem * pLayoutItem = NULL;
	XAP_Toolbar_Id id = 0;
	_wd * wd = NULL;
	for(i=0; i < count; i++)
	{
		pLayoutItem = m_pToolbarLayout->getLayoutItem(i);
		id = pLayoutItem->getToolbarId();
		wd = (_wd *) m_vecToolbarWidgets.getNthItem(i);
		if(id == AP_TOOLBAR_ID_FMT_STYLE)
			break;
	}
	if(i>=count)
		return false;
//
// GOT IT!
//
	UT_ASSERT(wd->m_id == AP_TOOLBAR_ID_FMT_STYLE);
	XAP_Toolbar_ControlFactory * pFactory = m_pUnixApp->getControlFactory();
	UT_ASSERT(pFactory);
	EV_Toolbar_Control * pControl = pFactory->getControl(this, id);
	AP_UnixToolbar_StyleCombo * pStyleC = static_cast<AP_UnixToolbar_StyleCombo *>(pControl);
	pStyleC->repopulate();
	GtkCombo * item = GTK_COMBO(wd->m_widget);
//
// Now the combo box has to be refilled from this
//						
	const UT_Vector * v = pControl->getContents();
	UT_ASSERT(v);
//
// Now  we must remove and delete the old glist so we can attach the new
// list of styles to the combo box.
//
// Try this....
//
    GtkList * oldlist = GTK_LIST(item->list);
	gtk_list_clear_items(oldlist,0,-1);
//
// Now make a new one.
//
	UT_uint32 items = v->getItemCount();
	for (UT_uint32 m=0; m < items; m++)
	{
		char * sz = (char *)v->getNthItem(m);
		GtkWidget * li = gtk_list_item_new_with_label(sz);
		gtk_widget_show(li);
		gtk_container_add (GTK_CONTAINER(GTK_COMBO(item)->list), li);
	}
//
// Don't need this anymore and we don't like memory leaks in abi
//
	delete pStyleC;
//
// I think we've finished!
//
	return true;
}




