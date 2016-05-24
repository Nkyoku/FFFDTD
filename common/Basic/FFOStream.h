#pragma once

#include "FFType.h"
#include <vector>
#include <stdio.h>



namespace FFFDTD{
	// 出力ストリーム
	class FFOStream{
	private:
		// メモリーバッファ
		std::vector<uint8_t> *m_Memory;

		// ファイルポインタ
		FILE *m_Fp;

		// ストリームの長さ
		size_t m_Length;

	private:
		// コンストラクタ
		FFOStream(void);

	public:
		// ファイルへの出力ストリームを作成する
		FFOStream(const char *filepath);
		
		// メモリーバッファへの出力ストリームを作成する
		FFOStream(const void *data, size_t length);

		// デストラクタ
		~FFOStream();
		
		// ストリームの長さを取得する
		size_t length(void) const{
			return m_Length;
		}

		// メモリーバッファへのポインタを取得する
		const void* getBuffer(void) const;

		// 出力ストリームへ任意の型を書き込む
		template<typename T>
		void write(const T &value);

		// 出力ストリームへ1バイト書き込む
		void write1byte(uint8_t value){
			return write<uint8_t>(value);
		}

		// 出力ストリームへ2バイト書き込む
		void write2byte(uint16_t value){
			return write<uint16_t>(value);
		}

		// 出力ストリームへ4バイト書き込む
		void write4byte(uint32_t value){
			return write<uint32_t>(value);
		}

		// 出力ストリームへ8バイト書き込む
		void write8byte(uint64_t value){
			return write<uint64_t>(value);
		}

		// 出力ストリームへ任意バイト数を書き込む
		void write(const void *data, size_t length);

		// 出力ストリームを任意バイト埋める
		void skip(size_t length, uint8_t fill);

	private:
		// コピーを禁止
		FFOStream(const FFOStream &stream);

		// 代入を禁止
		FFOStream& operator=(const FFOStream &stream);
	};



}
