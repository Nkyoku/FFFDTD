#pragma once

#include "FFPointObject.h"



namespace FFFDTD{
	// 電圧と電流の観測点
	class FFProbePoint : public FFPointObject{
		/*** メンバー変数 ***/
	private:
		



		/*** メソッド ***/
	public:
		// コンストラクタ
		FFProbePoint(void){}

		// コンストラクタ
		FFProbePoint(const FFPointObject &object) : FFPointObject(object){}

		// デストラクタ
		~FFProbePoint(){}

		
	};
}

