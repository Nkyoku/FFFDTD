#include "FFVolumeData.h"
#include "../FFConst.h"



namespace FFFDTD{
	// ボリュームデータのヘッダー文字列
	const char FFVolumeData::HEADER_STRING[] = {'F', 'F', 'V', 'L'};

	

	// コンストラクタ
	// 任意サイズのボリュームデータを作成する
	FFVolumeData::FFVolumeData(index_t wx, index_t wy, index_t wz)
		: m_Size(wx, wy, wz), m_Data(wz, nullptr)
	{
		if ((MAX_SIZE < m_Size.x) || (MAX_SIZE < m_Size.y) || (MAX_SIZE < m_Size.z)){
			throw;
		}
	}

	// デストラクタ
	FFVolumeData::~FFVolumeData(){
		for (FFSliceData *p : m_Data){
			if (p != nullptr){
				delete p;
			}
		}
	}

	// 入力ストリームからボリュームデータを読み込む
	// z_start, z_endに指定された範囲のみスライスデータを展開する
	// z_zero=trueのとき、Z=0に関して特別にスライスデータを展開する
	void FFVolumeData::loadFromIStream(FFIStream &stream, index_t z_start, index_t z_end, bool z_zero){
		// 既存のスライスデータを破棄する
		this->~FFVolumeData();

		// ヘッダーをパースする
		if (stream.check(HEADER_STRING, sizeof(HEADER_STRING)) == false){
			throw;
		}
		m_Size.x = stream.read4byte();
		m_Size.y = stream.read4byte();
		m_Size.z = stream.read4byte();
		if ((MAX_SIZE < m_Size.x) || (MAX_SIZE < m_Size.y) || (MAX_SIZE < m_Size.z)){
			throw;
		}
		
		// スライス情報をパースする
		uint64_t total_length = stream.tell() + 4 * m_Size.z;
		std::vector<uint32_t> length_list(m_Size.z, 0);
		for (index_t z = 0; z < m_Size.z; z++){
			uint32_t length = stream.read4byte();
			length_list[z] = length;
			total_length += length;
		}
		if (stream.length() < total_length){
			throw;
		}

		m_Data.resize(m_Size.z);
		for (index_t z = 0; z < m_Size.z; z++){
			uint64_t slice_offset = stream.tell();
			uint32_t slice_length = length_list[z];
			if (((z_start <= z) && (z <= z_end)) || ((z == 0) && z_zero)){
				m_Data[z] = new FFSliceData(stream, (uint64_t)slice_length);
				index2_t slice_size = (m_Data[z])->getSize();
				if ((m_Size.x != slice_size.x) || (m_Size.y != slice_size.y)){
					throw;
				}
			}
			else{
				m_Data[z] = nullptr;
			}
			stream.seek(slice_offset + slice_length);
		}
	}

	// スライスを作成する
	void FFVolumeData::createSlices(index_t z_start, index_t z_end, matid_t default_matid){
		if ((z_end < z_start) || (m_Size.z <= z_start) || (m_Size.z <= z_end)){
			throw;
		}
		for (index_t z = z_start; z <= z_end; z++){
			if (m_Data[z] != nullptr){
				// すでにスライスデータが存在する場合は削除する
				delete m_Data[z];
			}
			m_Data[z] = new FFSliceData(m_Size.x, m_Size.y, default_matid);
		}
	}

	// スライスを削除する
	void FFVolumeData::deleteSlices(index_t z_start, index_t z_end){
		if ((z_end < z_start) || (m_Size.z <= z_start) || (m_Size.z <= z_end)){
			throw;
		}
		for (index_t z = z_start; z <= z_end; z++){
			if (m_Data[z] != nullptr){
				delete m_Data[z];
				m_Data[z] = nullptr;
			}
		}
	}
	
	// スライスが存在するか確かめる
	bool FFVolumeData::isSliceExisted(index_t z) const{
		if (m_Size.z <= z){
			throw;
		}
		return (m_Data[z] != nullptr);
	}

	// 指定したZ座標のスライスデータを取得する
	// 範囲外の座標が指定された場合は例外を発生する
	FFSliceData* FFVolumeData::getSlice(index_t z){
		if (m_Size.z <= z){
			throw;
		}
		return m_Data[z];
	}

	// 使用されている最大の材質IDを計算する
	matid_t FFVolumeData::getMaximumMaterialID(void) const{
		matid_t matid = 0;
		for (FFSliceData *p : m_Data){
			if (p != nullptr){
				matid_t m = p->getMaximumMaterialID();
				if (matid < m){
					matid = m;
				}
			}
		}
		return matid;
	}


}

