#pragma once

#include "FFType.h"



namespace FFFDTD{
	// 回路部品の抽象クラス
	class FFCircuit{
	private:
		// 端子電圧の履歴
		std::vector<double> m_VoltageHistory;

		// 端子電流の履歴
		std::vector<double> m_CurrentHistory;

		// タイムステップ
		double m_Timestep;

	public:
		// コンストラクタ
		FFCircuit(void){}

		// デストラクタ
		virtual ~FFCircuit() = 0;

		// メモリーを確保する
		virtual void allocate(size_t size, double timestep){
			m_VoltageHistory.resize(size, 0.0);
			m_CurrentHistory.resize(size, 0.0);
			m_Timestep = timestep;
		}

		// 端子電圧V[n]を格納する
		void storeVoltage(size_t n, double voltage){
			m_VoltageHistory[n] = voltage;
		}

		// 端子電流I[n-1/2]を格納する
		void storeCurrent(size_t n, double current){
			m_CurrentHistory[n] = current;
		}

		// 端子電圧V[n]を計算する
		virtual double calcVoltage(size_t n) = 0;

		// 端子電圧の履歴を取得する
		const std::vector<double>& getVoltageHistory(void) const{
			return m_VoltageHistory;
		}

		// 端子電流の履歴を取得する
		const std::vector<double>& getCurrentHistory(void) const{
			return m_CurrentHistory;
		}

	protected:
		// 端子電圧V[n-m]を取得する
		double voltage(size_t n, size_t m) const{
			return (m <= n) ? m_VoltageHistory[n - m] : 0.0;
		}

		// 端子電流I[n-m-1/2]を取得する
		double current(size_t n, size_t m) const{
			return (m <= n) ? m_CurrentHistory[n - m] : 0.0;
		}

		// タイムステップを取得する
		double dt(void) const{
			return m_Timestep;
		}
	};
}
