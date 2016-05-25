#pragma once

#include "../FFConst.h"
//#include "../Basic/FFIStream.h"
#include "FFBitSliceData.h"



namespace FFFDTD{
	// 2値データの空間配置を保持するクラス
	class FFBitVolumeData{
	private:
		// ボリュームデータのヘッダー文字列
		static const char HEADER_STRING[];

		// ボリュームの大きさ
		index3_t m_Size;

		// スライスデータ
		std::vector<FFBitSliceData*> m_Data;

	public:
		// コンストラクタ
		FFBitVolumeData() : m_Size(0, 0, 0){}

		// コンストラクタ
		// 任意サイズのボリュームデータを作成する
		FFBitVolumeData(index_t wx, index_t wy, index_t wz);

		// デストラクタ
		~FFBitVolumeData();

		// 入力ストリームからボリュームデータを読み込む
		// z_start, z_endに指定された範囲のみスライスデータを展開する
		// z_zero=trueのとき、Z=0は特別にスライスデータを展開する
		//void loadFromIStream(FFIStream &stream, index_t z_start = 0, index_t z_end = MAX_SIZE - 1, bool z_zero = false);

		// スライスを作成する
		void createSlices(index_t z_start, index_t z_end, bool default_value);

		// スライスを削除する
		void deleteSlices(index_t z_start, index_t z_end);

		// スライスが存在するか確かめる
		bool isSliceExisted(index_t z) const;

		// ボリュームの大きさを取得する
		const index3_t& getSize(void) const{
			return m_Size;
		}

		// スライスの大きさを取得する
		index2_t getSliceSize(void) const{
			return index2_t(m_Size.x, m_Size.y);
		}

		// スライス数を取得する
		index_t getNumOfSlices(void) const{
			return m_Size.z;
		}

		// 指定した座標の2値データを取得する
		// 範囲外の座標が指定された場合は例外を発生する
		bool getPoint(index_t x, index_t y, index_t z) const{
			if ((m_Size.x <= x) || (m_Size.y <= y) || (m_Size.z <= z)){
				throw;
			}
			return getPointInternal(x, y, z);
		}

		// 指定した座標の2値データを取得する
		// 範囲外の座標には境界値を返す
		bool getPointClamp(sindex_t x, sindex_t y, sindex_t z) const{
			if ((m_Size.x == 0) || (m_Size.y == 0) || (m_Size.z == 0)){
				throw;
			}
			index_t x_ = (x < 0) ? 0 : ((sindex_t)m_Size.x <= x) ? m_Size.x - 1 : x;
			index_t y_ = (y < 0) ? 0 : ((sindex_t)m_Size.y <= y) ? m_Size.y - 1 : y;
			index_t z_ = (z < 0) ? 0 : ((sindex_t)m_Size.z <= z) ? m_Size.z - 1 : z;
			return getPointInternal(x_, y_, z_);
		}

		// 指定した座標の2値データを取得する
		// 正端は原点へループする
		bool getPointRepeat(index_t x, index_t y, index_t z) const{
			index_t x_ = (m_Size.x == x) ? 0 : x;
			index_t y_ = (m_Size.y == y) ? 0 : y;
			index_t z_ = (m_Size.z == z) ? 0 : z;
			return getPoint(x_, y_, z_);
		}

		// 指定したZ座標のスライスデータを取得する
		// 範囲外の座標が指定された場合は例外を発生する
		FFBitSliceData* getSlice(index_t z);

		// 指定したZ座標のスライスデータを取得する
		// 正端は原点へループする
		FFBitSliceData* getSliceRepeat(index_t z);

		// 指定した座標の2値データを書き換える
		void setPoint(index_t x, index_t y, index_t z, bool value){
			if ((m_Size.z <= z) || (m_Data[z] == nullptr)){
				throw;
			}
			(m_Data[z])->setPointInternal(x, y, value);
		}
	private:
		// 指定した座標の2値データを取得する
		bool getPointInternal(index_t x, index_t y, index_t z) const{
			if (m_Data[z] == nullptr){
				throw;
			}
			return (m_Data[z])->getPointInternal(x, y);
		}
	};
}
