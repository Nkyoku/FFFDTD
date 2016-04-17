#pragma once

#include "../FFType.h"
#include "../Basic/FFIStream.h"
#include "FFVoxel2D.h"



namespace MUFDTD{
	// 3次元のボクセルデータを保持するクラス
	class FFVoxel3D{
	private:
		// 3次元ボクセルデータのヘッダー文字列
		static const char HEADER_STRING[];

		// ボクセルデータの大きさ
		index3_t m_Size;

		// ボクセルデータ
		std::vector<FFVoxel2D> m_Data;

	public:
		// コンストラクタ
		FFVoxel3D();

		// 入力ストリームからボクセルデータを読み込む
		// 与えた入力ストリームは削除される
		void loadFromIStream(FFIStream &stream);

		// ボクセルデータの大きさを取得する
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

		// 指定した座標のボクセルデータを取得する
		// 範囲外の座標が指定された場合は例外を発生する
		bool getPoint(index_t x, index_t y, index_t z) const{
			if ((m_Size.x <= x) || (m_Size.y <= y) || (m_Size.z <= z)){
				throw;
			}
			return m_Data[z].getPointInternal(x, y);
		}

		// 指定した座標のボクセルデータを取得する
		// 範囲外の座標にはborderで指定した値を返す
		bool getPointBorder(index_t x, index_t y, index_t z, bool border = false) const{
			if ((m_Size.x <= x) || (m_Size.y <= y) || (m_Size.z <= z)){
				return border;
			}
			return m_Data[z].getPointInternal(x, y);
		}

		// 指定した座標のボクセルデータを取得する
		// 範囲外の座標には境界値を返す
		bool getPointClamp(int32_t x, int32_t y, int32_t z) const{
			if ((m_Size.x == 0) || (m_Size.y == 0) || (m_Size.z == 0)){
				throw;
			}
			index_t x_ = (x < 0) ? 0 : ((int32_t)m_Size.x <= x) ? m_Size.x - 1 : x;
			index_t y_ = (y < 0) ? 0 : ((int32_t)m_Size.y <= y) ? m_Size.y - 1 : y;
			index_t z_ = (z < 0) ? 0 : ((int32_t)m_Size.z <= z) ? m_Size.z - 1 : z;
			return m_Data[z_].getPointInternal(x_, y_);
		}

		// 指定したYZ座標のラインデータを取得する
		// 範囲外の座標が指定された場合は例外を発生する
		void getLine(std::vector<bool> &result, index_t y, index_t z) const{
			if ((m_Size.y <= y) || (m_Size.z <= z)){
				throw;
			}
			m_Data[z].getLineInternal(result, y);
		}

		// 指定したYZ座標のラインデータを取得する
		// 範囲外の座標にはborderで指定した値を返す
		void getLineBorder(std::vector<bool> &result, index_t y, index_t z, bool border = false) const{
			if ((m_Size.y <= y) || (m_Size.z <= z)){
				result = std::vector<bool>(m_Size.x, border);
			}
			else{
				m_Data[z].getLineInternal(result, y);
			}
		}

		// 指定したYZ座標のラインデータを取得する
		// 範囲外の座標には境界値を返す
		void getLineClamp(std::vector<bool> &result, int32_t y, int32_t z) const{
			if (m_Size.y == 0){
				throw;
			}
			index_t y_ = (y < 0) ? 0 : ((int32_t)m_Size.y <= y) ? m_Size.y - 1 : y;
			index_t z_ = (z < 0) ? 0 : ((int32_t)m_Size.z <= z) ? m_Size.z - 1 : z;
			m_Data[z_].getLineInternal(result, y_);
		}

		// 指定したZ座標のスライスデータを取得する
		// 範囲外の座標が指定された場合は例外を発生する
		const FFVoxel2D& getSlice(index_t z) const{
			if (m_Size.z <= z){
				throw;
			}
			return m_Data[z];
		}

		// 指定した座標のボクセルデータを書き換える
		void setPoint(index_t x, index_t y, index_t z, bool value){
			if ((m_Size.x <= x) || (m_Size.y <= y) || (m_Size.z <= z)){
				throw;
			}
			setPointInternal(x, y, z, value);
		}

	private:
		// 指定した座標のボクセルデータを取得する
		bool getPointInternal(index_t x, index_t y, index_t z) const{
			return m_Data[z].getPointInternal(x, y);
		}

		// 指定した座標のボクセルデータを書き換える
		void setPointInternal(index_t x, index_t y, index_t z, bool value){
			m_Data[z].setPointInternal(x, y, value);
		}
	};
}
