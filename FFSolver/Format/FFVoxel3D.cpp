#include "FFVoxel3D.h"
#include "../FFConst.h"
#include <memory>



namespace MUFDTD{
	// 2次元ボクセルデータのヘッダー文字列
	const char FFVoxel2D::HEADER_STRING[] = "FF3D";

	

	// コンストラクタ
	FFVoxel3D::FFVoxel3D(void)
		: m_Size(0, 0, 0)
	{

	}

	// 入力ストリームからボクセルデータを読み込む
	// 与えた入力ストリームは削除される
	void FFVoxel3D::loadFromIStream(FFIStream &stream){
		// ヘッダーをパースする
		if (stream.check(HEADER_STRING, strlen(HEADER_STRING)) == false){
			throw;
		}
		m_Size.x = stream.get4byte();
		m_Size.y = stream.get4byte();
		m_Size.z = stream.get4byte();
		if ((m_Size.x < 0) || (MAX_SIZE < m_Size.x) ||
			(m_Size.y < 0) || (MAX_SIZE < m_Size.y) ||
			(m_Size.z < 0) || (MAX_SIZE < m_Size.z)){
			throw;
		}
		m_Data.resize(m_Size.z);

		// スライス情報をパースする
		uint64_t total_length = stream.tell() + 4 * m_Size.z;
		std::vector<uint32_t> length_list(m_Size.z, 0);
		for (index_t z = 0; z < m_Size.z; z++){
			uint32_t length = stream.get4byte();
			length_list[z] = length;
			total_length += length;
		}
		if (stream.length() < total_length){
			throw;
		}
		for (index_t z = 0; z < m_Size.z; z++){
			uint64_t slice_offset = stream.tell();
			uint32_t slice_length = length_list[z];
			m_Data[z].loadFromIStream(stream, slice_length);
			index2_t slice_size = m_Data[z].getSize;
			if ((m_Size.x < slice_size.x) || (m_Size.y < slice_size.y)){
				throw;
			}
			stream.seek(slice_offset + slice_length);
		}
	}






}

