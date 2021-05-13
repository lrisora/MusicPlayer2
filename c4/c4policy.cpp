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

#include "c4policy.h"

CC4Policies::CC4Policies(unsigned int num_policies)
	:m_size(num_policies)
{
	if (m_size > 0)
	{
		m_policies = new CC4Policy[m_size];
		for (unsigned int i=0; i<m_size; ++i)
			m_policies[i] = CC4Policy();
	}
	else
		m_policies = NULL;
}

CC4Policies::~CC4Policies()
{
	if (NULL != m_policies)
		delete []m_policies;
}

bool CC4Policies::checkValid() const
{
	if (NULL == m_policies)
		return false;
	// TODO
	return true;
}

bool CC4Policies::setPolicy(unsigned int position, const CC4Policy& policy)
{
	if (position >= m_size)
		return false;

	m_policies[position] = policy;
	//memcpy((void*)(m_policies+position), &policy, sizeof(CC4Policy));

	return true;
}

bool CC4Policies::isContinueReadNextChar(unsigned char chr) const
{
	for (unsigned int i=0; i<m_size; ++i)
	{
		if (chr>=m_policies[i].m_begin && chr<=m_policies[i].m_end)
			return m_policies[i].m_readnext;
	}

	return false;
}