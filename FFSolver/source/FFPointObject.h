#pragma once

#include "FFType.h"



namespace FFFDTD{
	// 点状オブジェクトの基底クラス
	class FFPointObject{
		/*** メンバー変数 ***/
	private:
		// 座標
		index3_t m_Pos;

		// 向き
		DIR_e m_Dir;



		/*** メソッド ***/
	public:
		// コンストラクタ
		FFPointObject(void) : m_Pos(0, 0, 0), m_Dir(X_PLUS){}

		// コンストラクタ
		FFPointObject(const index3_t &pos, DIR_e dir) : m_Pos(pos), m_Dir(dir){}

		// デストラクタ
		virtual ~FFPointObject(){}

		// 座標を設定する
		void setPos(const index3_t &pos){
			m_Pos = pos;
		}

		// 向きを設定する
		void setDir(DIR_e dir){
			m_Dir = dir;
		}

		// 座標を取得する
		const index3_t& getPos(void) const{
			return m_Pos;
		}

		// 向きを取得する
		DIR_e getDir(void) const{
			return m_Dir;
		}
	};
}

