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

#ifndef UTCONTEXTGLYPH_H
#define UTCONTEXTGLYPH_H

#include "ut_misc.h"

#define CONTEXT_BUFF_SIZE 5

struct Letter
{
	UT_UCSChar code_low;
	UT_UCSChar code_high;
	UT_UCSChar initial;
	UT_UCSChar medial;
	UT_UCSChar final;
	UT_UCSChar alone;
};

struct UCSRange
{
	UT_UCSChar low;
	UT_UCSChar high;
};

enum GlyphContext {GC_ISOLATE,GC_INITIAL,GC_MEDIAL,GC_FINAL,GC_NOT_SET};

class UT_contextGlyph
{
	public:
		UT_contextGlyph();
		UT_UCSChar getGlyph(const UT_UCSChar * code,
							const UT_UCSChar * prev,
							const UT_UCSChar * next) const;
		
		void renderString(const UT_UCSChar * src,
							UT_UCSChar *dest,
							UT_uint32 len,
							const UT_UCSChar * prev,
							const UT_UCSChar * next) const;
		
	private:
		GlyphContext _evalGlyphContext( const UT_UCSChar * code,
										const UT_UCSChar * prev,
										const UT_UCSChar * next) const;
										
		static Letter 		*s_pGlyphTable;
		static UCSRange 	*s_pIgnore;
		static Letter		*s_pLigature;
		static Letter		*s_pLigRev;
		static bool			s_bInit;
};
#endif
