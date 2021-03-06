﻿#include "FFIStream.h"



namespace FFFDTD{
	// コンストラクタ
	FFIStream::FFIStream(void)
		: m_Head(nullptr), m_Current(nullptr), m_Tail(nullptr)
#if defined(_WIN32)
		, m_hFile(INVALID_HANDLE_VALUE), m_hMap(NULL)
#elif defined(__GNUC__)
		, m_hFile(-1)
#endif
	{

	}

	// コピーコンストラクタ
	FFIStream::FFIStream(const FFIStream &stream) : FFIStream(){
		m_Head = stream.m_Head;
		m_Current = stream.m_Current;
		m_Tail = stream.m_Tail;
	}

	// 代入
	FFIStream& FFIStream::operator=(const FFIStream &stream){
		this->~FFIStream();
		m_Head = stream.m_Head;
		m_Current = stream.m_Current;
		m_Tail = stream.m_Tail;
		return *this;
	}

	// ファイルから入力ストリームを作成する
	FFIStream::FFIStream(const char *filepath) : FFIStream(){
		uint64_t length;
#if defined(_WIN32)
		try{
			// ファイルを開く
			m_hFile = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if (m_hFile == INVALID_HANDLE_VALUE){
				throw;
			}

			// ファイルサイズを取得する
			LARGE_INTEGER li;
			GetFileSizeEx(m_hFile, &li);
			length = li.QuadPart;
			if (length == 0){
				throw;
			}

			// メモリマップトファイルオブジェクトを作成する
			m_hMap = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
			if (m_hMap == NULL){
				throw;
			}

			// メモリーに配置する
			m_Head = reinterpret_cast<const uint8_t*>(MapViewOfFile(m_hMap, FILE_MAP_READ, 0, 0, 0));
			if (m_Head == nullptr){
				throw;
			}
		}
		catch (...){
			if (m_hMap != NULL){
				CloseHandle(m_hMap);
				m_hMap = NULL;
			}
			if (m_hFile != INVALID_HANDLE_VALUE){
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
			}
			throw;
		}
#elif defined(__GNUC__)
		// ファイルを開く
		m_hFile = open(filepath, O_RDONLY);
		if (m_hFile == -1){
			throw;
		}

		try{
			// ファイルサイズを取得する
			struct stat result;
			fstat(m_hFile, &result);
			length = result.st_size;
			if (length == 0){
				throw;
			}

			// メモリにマップする
			void *p;
			p = mmap(0, (size_t)length, PROT_READ, MAP_SHARED, m_hFile, 0);
			if (p == MAP_FAILED){
				throw;
			}
			m_Head = reinterpret_cast<const uint8_t*>(p);
		}
		catch (...){
			close(m_hFile);
			m_hFile = -1;
		}
#endif
		m_Current = m_Head;
		m_Tail = m_Head + (size_t)length;
	}

	// 既存のメモリーバッファから入力ストリームを作成する
	FFIStream::FFIStream(const void *data, size_t length) : FFIStream(){
		m_Head = reinterpret_cast<const uint8_t*>(data);
		m_Current = m_Head;
		m_Tail = m_Head + length;
	}

	// 既存の入力ストリームの一部から入力ストリームを作成する
	FFIStream::FFIStream(const FFIStream &stream, size_t offset, size_t length) : FFIStream(){
		if ((offset + length) < offset){
			throw;
		}
		if (stream.length() < (offset + length)){
			throw;
		}
		m_Head = stream.m_Head + offset;
		m_Current = m_Head;
		m_Tail = m_Head + length;
	}

	// デストラクタ
	FFIStream::~FFIStream(){
#if defined(_WIN32)
		if (m_hFile != INVALID_HANDLE_VALUE){
			UnmapViewOfFile(m_Head);
			CloseHandle(m_hFile);
			CloseHandle(m_hMap);
			m_hFile = INVALID_HANDLE_VALUE;
			m_hMap = NULL;
		}
#elif defined(__GNUC__)
		if (m_Head != nullptr){
			munmap(const_cast<uint8_t*>(m_Head), (size_t)length());
			m_Head = nullptr;
		}
		if (m_hFile != -1){
			close(m_hFile);
			m_hFile = -1;
		}
#endif
	}

	// シークポインタを設定する
	void FFIStream::seek(size_t offset){
		if (length() < offset){
			throw;
		}
		m_Current = m_Head + offset;
	}

	// 入力ストリームから任意バイト読み取る
	void FFIStream::read(void *data, size_t length){
		if (length <= remaining()){
			memcpy(data, m_Current, length);
			m_Current += length;
		}
		else{
			throw;
		}
	}
	
	// 入力ストリームを任意バイト読み飛ばす
	void FFIStream::skip(size_t length){
		seek(tell() + length);
	}

	// 入力ストリームを読み取ってバイト列を比較する
	bool FFIStream::check(const void *data, size_t length){
		const uint8_t *p = reinterpret_cast<const uint8_t*>(data);
		while (0 < length--){
			if (*p++ != read1byte()){
				skip(length);
				return false;
			}
		}
		return true;
	}



}

