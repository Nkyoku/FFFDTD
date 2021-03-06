﻿#include "FFSliceData.h"
#include "../FFConst.h"



namespace FFFDTD{
	// スライスデータのヘッダー文字列
	const char FFSliceData::HEADER_STRING[4] = {'F', 'F', 'S', 'L'};
	


	// コンストラクタ
	// 任意サイズのスライスデータを作成する
	FFSliceData::FFSliceData(index_t wx, index_t wy, matid_t default_matid)
		: m_Size(wx, wy), m_Data(wx * wy, default_matid)
	{
		if ((MAX_SIZE < m_Size.x) || (MAX_SIZE < m_Size.y)){
			throw;
		}
	}

	// コンストラクタ
	// 入力ストリームからスライスデータを読み込む
	FFSliceData::FFSliceData(FFIStream &stream){
		loadFromIStream(stream, stream.length());
	}

	// コンストラクタ
	// 入力ストリームからスライスデータを読み込む
	FFSliceData::FFSliceData(FFIStream &stream, uint64_t length){
		loadFromIStream(stream, length);
	}

	// 入力ストリームからスライスデータを読み込む
	void FFSliceData::loadFromIStream(FFIStream &stream, uint64_t length){
		// ヘッダーをパースする
		uint64_t offset = sizeof(HEADER_STRING) + 4 + 4 + 4;
		uint32_t bytes_per_matid;
		if (length < offset){
			throw;
		}
		if (stream.check(HEADER_STRING, sizeof(HEADER_STRING)) == false){
			throw;
		}
		m_Size.x = stream.read4byte();
		m_Size.y = stream.read4byte();
		bytes_per_matid = stream.read4byte();
		if ((MAX_SIZE < m_Size.x) || (MAX_SIZE < m_Size.y)){
			throw;
		}
		stream.read1byte();
		// デコードする
		size_t remaining = (size_t)m_Size.x * m_Size.y;
		m_Data.resize(remaining);
		auto it = m_Data.begin();
		if (bytes_per_matid == 1){
			// PECフラグと材質IDは1バイトで表される
			while (0 < remaining){
				offset += 2;
				if (length < offset){
					throw;
				}
				matid_t matid = stream.read1byte();
				uint8_t count = stream.read1byte();
				if (remaining < count){
					throw;
				}
				while (0 < count--){
					*it++ = matid;
				}
			}
		}
		else if (bytes_per_matid == 2){
			// PECフラグと材質IDは2バイトで表される
			while (0 < remaining){
				offset += 3;
				if (length < offset){
					throw;
				}
				matid_t matid = stream.read2byte();
				uint8_t count = stream.read1byte();
				if (remaining < count){
					throw;
				}
				while (0 < count--){
					*it++ = matid;
				}
			}
		}
		else{
			throw;
		}
	}

	// 使用されている最大の材質IDを計算する
	matid_t FFSliceData::getMaximumMaterialID(void) const{
		matid_t matid = 0;
		for (matid_t m : m_Data){
			if (matid < m){
				matid = m;
			}
		}
		return matid;
	}


}

