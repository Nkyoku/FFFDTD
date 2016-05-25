#pragma once

#include "../FFType.h"

#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#elif defined(__GNUC__)
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#else
#error Not implemented
#endif



namespace FFFDTD{
	// 入力ストリーム
	class FFIStream{
	private:
		// バッファの先頭のポインタ
		const uint8_t *m_Head;

		// バッファの現在のポインタ
		const uint8_t *m_Current;

		// バッファの末端のポインタ
		const uint8_t *m_Tail;

#ifdef _WIN32
		// ファイルハンドル
		HANDLE m_hFile;

		// メモリマップトファイルのハンドル
		HANDLE m_hMap;
#elif __GNUC__
		// ファイルディスクリプタ
		int m_hFile;
#endif

	public:
		// コンストラクタ
		FFIStream(void);

		// コピーコンストラクタ
		FFIStream(const FFIStream &stream);

		// 代入
		FFIStream& operator=(const FFIStream &stream);

		// ファイルから入力ストリームを作成する
		FFIStream(const char *filepath);
		
		// 既存のメモリーバッファから入力ストリームを作成する
		FFIStream(const void *data, size_t length);

		// 既存の入力ストリームの一部から入力ストリームを作成する
		FFIStream(const FFIStream &stream, size_t offset, size_t length);

		// デストラクタ
		~FFIStream();
		
		// ストリームの長さを取得する
		size_t length(void) const{
			return m_Tail - m_Head;
		}

		// シークポインタを取得する
		size_t tell(void) const{
			return m_Current - m_Head;
		}

		// シークポインタを設定する
		void seek(size_t offset);

		// データの残りバイト数を取得する
		size_t remaining(void) const{
			return m_Tail - m_Current;
		}

		// 入力ストリームから任意の型を読み取る
		template<typename T>
		T read(void);

		// 入力ストリームから1バイト読み取る
		uint8_t read1byte(void){
			return read<uint8_t>();
		}

		// 入力ストリームから2バイト読み取る
		uint16_t read2byte(void){
			return read<uint16_t>();
		}

		// 入力ストリームから4バイト読み取る
		uint32_t read4byte(void){
			return read<uint32_t>();
		}

		// 入力ストリームから8バイト読み取る
		uint64_t read8byte(void){
			return read<uint64_t>();
		}

		// 入力ストリームから任意バイト読み取る
		void read(void *data, size_t length);

		// 入力ストリームを任意バイト読み飛ばす
		void skip(size_t length);

		// 入力ストリームを読み取ってバイト列を比較する
		bool check(const void *data, size_t length);

	};



	// 入力ストリームから任意の型を読み取る
	template<typename T>
	T FFIStream::read(void){
		if (sizeof(T) <= remaining()){
			auto &p = reinterpret_cast<const T*&>(m_Current);
			return *p++;
		}
		else{
			throw;
		}
	}



}
