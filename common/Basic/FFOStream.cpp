#include "FFOStream.h"



namespace MUFDTD{
	// コンストラクタ
	FFOStream::FFOStream(void)
		: m_Memory(nullptr)
		, m_Fp(nullptr)
		, m_Length(0)
	{

	}

	// ファイルから入力ストリームを作成する
	FFOStream::FFOStream(const wchar_t *filepath) : FFOStream(){
		m_Fp = _wfopen(filepath, L"wb");
		if (m_Fp == nullptr){
			throw;
		}
	}

	// 既存のメモリーバッファから入力ストリームを作成する
	FFOStream::FFOStream(const void *data, size_t length) : FFOStream(){
		m_Memory = new std::vector<uint8_t>();
	}
	
	// デストラクタ
	FFOStream::~FFOStream(){
		if (m_Fp != nullptr){
			fclose(m_Fp);
			m_Fp = nullptr;
		}
		if (m_Memory != nullptr){
			delete m_Memory;
			m_Memory = nullptr;
		}
	}

	// メモリーバッファへのポインタを取得する
	const void* FFOStream::getBuffer(void) const{
		if (m_Memory != nullptr){
			return m_Memory->data();
		}
		return nullptr;
	}

	// 出力ストリームへ任意の型を書き込む
	template<typename T>
	void FFOStream::write(const T &value){
		if (m_Fp != nullptr){
			if (fwrite(&value, sizeof(T), 1, m_Fp) != 1){
				throw;
			}
		}
		else{
			const uint8_t *p = reinterpret_cast<const uint8_t*>(&value);
			for (size_t i = 0; i < sizeof(T); i++){
				m_Memory->push_back(*p++);
			}
		}
		if ((m_Length + sizeof(T)) < m_Length){
			throw;
		}
		m_Length += sizeof(T);
	}

	// 出力ストリームへ任意バイト数を書き込む
	void FFOStream::write(const void *data, size_t length){
		if (m_Fp != nullptr){
			if (fwrite(data, 1, length, m_Fp) != length){
				throw;
			}
		}
		else{
			const uint8_t *p = reinterpret_cast<const uint8_t*>(data);
			for (size_t i = 0; i < length; i++){
				m_Memory->push_back(*p++);
			}
		}
		if ((m_Length + length) < m_Length){
			throw;
		}
		m_Length += length;
	}

	// 出力ストリームを任意バイト埋める
	void FFOStream::skip(size_t length, uint8_t fill){
		while (0 < length--){
			write1byte(fill);
		}
	}



}

