#pragma once

#include "FFType.h"
//#include "Basic/FFIStream.h"



namespace MUFDTD{
	// 2値データの空間配置を保持するクラス
	class FFBitVolumeData;
	
	// 2値データの平面内配置を保持するクラス
	class FFBitSliceData{
		friend class FFBitVolumeData;

	private:
		// スライスデータのヘッダー文字列
		static const char HEADER_STRING[];

		// スライスの大きさ
		index2_t m_Size;

		// 2値データの平面内配置
		std::vector<bool> m_Data;

	public:
		// コンストラクタ
		FFBitSliceData(void) : m_Size(0, 0){}

		// コンストラクタ
		// 任意サイズのスライスデータを作成する
		FFBitSliceData(index_t wx, index_t wy, bool default_value);
		
		// コンストラクタ
		// 入力ストリームからスライスデータを読み込む
		//FFBitSliceData(FFIStream &stream);

		// コンストラクタ
		// 入力ストリームからスライスデータを読み込む
		//FFBitSliceData(FFIStream &stream, uint64_t length);

		// 入力ストリームからスライスデータを読み込む
		//void loadFromIStream(FFIStream &stream, uint64_t length);

		// スライスデータを出力ストリームに格納する
		//void storeToOStream(void) const;

		// スライスの大きさを取得する
		const index2_t& getSize(void) const{
			return m_Size;
		}

		// 指定した座標の2値データを取得する
		// 範囲外の座標が指定された場合は例外を発生する
		bool getPoint(index_t x, index_t y) const{
			if ((m_Size.x <= x) || (m_Size.y <= y)){
				throw;
			}
			return getPointInternal(x, y);
		}

		// 指定した座標の2値データを取得する
		// 範囲外の座標には境界値を返す
		bool getPointClamp(sindex_t x, sindex_t y) const{
			if ((m_Size.x == 0) || (m_Size.y == 0)){
				throw;
			}
			index_t x_ = (x < 0) ? 0 : ((sindex_t)m_Size.x <= x) ? m_Size.x - 1 : x;
			index_t y_ = (y < 0) ? 0 : ((sindex_t)m_Size.y <= y) ? m_Size.y - 1 : y;
			return getPointInternal(x_, y_);
		}

		// 指定した座標の2値データを取得する
		// 正端は原点へループする
		bool getPointRepeat(index_t x, index_t y) const{
			index_t x_ = (m_Size.x == x) ? 0 : x;
			index_t y_ = (m_Size.y == y) ? 0 : y;
			return getPoint(x_, y_);
		}

		// 指定した座標の2値データを書き換える
		void setPoint(index_t x, index_t y, bool value){
			if ((m_Size.x <= x) || (m_Size.y <= y)) throw;
			setPointInternal(x, y, value);
		}
		
		// 指定した座標の2値データを書き換える
		// 正端は原点へループする
		void setPointRepeat(index_t x, index_t y, bool value){
			index_t x_ = (m_Size.x == x) ? 0 : x;
			index_t y_ = (m_Size.y == y) ? 0 : y;
			return setPoint(x_, y_, value);
		}

	private:
		// 指定した座標の2値データを取得する
		bool getPointInternal(index_t x, index_t y) const{
			return m_Data[(size_t)x + (size_t)m_Size.x * y];
		}

		// 指定した座標の2値データを書き換える
		void setPointInternal(index_t x, index_t y, bool value){
			m_Data[(size_t)x + (size_t)m_Size.x * y] = value;
		}
	};
}
