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

#include <stdlib.h>
#include <time.h>
#include <glade/glade.h>

#include "ut_string.h"
#include "ut_assert.h"
#include "ut_debugmsg.h"

// This header defines some functions for Unix dialogs,
// like centering them, measuring them, etc.
#include "xap_UnixDialogHelper.h"

#include "xap_App.h"
#include "xap_UnixApp.h"
#include "xap_Frame.h"

#include "ap_Strings.h"
#include "ap_Dialog_Id.h"
#include "ap_Dialog_Insert_DateTime.h"
#include "ap_UnixDialog_Insert_DateTime.h"

/*****************************************************************/

#define	LIST_ITEM_INDEX_KEY "index"

/*****************************************************************/

XAP_Dialog * AP_UnixDialog_Insert_DateTime::static_constructor(XAP_DialogFactory * pFactory,
															   XAP_Dialog_Id id)
{
	AP_UnixDialog_Insert_DateTime * p = new AP_UnixDialog_Insert_DateTime(pFactory,id);
	return p;
}

AP_UnixDialog_Insert_DateTime::AP_UnixDialog_Insert_DateTime(XAP_DialogFactory * pDlgFactory,
															 XAP_Dialog_Id id)
	: AP_Dialog_Insert_DateTime(pDlgFactory,id)
{
	m_windowMain = NULL;
	m_tvFormats = NULL;
}

AP_UnixDialog_Insert_DateTime::~AP_UnixDialog_Insert_DateTime(void)
{
}

/*****************************************************************/
/*****************************************************************/

void AP_UnixDialog_Insert_DateTime::runModal(XAP_Frame * pFrame)
{
	UT_return_if_fail(pFrame);
	
	// Build the window's widgets and arrange them
	m_windowMain = _constructWindow();
	UT_return_if_fail(m_windowMain);

	// Populate the window's data items
	_populateWindowData();

	switch(abiRunModalDialog(GTK_DIALOG(m_windowMain), pFrame, this,
							 GTK_RESPONSE_CANCEL, false ))
	{
		case GTK_RESPONSE_OK:
			event_OK();
			break;
		default:
			m_answer = AP_Dialog_Insert_DateTime::a_CANCEL;
			break;
	}

	abiDestroyWidget ( m_windowMain ) ;
}

void AP_UnixDialog_Insert_DateTime::event_OK(void)
{
	UT_ASSERT(m_windowMain && m_tvFormats);

	GtkTreeSelection * selection;
	GtkTreeIter iter;
	GtkTreeModel * model;

	selection = gtk_tree_view_get_selection( GTK_TREE_VIEW(m_tvFormats) );

	// if there is no selection, or the selection's data (GtkListItem widget)
	// is empty, return cancel.  GTK can make this happen.
	if ( !selection || 
		 !gtk_tree_selection_get_selected (selection, &model, &iter)
	   )
	{
		m_answer = AP_Dialog_Insert_DateTime::a_CANCEL;
		return;
	}

	// get the ID of the selected DataTime format	
	gtk_tree_model_get (model, &iter, 1, &m_iFormatIndex, -1);
	m_answer = AP_Dialog_Insert_DateTime::a_OK;
}

/*****************************************************************/
GtkWidget * AP_UnixDialog_Insert_DateTime::_constructWindow(void)
{
	GtkWidget * window;	
	const XAP_StringSet * pSS = m_pApp->getStringSet();
	
	// get the path where our glade file is located
	XAP_UnixApp * pApp = static_cast<XAP_UnixApp*>(m_pApp);
	UT_String glade_path( pApp->getAbiSuiteAppGladeDir() );
	glade_path += "/ap_UnixDialog_Insert_DateTime.glade";
	
	// load the dialog from the glade file
	GladeXML *xml = abiDialogNewFromXML( glade_path.c_str() );

	// Update our member variables with the important widgets that 
	// might need to be queried or altered later
	window = glade_xml_get_widget(xml, "ap_UnixDialog_Insert_DateTime");
	m_tvFormats = glade_xml_get_widget(xml, "tvFormats");

	// set the single selection mode for the TreeView
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (m_tvFormats)), GTK_SELECTION_SINGLE);		
	
	// set the dialog title
	abiDialogSetTitle(window, pSS->getValueUTF8(AP_STRING_ID_DLG_DateTime_DateTimeTitle).c_str());
	
	// localize the strings in our dialog
	
	localizeLabelMarkup(glade_xml_get_widget(xml, "lbAvailableFormats"), pSS, AP_STRING_ID_DLG_DateTime_AvailableFormats);
	
	return window;
}

void AP_UnixDialog_Insert_DateTime::_populateWindowData(void)
{
	UT_ASSERT(m_windowMain && m_tvFormats);

	// NOTE : this code is similar to the Windows dialog code to do
	// NOTE : the same thing.  if you are implementing this dialog
	// NOTE : for a new front end, this is the formatting logic 
	// NOTE : you'll want to use to populate your list

	UT_sint32 i;
	
	// this constant comes from ap_Dialog_Insert_DateTime.h
    char szCurrentDateTime[CURRENT_DATE_TIME_SIZE];
	
    time_t tim = time(NULL);
	
    struct tm *pTime = localtime(&tim);
	
	GtkListStore *model;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;	
	
	model = gtk_list_store_new (2, 
							    G_TYPE_STRING,
								G_TYPE_INT
	                            );
	
 	// build a list of all items
    for (i = 0; InsertDateTimeFmts[i] != NULL; i++)
	{
        strftime(szCurrentDateTime, CURRENT_DATE_TIME_SIZE, InsertDateTimeFmts[i], pTime);

		// Add a new row to the model
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
					  		0, szCurrentDateTime,
							1, i,
					  		-1);
	}
	
	gtk_tree_view_set_model( GTK_TREE_VIEW(m_tvFormats), (GtkTreeModel *)model);

	// create a column, and add it to the TreeView
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Format",
							 renderer,
							 "text", 
							 0,
							 NULL);
	//gtk_tree_view_column_set_sort_column_id (column, COLUMN_DESCRIPTION);
	gtk_tree_view_append_column( GTK_TREE_VIEW(m_tvFormats), column);	
	
	g_object_unref (model);
	
	// now select first item in box
 	gtk_widget_grab_focus (m_tvFormats);
}

