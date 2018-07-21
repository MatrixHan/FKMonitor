#include "Err.h"

ERR::ERR(string errString):m_ErrString(errString),m_IsOK(false){
}
ERR::ERR():m_ErrString(""),m_IsOK(true){
}
ERR & ERR::operator=(const ERR & rhs) {
	m_ErrString = rhs.m_ErrString;
	m_IsOK = rhs.m_IsOK;
	return *this;
}
bool ERR::operator==(const ERR & rhs) const {
	return (m_ErrString == rhs.m_ErrString &&
	        m_IsOK == rhs.m_IsOK);
}