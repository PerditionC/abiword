/* -*- mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: t -*- */
/* AbiSource
 *
 * Copyright (C) 2009 Firat Kiyak <firatkiyak@gmail.com>
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

#ifndef _OXMLI_LISTENERSTATE_TABLE_H_
#define _OXMLI_LISTENERSTATE_TABLE_H_

#include "OXMLi_ListenerState.h"
#include "OXMLi_Types.h"
#include "OXML_Types.h"
#include "OXML_Element_Table.h"

/* \class OXMLi_ListenerState_Table
 * \brief This ListenerState parses the Tables.
*/
class OXMLi_ListenerState_Table : public OXMLi_ListenerState
{
public:
	OXMLi_ListenerState_Table();
	virtual void startElement (OXMLi_StartElementRequest * rqst) override;
	virtual void endElement (OXMLi_EndElementRequest * rqst) override;
	virtual void charData (OXMLi_CharDataRequest * rqst) override;

private:
	std::stack<OXML_SharedElement_Table> m_tableStack;
	std::stack<OXML_Element_Row*> m_rowStack;
	std::stack<OXML_SharedElement_Cell> m_cellStack;
};

#endif //_OXMLI_LISTENERSTATE_TABLE_H_

