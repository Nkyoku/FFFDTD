#include "FFPort.h"
#include "FFSolver.h"



namespace FFFDTD{
	// コンストラクタ
	// 指定した回路部品をポートに割り当てる
	FFPort::FFPort(FFCircuit *circuit)
		: m_Circuit(circuit)
		, m_EProbeID(~0), m_EProbeCoef(0.0)
	{
	
	}

	// デストラクタ
	FFPort::~FFPort(){
		delete m_Circuit;
	}

	// 電界プローブを割り当てる
	void FFPort::attachEProbe(oindex_t probe_id, double iwidth){
		
	}

	// 磁界プローブを割り当てる
	void FFPort::attachMProbe(oindex_t probe_id, double width){

	}

	// 次の出力値を計算する
	void FFPort::calcValue(FFSolver *solver, size_t n){
		// 磁界の観測値から電流を計算する
		double current = 0.0;
		for (size_t i = 0; i < m_MProbeIDList.size(); i++){
			current += solver->getProbeValue(m_MProbeIDList[i], n, ProbeType::TD) * m_MProbeCoefList[i];
		}

		// 電圧を計算し、電界を励振する
		double voltage = m_Circuit->calcVoltage(n, current);
		solver->setTDProbeValue(m_EProbeID, (real)(-voltage * m_EProbeCoef));
	}
	
}

