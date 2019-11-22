/* Copyright (C) 2006 Marc Maurer <uwog@uwog.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#ifndef AP_DIALOG_COLLABORATIONJOIN_H
#define AP_DIALOG_COLLABORATIONJOIN_H

#include "ut_types.h"
#include "xap_Frame.h"
#include "xap_Dialog.h"
#include "xav_View.h"
#include "ut_vector.h"
#include <account/xp/AccountHandler.h>
#include <account/xp/Buddy.h>
#include <account/xp/EventListener.h>

extern pt2Constructor ap_Dialog_CollaborationJoin_Constructor;

class AP_Dialog_CollaborationJoin : public XAP_Dialog_NonPersistent, EventListener
{
public:
	AP_Dialog_CollaborationJoin(XAP_DialogFactory * pDlgFactory, XAP_Dialog_Id id);
	virtual ~AP_Dialog_CollaborationJoin(void);

	virtual void runModal(XAP_Frame * pFrame) override = 0;

	virtual void signal(const Event& event, BuddyPtr pSource) override;

	typedef enum { a_CANCEL, a_OPEN } tAnswer;

	AP_Dialog_CollaborationJoin::tAnswer	getAnswer(void) const
		{ return m_answer; }

	BuddyPtr				getBuddy()
		{ return m_pBuddy; };

	DocHandle*				getDocHandle()
		{ return m_pDocHandle; };

protected:
	AP_Dialog_CollaborationJoin::tAnswer m_answer;
	BuddyPtr			m_pBuddy;
	DocHandle*			m_pDocHandle;

	void				_eventAddBuddy();
	void				_refreshAllDocHandlesAsync();

	void				_refreshAccounts();

	virtual void		_refreshWindow() = 0;
	virtual void		_enableBuddyAddition(bool bEnabled) = 0;
	virtual void		_addDocument(BuddyPtr pBuddy, DocHandle* pDocHandle) = 0;
};

#endif /* AP_DIALOG_COLLABORATIONJOIN_H */
