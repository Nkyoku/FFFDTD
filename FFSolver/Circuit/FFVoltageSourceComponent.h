#pragma once

#include "FFCircuit.h"
#include "FFWaveform.h"



namespace FFFDTD{
	// 電圧源の回路部品クラス
	class FFVoltageSourceComponent : public FFCircuit{
	private:
		// 出力電圧波形
		FFWaveform *m_Waveform;

		// 内部直列抵抗
		double m_ESR;

	public:
		// コンストラクタ
		FFVoltageSourceComponent(FFWaveform *waveform, double esr = 0.0)
			: m_Waveform(waveform), m_ESR(esr)
		{

		}

		// デストラクタ
		~FFVoltageSourceComponent(){
			delete m_Waveform;
		}

		// 端子電圧V[n]を計算する
		double calcVoltage(size_t n, double current) override{
			double vinc = m_Waveform->getValue(dt() * n);
			double voltage = vinc - m_ESR * current;
			storeValues(n, voltage, current);
			return voltage;
		}
	};
}
