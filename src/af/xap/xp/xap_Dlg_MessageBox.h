/* AbiSource Application Framework
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

#ifndef AP_DIALOG_MESSAGEBOX_H
#define AP_DIALOG_MESSAGEBOX_H

#include "xap_Dialog.h"

class AP_Dialog_MessageBox : public AP_Dialog_NonPersistent
{
public:
	AP_Dialog_MessageBox(AP_DialogFactory * pDlgFactory, AP_Dialog_Id id);
	virtual ~AP_Dialog_MessageBox(void);

	virtual void					runModal(XAP_Frame * pFrame) = 0;

	typedef enum { b_O, b_OC, b_YN, b_YNC }			tButtons;
	typedef enum { a_OK, a_CANCEL, a_YES, a_NO }	tAnswer;
	
	void							setMessage(const char * szMessage);
	void							setMessage(const char * szMessage, const char * sz1);
	void							setButtons(AP_Dialog_MessageBox::tButtons buttons);
	void							setDefaultAnswer(AP_Dialog_MessageBox::tAnswer answer);
	AP_Dialog_MessageBox::tAnswer	getAnswer(void) const;

protected:
	char *							m_szMessage;
	AP_Dialog_MessageBox::tButtons	m_buttons;
	AP_Dialog_MessageBox::tAnswer	m_defaultAnswer;
	AP_Dialog_MessageBox::tAnswer	m_answer;
};

#endif /* AP_DIALOG_MESSAGEBOX_H */
