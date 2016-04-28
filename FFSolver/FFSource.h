#pragma once

#include "FFObject.h"
#include "MUFunction.h"



namespace FFFDTD{
	// 信号源の種類
	enum class SourceType{
		Voltage,		// 電圧源
	};

	

	// 信号源の情報を保持する基底クラス
	class MUSourceInfo{
		/*** 定数 ***/
	public:
		// 内部抵抗の初期値
		static const double DEFAULT_RESISTANCE;

		
		
		/*** メンバー変数 ***/
	private:
		// 種類
		SourceType m_SourceType;

		// 出力波形関数
		const MUFunction *m_Function;

		// 内部抵抗
		double m_Resistance;



		/*** メソッド ***/
	public:
		// コンストラクタ
		MUSourceInfo(SourceType type, const MUFunction *function = nullptr)
			: m_SourceType(type), m_Function(function){}
		
		// 信号源の種類を取得する
		SourceType getSourceType(void) const{
			return m_SourceType;
		}

		// 出力波形関数を設定する
		void setFunction(MUFunction *function){
			m_Function = function;
		}

		// 出力波形関数を取得する
		const MUFunction* getFunction(void) const{
			return m_Function;
		}

		// 内部抵抗を設定する
		void setResistance(double impedance){
			m_Resistance = impedance;
		}

		// 内部抵抗を取得する
		double getResistance(void) const{
			return m_Resistance;
		}
	};





	// 信号源を計算する基底クラス
	class DSource : public DPointObject, public DSourceInfo{
		/*** メンバー変数 ***/
	protected:
		// 電界測定プローブID
		int m_ProbeID;





		


		/*** メソッド ***/
	public:
		// コンストラクタ
		DSource(const DPointObject &point, const DSourceInfo &info);









	};



	// 信号源の測定値を保持するクラス
	class DSourceResult : public DPointObject, public DSourceInfo{
		/*** メンバー変数 ***/
	protected:
		// 測定値
		std::vector<dvec2> m_Measurement;

		// タイムステップ
		double m_Timestep;

		// 論理座標
		fvec3 m_LPos;

		// 物理座標
		dvec3 m_PPos;



		/*** メソッド ***/
	public:
		// コンストラクタ
		DSourceResult(const DSource &source);

		// 測定値を格納する


		// タイムステップを設定する
		void setTimestep(double timestep){
			m_Timestep = timestep;
		}

		// タイムステップを取得する
		double getTimestep(void) const{
			return m_Timestep;
		}
		/*
		// 入射電圧の測定値を取得する
		bool getVincList(std::vector<double> &result) const;

		// 入力電圧の測定値を取得する
		bool getVinList(std::vector<double> &result) const;

		// 入射電流の測定値を取得する
		bool getIincList(std::vector<double> &result) const;

		// 入力電流の測定値を取得する
		bool getIinList(std::vector<double> &result) const;
		*/


		/*
		// 入力インピーダンスの周波数特性を計算する
		void calcImpedance(const std::vector<double> &freq_list, std::vector<complex> &zin_data) const;

		// Sパラメータの周波数特性を計算する
		void calcSParameter(const DPort *recv_port, const std::vector<double> &freq_list, std::vector<complex> &spara_data) const;

	private:
		// 離散フーリエ変換を計算する
		void DPort::calcDFT(const std::vector<double> &data_list, const std::vector<double> &freq_list, std::vector<complex> &result, FrequencyResponce_e fr_type = FR_DFT) const;
		*/
	};
}
