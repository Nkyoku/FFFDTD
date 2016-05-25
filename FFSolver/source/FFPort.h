#pragma once

#include "Circuit/FFCircuit.h"



namespace FFFDTD{
	class FFSolver;

	// ポート
	class FFPort{
		/*** メンバー変数 ***/
	private:
		// 割り当てられた回路部品
		FFCircuit *m_Circuit;

		// 割り当てられた時間ドメイン電界プローブ
		oindex_t m_EProbeID;
		double m_EProbeCoef;

		// 割り当てられた時間ドメイン磁界プローブ
		std::vector<oindex_t> m_MProbeIDList;
		std::vector<double> m_MProbeCoefList;



		/*** メソッド ***/
	public:
		// コンストラクタ
		// 指定した回路部品をポートに割り当てる
		FFPort(FFCircuit *circuit);

		// デストラクタ
		~FFPort();

		// メモリーを確保する
		void allocate(size_t size, double timestep);

		// 電界プローブを割り当てる
		void attachEProbe(oindex_t probe_id, double iwidth);

		// 磁界プローブを割り当てる
		void attachMProbe(oindex_t probe_id, double width);

		// 次の出力値を計算する
		void calcValue(FFSolver *solver, size_t n);

		// 回路を取得する
		const FFCircuit* getCircuit(void) const{
			return m_Circuit;
		}
	};
}

