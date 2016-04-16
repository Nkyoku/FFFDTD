#pragma once

#include "../FFType.h"



namespace MUFDTD{
	// 入力ストリームの基底クラス
	class FFIStream{
	public:
		// デストラクタ
		virtual ~FFIStream() = 0;
		
		// ストリームの長さを取得する
		virtual uint64_t length(void) const = 0;

		// シークポインタを取得する
		virtual uint64_t tell(void) const = 0;

		// シークポインタを設定する
		virtual void seek(uint64_t offset) = 0;

		// 入力ストリームから1バイト読み取る
		virtual uint8_t get1byte(void) = 0;

		// 入力ストリームから2バイト読み取る
		uint16_t get2byte(void);

		// 入力ストリームから4バイト読み取る
		uint32_t get4byte(void);

		// 入力ストリームから8バイト読み取る
		uint64_t get8byte(void);

		// 入力ストリームから任意バイト読み取る
		void get(void *p, size_t length);

		// 入力ストリームを任意バイト読み飛ばす
		void skip(size_t length);

		// 入力ストリームを読み取ってバイト列を比較する
		bool check(const void *p, size_t length);
	};



	// ファイルからの入力ストリーム
	class FFIFileStream : public FFIStream{
	private:
		// ファイルポインタ
		FILE *m_fp;

		// ファイルサイズ
		uint64_t m_Length;

	public:
		// コンストラクタ
		FFIFileStream(const char *path);

		// デストラクタ
		~FFIFileStream();

		// ストリームの長さを取得する
		uint64_t length(void) const override{
			return m_Length;
		}

		// シークポインタを取得する
		uint64_t tell(void) const override;

		// シークポインタを設定する
		void seek(uint64_t offset) override;

		// 入力ストリームから1バイト読み取る
		uint8_t get1byte(void) override;
	};



	// メモリーからの入力ストリーム
	class FFIMemoryStream : public FFIStream{
	private:
		// バッファの先頭のポインタ
		const uint8_t *m_Head;

		// バッファの現在のポインタ
		const uint8_t *m_Current;

		// バッファの末端のポインタ
		const uint8_t *m_Tail;

	public:
		// コンストラクタ
		FFIMemoryStream(const void *data, size_t length);

		// デストラクタ
		~FFIMemoryStream(){};

		// ストリームの長さを取得する
		uint64_t length(void) const override{
			return m_Tail - m_Head;
		}

		// シークポインタを取得する
		uint64_t tell(void) const override{
			return m_Current - m_Head;
		}

		// シークポインタを設定する
		void seek(uint64_t offset) override;

		// 入力ストリームから1バイト読み取る
		uint8_t get1byte(void) override;
	};



	/*// 入力ストリームからの入力ストリーム
	class FFIStreamInIStream : public FFIStream{
	private:
		// 入力ストリーム
		FFIStream *m_Stream;

		// 部分入力ストリームのオフセット
		uint64_t m_Offset;

		// 部分入力ストリームの長さ
		uint64_t m_Length;

	public:
		// コンストラクタ
		FFIStreamInIStream(FFIStream &stream);

		// コンストラクタ
		FFIStreamInIStream(FFIStream &stream, uint64_t length);

		// コンストラクタ
		FFIStreamInIStream(FFIStream &stream, uint64_t offset, uint64_t length);

		// ストリームの長さを取得する
		uint64_t length(void) const override{
			return m_Length;
		}

		// シークポインタを取得する
		uint64_t tell(void) const override;

		// シークポインタを設定する
		void seek(uint64_t offset) override;
	};*/


}
