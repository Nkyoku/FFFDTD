#ifndef _WIN32
#define _FILE_OFFSET_BITS 64
#endif
#include "FFIStream.h"
#include <sys/stat.h>



namespace MUFDTD{
	
	
	/*** FFIStream ***/
	
	// 入力ストリームから2バイト読み取る
	uint16_t FFIStream::get2byte(void){
		uint16_t result;
		result  = (uint16_t)get1byte() << 8;
		result |= (uint16_t)get1byte();
		return result;
	}

	// 入力ストリームから4バイト読み取る
	uint32_t FFIStream::get4byte(void){
		uint32_t result;
		result  = (uint32_t)get1byte() << 24;
		result |= (uint32_t)get1byte() << 16;
		result |= (uint32_t)get1byte() << 8;
		result |= (uint32_t)get1byte();
		return result;
	}

	// 入力ストリームから8バイト読み取る
	uint64_t FFIStream::get8byte(void){
		uint64_t result;
		result  = (uint64_t)get1byte() << 56;
		result |= (uint64_t)get1byte() << 48;
		result |= (uint64_t)get1byte() << 40;
		result |= (uint64_t)get1byte() << 32;
		result |= (uint64_t)get1byte() << 24;
		result |= (uint64_t)get1byte() << 16;
		result |= (uint64_t)get1byte() << 8;
		result |= (uint64_t)get1byte();
		return result;
	}

	// 入力ストリームから任意バイト読み取る
	void FFIStream::get(void *p_, size_t length){
		if (p_ != nullptr){
			uint8_t *p = reinterpret_cast<uint8_t*>(p_);
			while (0 < length--){
				*p++ = get1byte();
			}
		}
		else{
			throw;
		}
	}
	
	// 入力ストリームを任意バイト読み飛ばす
	void FFIStream::skip(size_t length){
		while (0 < length--){
			get1byte();
		}
	}

	// 入力ストリームを読み取ってバイト列を比較する
	bool FFIStream::check(const void *p_, size_t length){
		const uint8_t *p = reinterpret_cast<const uint8_t*>(p_);
		while (0 < length--){
			if (*p++ != get1byte()){
				skip(length);
				return false;
			}
		}
		return true;
	}



	/*** FFIFileStream ***/

	// コンストラクタ
	FFIFileStream::FFIFileStream(const char *path){
		// ファイルサイズを取得する
#ifdef _WIN32
		struct _stat64 status;
		if (_stat64(path, &status) != 0){
			throw;
		}
#else
		struct stat status;
		if (stat(path, &status) != 0){
			throw;
		}
#endif
		m_Length = static_cast<uint64_t>(status.st_size);

		// ファイルを開く
		m_fp = fopen(path, "rb");
		if (m_fp != nullptr){
			throw;
		}
	}

	// デストラクタ
	FFIFileStream::~FFIFileStream(){
		if (m_fp != nullptr){
			fclose(m_fp);
		}
	}

	// シークポインタを取得する
	uint64_t FFIFileStream::tell(void) const{
#ifdef _WIN32
		return static_cast<uint64_t>(_ftelli64(m_fp));
#else
		return static_cast<uint64_t>(ftello(m_fp));
#endif
	}

	// シークポインタを設定する
	void FFIFileStream::seek(uint64_t offset){
		if (m_Length < offset){
			throw;
		}
#ifdef _WIN32
		_fseeki64(m_fp, offset, SEEK_SET);
#else
		fseeko(m_fp, offset, SEEK_SET);
#endif
	}

	// 入力ストリームから1バイト読み取る
	uint8_t FFIFileStream::get1byte(void){
		int c = fgetc(m_fp);
		if (c == EOF){
			throw;
		}
		return static_cast<uint8_t>(c);
	}
	
	

	/*** FFIMemoryStream ***/

	// コンストラクタ
	FFIMemoryStream::FFIMemoryStream(const void *data, size_t length){
		m_Head = reinterpret_cast<const uint8_t*>(data);
		m_Current = m_Head;
		m_Tail = m_Head + length;
	}

	// シークポインタを設定する
	void FFIMemoryStream::seek(uint64_t offset){
		if ((m_Tail - m_Head) < offset){
			throw;
		}
		m_Current = m_Head + offset;
	}

	// 入力ストリームから1バイト読み取る
	uint8_t FFIMemoryStream::get1byte(void){
		if (m_Tail <= m_Current){
			throw;
		}
		return *m_Current++;
	}



	/*** FFIStreamInIStream ***/
	
	/*// コンストラクタ
	FFIStreamInIStream::FFIStreamInIStream(FFIStream &stream){
		m_Stream = &stream;
		m_Offset = stream.tell();
		m_Length = stream.length() - stream.tell();
	}

	// コンストラクタ
	FFIStreamInIStream::FFIStreamInIStream(FFIStream &stream, uint64_t length){
		m_Stream = &stream;
		m_Offset = stream.tell();
		m_Length = stream.length() - m_Offset;
		if (length <= m_Length){
			m_Length = length;
		}
		else{
			throw;
		}
	}

	// コンストラクタ
	FFIStreamInIStream::FFIStreamInIStream(FFIStream &stream, uint64_t offset, uint64_t length){
		m_Stream = &stream;
		if (m_Stream->length() < offset){
			throw;
		}
		m_Offset = offset;
		m_Length = m_Stream->length() - m_Offset;
		if (length <= m_Length){
			m_Length = length;
		}
		else{
			throw;
		}
	}

	// シークポインタを取得する
	uint64_t FFIStreamInIStream::tell(void) const{
		uint64_t offset = m_Stream->tell();
		if (offset < m_Offset){
			throw;
		}
		offset -= m_Offset;
		if (m_Length < offset){
			throw;
		}
		return offset;
	}

	// シークポインタを設定する
	void FFIStreamInIStream::seek(uint64_t offset){
		if (m_Length < offset){
			throw;
		}
		m_Stream->seek(m_Offset + offset);
	}*/











}

