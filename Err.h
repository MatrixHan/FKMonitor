#pragma once
#include <string>
using namespace std;
class ERR{
	string m_ErrString;
	bool m_IsOK;
public :
	ERR(string errString);
	ERR & operator= (const ERR & rhs);
	bool operator == (const ERR & rhs) const;
	bool IsOK() { return m_IsOK; };
	string String() { return m_ErrString; };
	ERR();
	~ERR() {};
};
#define OK ERR()