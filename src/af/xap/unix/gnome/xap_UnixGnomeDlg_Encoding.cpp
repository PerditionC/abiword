/* AbiWord
 * Copyright (C) 2000 AbiSource, Inc.
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

#include "ut_string.h"
#include "ut_assert.h"
#include "ut_debugmsg.h"

// This header defines some functions for Unix dialogs,
// like centering them, measuring them, etc.
#include "ut_dialogHelper.h"

#include "xap_App.h"
#include "xap_UnixGnomeApp.h"
#include "xap_UnixGnomeFrame.h"

#include "xap_Strings.h"
#include "xap_Dialog_Id.h"
#include "xap_UnixGnomeDlg_Encoding.h"

/*****************************************************************/

XAP_Dialog * XAP_UnixGnomeDialog_Encoding::static_constructor(XAP_DialogFactory * pFactory,
													 XAP_Dialog_Id id)
{
	XAP_UnixGnomeDialog_Encoding * p = new XAP_UnixGnomeDialog_Encoding(pFactory,id);
	return p;
}

XAP_UnixGnomeDialog_Encoding::XAP_UnixGnomeDialog_Encoding(XAP_DialogFactory * pDlgFactory,
										 XAP_Dialog_Id id)
	: XAP_UnixDialog_Encoding(pDlgFactory,id)
{
}

XAP_UnixGnomeDialog_Encoding::~XAP_UnixGnomeDialog_Encoding(void)
{
}

void XAP_UnixGnomeDialog_Encoding::runModal(XAP_Frame * pFrame)
{

	UT_ASSERT(pFrame);
	XAP_UnixDialog_Encoding::runModal( pFrame);
}
