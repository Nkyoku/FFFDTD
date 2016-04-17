#include "FFVoxel2D.h"
#include "../FFConst.h"
#include <memory>



namespace MUFDTD{
	// 2次元ボクセルデータのヘッダー文字列
	const char FFVoxel2D::HEADER_STRING[] = "FF2D";
	


	// コンストラクタ
	FFVoxel2D::FFVoxel2D(index_t wx, index_t wy, bool default_value)
		: m_Size(wx, wy), m_Data(wx * wy, default_value)
	{
		if ((MAX_SIZE < m_Size.x) || (MAX_SIZE < m_Size.y)){
			throw;
		}
	}

	// コンストラクタ
	// 入力ストリームからボクセルデータを読み込む
	// 与えた入力ストリームは削除される
	FFVoxel2D::FFVoxel2D(FFIStream &stream, uint64_t length){
		// ヘッダーをパースする
		uint64_t offset = strlen(HEADER_STRING) + 4 + 4;
		if (length < offset){
			throw;
		}
		if (stream.check(HEADER_STRING, strlen(HEADER_STRING)) == false){
			throw;
		}
		m_Size.x = stream.get4byte();
		m_Size.y = stream.get4byte();
		if ((MAX_SIZE < m_Size.x) || (MAX_SIZE < m_Size.y)){
			throw;
		}

		// メモリーを確保する
		m_Data.resize(m_Size.x * m_Size.y);

		// デコードする
		// 0の個数 → 1の個数 → 0の個数 → ・・・ という順番に1バイトのデータが続く
		size_t remaining = (size_t)m_Size.x * m_Size.y;
		bool value = false;
		size_t it = 0;
		while (0 < remaining){
			if (length <= offset){
				throw;
			}
			uint8_t count = stream.get1byte();
			if (remaining < count){
				throw;
			}
			remaining -= count;
			while (0 < count--){
				m_Data[it++] = value;
			}
			value = !value;
		}
	}

	// 指定したY座標のラインデータを取得する
	void FFVoxel2D::getLineInternal(std::vector<bool> &result, index_t y) const{
		result.resize(m_Size.x);
		size_t size = m_Size.x;
		size_t start = size * y;
		for (size_t i = 0; i < size; i++){
			result[i] = m_Data[start + i];
		}
	}




}

