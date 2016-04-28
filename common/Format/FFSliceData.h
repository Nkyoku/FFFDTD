#pragma once

#include "FFType.h"
#include "Basic/FFIStream.h"



namespace MUFDTD{
	// 材質の空間配置を保持するクラス
	class FFVolumeData;
	
	// 材質の平面内配置を保持するクラス
	class FFSliceData{
		friend class FFVolumeData;

	private:
		// スライスデータのヘッダー文字列
		static const char HEADER_STRING[];

		// スライスの大きさ
		index2_t m_Size;

		// 材質の平面内配置
		std::vector<matid_t> m_Data;

	public:
		// コンストラクタ
		FFSliceData(void) : m_Size(0, 0){}

		// コンストラクタ
		// 任意サイズのスライスデータを作成する
		FFSliceData(index_t wx, index_t wy, matid_t default_matid);
		
		// コンストラクタ
		// 入力ストリームからスライスデータを読み込む
		FFSliceData(FFIStream &stream);

		// コンストラクタ
		// 入力ストリームからスライスデータを読み込む
		FFSliceData(FFIStream &stream, uint64_t length);

		// 入力ストリームからスライスデータを読み込む
		void loadFromIStream(FFIStream &stream, uint64_t length);

		// スライスデータを出力ストリームに格納する
		//void storeToOStream(void) const;

		// スライスの大きさを取得する
		const index2_t& getSize(void) const{
			return m_Size;
		}

		// 指定した座標の材質IDを取得する
		// 範囲外の座標が指定された場合は例外を発生する
		matid_t getPoint(index_t x, index_t y) const{
			if ((m_Size.x <= x) || (m_Size.y <= y)){
				throw;
			}
			return getPointInternal(x, y);
		}

		// 指定した座標の材質IDを取得する
		// 範囲外の座標には境界値を返す
		matid_t getPointClamp(sindex_t x, sindex_t y) const{
			if ((m_Size.x == 0) || (m_Size.y == 0)){
				throw;
			}
			index_t x_ = (x < 0) ? 0 : ((sindex_t)m_Size.x <= x) ? m_Size.x - 1 : x;
			index_t y_ = (y < 0) ? 0 : ((sindex_t)m_Size.y <= y) ? m_Size.y - 1 : y;
			return getPointInternal(x_, y_);
		}

		// 指定した座標の材質IDを取得する
		// 正端は原点へループする
		matid_t getPointRepeat(index_t x, index_t y) const{
			index_t x_ = (m_Size.x == x) ? 0 : x;
			index_t y_ = (m_Size.y == y) ? 0 : y;
			return getPoint(x_, y_);
		}

		// 指定した座標の材質IDを書き換える
		void setPoint(index_t x, index_t y, matid_t matid){
			if ((m_Size.x <= x) || (m_Size.y <= y)) throw;
			setPointInternal(x, y, matid);
		}

		// 使用されている最大の材質IDを計算する
		matid_t getMaximumMaterialID(void) const;

	private:
		// 指定した座標の材質IDを取得する
		matid_t getPointInternal(index_t x, index_t y) const{
			return m_Data[(size_t)x + (size_t)m_Size.x * y];
		}

		// 指定した座標の材質IDを書き換える
		void setPointInternal(index_t x, index_t y, matid_t matid){
			m_Data[(size_t)x + (size_t)m_Size.x * y] = matid;
		}
	};
}
