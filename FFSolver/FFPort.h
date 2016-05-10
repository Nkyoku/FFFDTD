#pragma once

#include "FFProbePoint.h"
#include "Circuit/FFCircuit.h"



namespace FFFDTD{
	// ポート
	class FFPort{
		/*** メンバー変数 ***/
	private:
		// 割り当てられた観測点ID
		oindex_t m_ProbePointID;

		// 割り当てられた回路部品
		FFCircuit *m_Circuit;



		/*** メソッド ***/
	public:
		// コンストラクタ
		// 指定した回路部品をポートに割り当てる
		FFPort(FFCircuit *circuit);

		// デストラクタ
		~FFPort();

		// 観測点を割り当てる
		void attachProbePoint(oindex_t probe_point);



		
	};
}

