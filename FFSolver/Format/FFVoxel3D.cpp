#include "FFVoxel3D.h"
#include "../FFConst.h"
#include <memory>



namespace MUFDTD{
	// 2次元ボクセルデータのヘッダー文字列
	const char FFVoxel2D::HEADER_STRING[] = "FF3D";

	
	/*
#pragma pack(1)
	// ヘッダーのフォーマット
	struct HEADER_t{
		uint32_t header_string;		// ヘッダー文字列
		uint32_t size_x;			// ボクセルデータの大きさ(X方向)
		uint32_t size_y;			// ボクセルデータの大きさ(Y方向)
		uint32_t size_z;			// ボクセルデータの大きさ(Z方向)
		SLICEINFO_t sliceinfo[];	// スライス情報(size_z個)
	};

	// スライス情報のフォーマット
	struct SLICEINFO_t{
		uint64_t offset;			// ヘッダーの始めからのオフセット
		uint64_t length;			// スライスの長さ
	};
#pragma pack()
	*/



	// コンストラクタ
	FFVoxel3D::FFVoxel3D(FFIStream *stream_)
		: m_Stream(stream_), m_Size(0, 0, 0)
	{
		std::unique_ptr<FFIStream> stream(stream_);

		// ヘッダーをパースする
		if (stream->check(HEADER_STRING, strlen(HEADER_STRING)) == false) throw;
		m_Size.x = stream->get4byte();
		m_Size.y = stream->get4byte();
		m_Size.z = stream->get4byte();
		if ((m_Size.x < 0) || (MAX_SIZE < m_Size.x)) throw;
		if ((m_Size.y < 0) || (MAX_SIZE < m_Size.y)) throw;
		if ((m_Size.z < 0) || (MAX_SIZE < m_Size.z)) throw;

		// スライス情報をパースする
		uint64_t last_offset = stream->tell() + m_Size.z * 2 * sizeof(uint64_t);
		for (uint32_t rz = m_Size.z; 0 < rz; rz--){
			SLICEINFO_t info;
			info.offset = stream->get8byte();
			info.length = stream->get8byte();
			if (info.offset < last_offset) throw;
			if ((info.offset + info.length) <= last_offset) throw;
			last_offset += info.offset + info.length;
			m_SliceInfo.push_back(info);
		}
		if (stream->length() < last_offset) throw;
	}

	// デストラクタ
	FFVoxel3D::~FFVoxel3D(){
		delete m_Stream;
	}

	// 1スライスを展開する
	std::vector<bool>* FFVoxel3D::loadSlice(index_t z) const{
		if (m_Size.z <= z) throw;

		// スライスのボクセルデータの場所へシークする
		m_Stream->seek(m_SliceInfo[z].offset);

		uint64_t length = m_SliceInfo[z].length;
		uint64_t remaining = (uint64_t)m_Size.x * m_Size.y;

		while (1 < length){




			matid_t matid = m_Stream->get1byte();
			index_t count = m_Stream->get1byte();
			length -= 2 * sizeof(uint8_t);
			if (remaining < count) throw;
			remaining -= count;
			while (0 < count--){
				*data = matid;
			}
		}
		if ((length != 0) || (remaining != 0)) throw;
	}
}

