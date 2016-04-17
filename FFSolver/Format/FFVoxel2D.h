#pragma once

#include "../FFType.h"
#include "../Basic/FFIStream.h"



namespace MUFDTD{
	// 3次元のボクセルデータを保持するクラス
	class FFVoxel3D;
	
	// 2次元ボクセルデータを保持するクラス
	class FFVoxel2D{
		friend class FFVoxel3D;

	private:
		// 2次元ボクセルデータのヘッダー文字列
		static const char HEADER_STRING[];

		// ボクセルデータの大きさ
		index2_t m_Size;

		// ボクセルデータ
		std::vector<bool> m_Data;

	public:
		// コンストラクタ
		FFVoxel2D(void) : m_Size(0, 0){}

		// コンストラクタ
		FFVoxel2D(index_t wx, index_t wy, bool default_value = false);
		
		// コンストラクタ
		// 入力ストリームからボクセルデータを読み込む
		// 与えた入力ストリームは削除される
		FFVoxel2D(FFIStream &stream) : FFVoxel2D(stream, stream.length()){}

		// コンストラクタ
		// 入力ストリームからボクセルデータを読み込む
		// 与えた入力ストリームは削除される
		FFVoxel2D(FFIStream &stream, uint64_t length);

		// ボクセルデータを出力ストリームに格納する
		//void storeToOStream(void) const;

		// ボクセルデータの大きさを取得する
		const index2_t& getSize(void) const{
			return m_Size;
		}

		// 指定した座標のボクセルデータを取得する
		// 範囲外の座標が指定された場合は例外を発生する
		bool getPoint(index_t x, index_t y) const{
			if ((m_Size.x <= x) || (m_Size.y <= y)){
				throw;
			}
			return getPointInternal(x, y);
		}

		// 指定した座標のボクセルデータを取得する
		// 範囲外の座標にはborderで指定した値を返す
		bool getPointBorder(index_t x, index_t y, bool border = false) const{
			if ((m_Size.x <= x) || (m_Size.y <= y)){
				return border;
			}
			return getPointInternal(x, y);
		}

		// 指定した座標のボクセルデータを取得する
		// 範囲外の座標には境界値を返す
		bool getPointClamp(int32_t x, int32_t y) const{
			if ((m_Size.x == 0) || (m_Size.y == 0)){
				throw;
			}
			index_t x_ = (x < 0) ? 0 : ((int32_t)m_Size.x <= x) ? m_Size.x - 1 : x;
			index_t y_ = (y < 0) ? 0 : ((int32_t)m_Size.y <= y) ? m_Size.y - 1 : y;
			return getPointInternal(x_, y_);
		}

		// 指定したY座標のラインデータを取得する
		// 範囲外の座標が指定された場合は例外を発生する
		void getLine(std::vector<bool> &result, index_t y) const{
			if (m_Size.y <= y){
				throw;
			}
			getLineInternal(result, y);
		}

		// 指定したY座標のラインデータを取得する
		// 範囲外の座標にはborderで指定した値を返す
		void getLineBorder(std::vector<bool> &result, index_t y, bool border = false) const{
			if (m_Size.y <= y){
				result = std::vector<bool>(m_Size.x, border);
			}
			else{
				getLineInternal(result, y);
			}
		}

		// 指定したY座標のラインデータを取得する
		// 範囲外の座標には境界値を返す
		void getLineClamp(std::vector<bool> &result, int32_t y) const{
			if (m_Size.y == 0){
				throw;
			}
			index_t y_ = (y < 0) ? 0 : ((int32_t)m_Size.y <= y) ? m_Size.y - 1 : y;
			getLineInternal(result, y_);
		}

		// 指定した座標のボクセルデータを書き換える
		void setPoint(index_t x, index_t y, bool value){
			if ((m_Size.x <= x) || (m_Size.y <= y)) throw;
			setPointInternal(x, y, value);
		}

	private:
		// 指定した座標のボクセルデータを取得する
		bool getPointInternal(index_t x, index_t y) const{
			return m_Data[(size_t)x + (size_t)m_Size.x * y];
		}

		// 指定したY座標のラインデータを取得する
		void getLineInternal(std::vector<bool> &result, index_t y) const;

		// 指定した座標のボクセルデータを書き換える
		void setPointInternal(index_t x, index_t y, bool value){
			m_Data[(size_t)x + (size_t)m_Size.x * y] = value;
		}
	};
}
