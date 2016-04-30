#include "FFPort.h"



namespace FFFDTD{
	// コンストラクタ
	// 指定した回路部品をポートに割り当てる
	FFPort::FFPort(FFCircuit *circuit)
		: m_ProbePointID(~0), m_Circuit(circuit)
	{
	
	}

	// デストラクタ
	FFPort::~FFPort(){
		delete m_Circuit;
	}

	// 観測点をアタッチする
	void FFPort::attachProbePoint(oindex_t probe_point){
		m_ProbePointID = probe_point;
	}



	
	
}

