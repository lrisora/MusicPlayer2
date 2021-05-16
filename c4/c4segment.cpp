/************************************************************************/
/*                                                                      */
/* c4-lib                                                               */
/* c4-lib is A Common Codes Converting Context library.                 */
/*                                                                      */
/* Version: 0.1                                                         */
/* Author:  wei_w (weiwl07@gmail.com)                                   */
/* Published under LGPL-3.0                                             */
/* https://www.gnu.org/licenses/lgpl-3.0.en.html                        */
/*                                                                      */
/* Copyright 2016 Project c4                                            */
/* https://github.com/kuyur/c4                                          */
/*                                                                      */
/************************************************************************/

#include "c4segment.h"

CC4Segments::CC4Segments(unsigned int num_segments)
	:m_size(num_segments)
{
	if (m_size>0)
	{
		m_segments = new CC4Segment[m_size];
		for (unsigned int i=0; i<m_size; ++i)
			m_segments[i] = CC4Segment();
	}
	else
		m_segments = NULL;
}

CC4Segments::~CC4Segments()
{
	if (NULL != m_segments)
		delete []m_segments;
}

bool CC4Segments::checkValid() const
{
	if (NULL == m_segments)
		return false;
	// TODO
	return true;
}

unsigned int CC4Segments::calBufferLength() const
{
	// TODO
	return 0;
}

bool CC4Segments::setSegment(unsigned int position, const CC4Segment& segment)
{
	if (position>=m_size)
		return false;

	m_segments[position] = segment;
	//memcpy((void*)(m_segments+position), (void*)&segment, sizeof(CC4Segment));

	return true;
}

const CC4Segment* CC4Segments::findMatchedSegment(wchar_t chr) const
{
	if (!checkValid())
		return NULL;
	for (unsigned int i=0; i<m_size; ++i)
	{
		if (chr>=m_segments[i].m_begin && chr<=m_segments[i].m_end)
			return m_segments+i;
	}
	return NULL;
}