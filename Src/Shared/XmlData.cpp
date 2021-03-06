///////////////////////////////////////////////////////////////////////////////
// $Workfile: XmlData.cpp $
// $Archive: /WorkspaceWhiz/Src/Shared/XmlData.cpp $
// $Date: 2003/01/05 $ $Revision: #6 $ $Author: Joshua $
///////////////////////////////////////////////////////////////////////////////
// This source file is part of the Workspace Whiz source distribution and
// is Copyright 1997-2003 by Joshua C. Jensen.  (http://workspacewhiz.com/)
//
// The code presented in this file may be freely used and modified for all
// non-commercial and commercial purposes so long as due credit is given and
// this header is left intact.
///////////////////////////////////////////////////////////////////////////////
#include "XmlData.h"

XmlNode* XmlNode::Find(const CString& name)
{
	if (name == m_name)
		return this;

	XmlNode* childNode = (XmlNode*)GetFirstChildNode();
	while (childNode)
	{
		XmlNode* foundNode = childNode->Find(name);
		if (foundNode)
			return foundNode;
		childNode = (XmlNode*)childNode->GetNextSiblingNode();
	}

	return NULL;
}


XmlNode::Attribute* XmlNode::FindAttribute(const CString& name)
{
	POSITION pos = m_attributes.GetHeadPosition();
	while (pos)
	{
		Attribute& attr = m_attributes.GetNext(pos);
		if (attr.m_name == name)
			return &attr;
	}

	return NULL;
}


void XmlNode::SetAttribute(const CString& name, const CString& value)
{
	Attribute* attr = FindAttribute(name);
	if (!attr)
	{
		Attribute newAttr;
		newAttr.m_name = name;
		newAttr.m_value = value;

		m_attributes.AddTail(newAttr);
	}
	else
	{
		attr->m_value = value;
	}
}

	
XmlData::~XmlData()
{
	delete m_xmlRoot;
}


static CString ReadTag(LPCSTR& ptr)
{
	while (true)
	{
		if (*ptr == 0)
		{
			return "";
		}
		else if (*ptr == '<')
		{
			ptr++;
			LPCSTR startTagPtr = ptr;
			// Start of tag.
			bool quoted = false;
			char quoteChar = 0;
			while (true)
			{
				if (*ptr == 0)
				{
					return "";
				}
				else if (*ptr == '"'  ||  *ptr == '\'')
				{
					if (!quoted)
					{
						quoted = true;
						quoteChar = *ptr;
					}
					else if (*ptr == quoteChar)
					{
						quoted = false;
					}
				}
				else if (*ptr == '\n'  ||  *ptr == '\r'  ||  *ptr == '\t')
				{
					*(char*)ptr = ' ';
				}
				else if (*ptr == '>'  &&  !quoted)
				{
					CString ret(startTagPtr, ptr - startTagPtr);
					ptr++;
					return ret;
				}

				ptr++;
			}
		}

		ptr++;
	}

	return "";
}


/**
**/
static CString ParseToken(LPCSTR& ptr)
{
	// Remove leading whitespace.
	while (*ptr != 0  &&  *ptr == ' ')
	{
		ptr++;
	}
		
	// Get the parameter.
	bool inQuote = false;
	char quoteChar = 0;
	if (*ptr == '"'  ||  *ptr == '\'')
	{
		inQuote = true;
		quoteChar = *ptr;
		ptr++;
	}

	LPCSTR startTokenPtr = ptr;

	while (*ptr != 0)
	{
		if ((*ptr == '/'  ||  *ptr == '=')  &&  !inQuote)
		{
			break;
		}
		else if (*ptr == quoteChar)
		{
			if (inQuote)
			{
				LPCSTR endTokenPtr = ptr;
				ptr++;
				return CString(startTokenPtr, endTokenPtr - startTokenPtr);
			}
			else
			{
				ASSERT(0);
				break;
			}
		}
		else if (*ptr == ' ')
		{
			if (!inQuote)
			{
				break;
			}

			ptr++;
		}
		else
		{
			ptr++;
		}
	}

	return CString(startTokenPtr, ptr - startTokenPtr);
}


/**
**/
static void SkipPastChar(LPCSTR& ptr, char ch)
{
	// Remove leading whitespace.
	while (*ptr != 0)
	{
		if (*ptr == ch)
		{
			ptr++;
			break;
		}
		else if (*ptr == '/')
		{
			// Don't skip terminators.
			break;
		}
		
		ptr++;
	}
}


bool XmlData::ParseXmlFile(const CString& filename)
{
	delete m_xmlRoot;
	m_xmlRoot = NULL;

	CFile file;
	if (!file.Open(filename, CFile::modeRead))
	{
		return false;
	}

	bool ret = ParseXmlFile(file);

	file.Close();

	return ret;
}



bool XmlData::ParseXmlFile(CFile& file)
{
	delete m_xmlRoot;
	m_xmlRoot = NULL;

	DWORD size = (DWORD)file.GetLength();
	char* buf = WNEW char[size + 1];
	file.Read(buf, size);
	buf[size] = 0;

	// Find a tag.
	WList<XmlNode*> xmlNodeStack;
	m_xmlRoot = WNEW XmlNode;
	xmlNodeStack.AddTail(m_xmlRoot);

	LPCSTR bufPos = buf;
	CString lastTag;
	while (true)
	{
		XmlNode* curNode = xmlNodeStack.GetTail();
		CString tag = ReadTag(bufPos);
		if (tag.IsEmpty())
			break;
		lastTag = tag;

		LPCSTR ptr = (LPCTSTR)tag;

		if (*ptr == '/')
		{
			xmlNodeStack.RemoveTail();
			ptr++;
			continue;
		}

		XmlNode* newNode = WNEW XmlNode;
		newNode->m_name = ParseToken(ptr);

		bool closeTag = false;
		while (*ptr)
		{
			XmlNode::Attribute attr;
			attr.m_name = ParseToken(ptr);
			SkipPastChar(ptr, '=');
			attr.m_value = ParseToken(ptr);
			if (*ptr == '/')
			{
				closeTag = true;
				ptr++;
			}

			if (!attr.m_name.IsEmpty())
				newNode->m_attributes.AddTail(attr);
		}

		if (!closeTag)
		{
			xmlNodeStack.AddTail(newNode);
		}

		newNode->AttachNode(curNode);
	}

	delete [] buf;

	return true;
}



XmlNode* XmlData::Find(const CString& name)
{
	if (m_xmlRoot)
		return m_xmlRoot->Find(name);
	return NULL;
}
