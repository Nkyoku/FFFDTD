#include "FFException.h"
#include <stdio.h>
#include <stdarg.h>
#include <sstream>




namespace FFFDTD{
	// コンストラクタ
	FFException::FFException(const char *format, ...){
		va_list list;
		va_start(list, format);
		char buf1[1024];
		vsnprintf(buf1, sizeof(buf1), format, list);
		va_end(list);

		char buf2[2048];
		snprintf(buf2, sizeof(buf2), "An exception occured '%s'", buf1);
		m_Message = buf2;
	}

	// コンストラクタ
	FFException::FFException(int line, const char *file){
		char buf[1024];
		snprintf(buf, sizeof(buf), "An exception occured at line %d of file %s", line, file);
		m_Message = buf;
	}

	// コンストラクタ
	FFException::FFException(int line, const char *file, const char *format, ...){
		va_list list;
		va_start(list, format);
		char buf1[1024];
		vsnprintf(buf1, sizeof(buf1), format, list);
		va_end(list);

		char buf2[1024];
		snprintf(buf2, sizeof(buf2), "An exception occured '%s' at line %d of file %s", buf1, line, file);
		m_Message = buf2;
	}
}
