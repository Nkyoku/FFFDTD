#pragma once

#include "FFType.h"
#include "FFConst.h"



namespace FFFDTD{
	// 媒質の物性情報を保持するクラス
	class FFMaterial{
		/*** メンバー変数 ***/
	private:
		// 比誘電率ε_r
		double m_eps_r;

		// 導電率σ[S/m]
		double m_sigma;

		// 比透磁率μ_r
		double m_mu_r;



		/*** メソッド ***/
	public:
		// コンストラクタ
		FFMaterial(double eps_r = 1.0, double sigma = 0.0, double mu_r = 1.0)
			: m_eps_r(eps_r), m_sigma(sigma), m_mu_r(mu_r){}

		bool operator==(const FFMaterial &mat){
			return (m_eps_r == mat.m_eps_r) && (m_sigma == mat.m_sigma) && (m_mu_r == mat.m_mu_r);
		}

		// 比誘電率を取得する
		double eps_r(void) const{
			return m_eps_r;
		}

		// 導電率を取得する
		double sigma(void) const{
			return m_sigma;
		}

		// 比透磁率を取得する
		double mu_r(void) const{
			return m_mu_r;
		}

		// 誘電率を取得する
		double eps(void) const{
			return m_eps_r * EPS_0;
		}

		// 透磁率を取得する
		double mu(void) const{
			return m_mu_r * MU_0;
		}

		// 電界の係数を計算する
		rvec3 calcECoef(double dt, double dl1, double dl2) const{
			return calcECoef(m_eps_r, m_sigma, dt, dl1, dl2);
		}

		// PML中の電界の係数を計算する
		rvec2 calcECoefPML(double dt) const{
			return calcECoefPML(m_eps_r, m_sigma, dt);
		}

		// PML中の電束密度の係数を計算する
		rvec2 calcDCoefPML(double dt, double dl) const{
			return calcDCoefPML(m_eps_r, 0.0, dt, dl);
		}

		// 磁界の係数を計算する
		rvec3 calcHCoef(double dt, double dl1, double dl2) const{
			return calcHCoef(m_mu_r, 0.0, dt, dl1, dl2);
		}

		// PML中の磁界の係数を計算する
		rvec2 calcHCoefPML(double dt, double dl) const{
			return calcHCoefPML(m_mu_r, 0.0, dt, dl);
		}

	public:
		// 真空の媒質を取得する
		static FFMaterial vacuum(void){
			return FFMaterial();
		}

		// 電界の係数を計算する
		static rvec3 calcECoef(double eps_r, double sigma, double dt, double dl1, double dl2){
			double p1 = 2.0 * EPS_0 * eps_r;
			double p2 = sigma * dt;
			double r12 = 1.0 / (p1 + p2);
			double s1 = 2.0 * dt * r12;
			return rvec3((real)((p1 - p2) * r12), (real)(s1 / dl1), (real)(s1 / dl2));
		}

		// PML中の電界の係数を計算する
		static rvec2 calcECoefPML(double eps_r, double sigma, double dt){
			double p1 = 2.0 * EPS_0 * eps_r;
			double p2 = sigma * dt;
			double r12 = 1.0 / (p1 + p2);
			return rvec2((real)((p1 - p2) * r12), (real)(2.0 * r12));
		}

		// PML中の電束密度の係数を計算する
		static rvec2 calcDCoefPML(double eps_r, double pml_sigma, double dt, double dl){
			double p1 = 2.0 * EPS_0 * eps_r;
			double p2 = pml_sigma * dt;
			double r12 = 1.0 / (p1 + p2);
			return rvec2((real)((p1 - p2) * r12), (real)(p1 * dt * r12 / dl));
		}

		// 磁界の係数を計算する
		static rvec3 calcHCoef(double mu_r, double sigma_m, double dt, double dl1, double dl2){
			double p1 = 2.0 * MU_0 * mu_r;
			double p2 = sigma_m * dt;
			double r12 = 1.0 / (p1 + p2);
			double s1 = 2.0 * dt * r12;
			return rvec3((real)((p1 - p2) * r12), (real)(s1 / dl1), (real)(s1 / dl2));
		}

		// PML中の磁界の係数を計算する
		static rvec2 calcHCoefPML(double mu_r, double pml_sigma_m, double dt, double dl){
			double p1 = 2.0 * MU_0 * mu_r;
			double p2 = pml_sigma_m * dt;
			double r12 = 1.0 / (p1 + p2);
			return rvec2((real)((p1 - p2) * r12), (real)(2.0 * dt * r12 / dl));
		}
	};
}

