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

		// コピーコンストラクタ
		FFVoxel3D(const FFVoxel3D &obj);

		// コンストラクタ
		// 与えた入力ストリームは削除される
		FFVoxel3D(FFIStream *stream);
		
		// デストラクタ
		~FFVoxel3D();

		// 入力ストリームからボクセルデータを読み込む
		// 与えた入力ストリームは削除される
		void loadFromIStream(FFIStream *stream);

		// ボクセルデータの大きさを取得する
		const index3_t& getSize(void) const{
			return m_Size;
		}

		// スライスの最大の大きさを取得する
		index2_t getMaxSliceSize(void) const{
			return index2_t(m_Size.x, m_Size.y);
		}

		// スライス数を取得する
		index_t getNumOfSlices(void) const{
			return m_Size.z;
		}

		






	private:
		// 代入を禁止する
		FFVoxel3D& operator=(const FFVoxel3D &obj){}

	};
}
