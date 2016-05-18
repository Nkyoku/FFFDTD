// シミュレータ本体

#include "FFSituation.h"
#include "FFSolverCPU.h"
#include "Circuit/FFVoltageSourceComponent.h"

#include <stdio.h>
#include <chrono>
#include <mpi.h>



using namespace FFFDTD;
using namespace glm;



#define MULTI_SOLVER_TEST 1



template<typename T>
static std::vector<T> constspace(T value, int n){
	std::vector<T> output;
	for (int i = 0; i < n; i++){
		output.push_back(value);
	}
	return output;
}

template<typename T>
static std::vector<double> linspace(T start, T end, int n){
	std::vector<T> output;
	if (0 < n){
		output.push_back(start);
		for (int i = 1; i < n; i++){
			output.push_back(start + (end - start) * i / (n - 1));
		}
	}
	return output;
}



// メイン
int main(int argc, char *argv[]){
	// MPIを初期化する
	MPI_Init(&argc, &argv);

	try{
		// 自プロセスのランクを取得する
		int my_rank;
		int total_process;
		MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
		MPI_Comm_size(MPI_COMM_WORLD, &total_process);
		printf("Process Rank    : %d\n", my_rank);
		printf("Total Processes : %d\n", total_process);

#if MULTI_SOLVER_TEST == 0
		// シングルソルバーテスト

		// FFSituationの生成
		FFGrid grid_x = constspace(0.002, 50);
		FFGrid grid_y = constspace(0.002, 50);
		FFGrid grid_z = constspace(0.002, 151);
		BC_t bc = {
			BoundaryCondition::PML,
			BoundaryCondition::PML,
			BoundaryCondition::PML,
			index3_t(5, 5, 5),
			2.0,
			1e-5
		};
		FFSituation situation;
		situation.setGrids(grid_x, grid_y, grid_z, bc);
		situation.setDivision(0, 151);
		situation.createVolumeData();

		index3_t global_size = situation.getGlobalSize();
		printf("Situation1 :\n");
		printf("\tGlobalSize  : %dx%dx%d\n", global_size.x, global_size.y, global_size.z);
		printf("\tLocalSize   : %d\n", situation.getLocalSize());
		printf("\tLocalOffset : %d\n", situation.getLocalOffset());

		situation.initializeMaterialList(0);
		situation.placePECCuboid(index3_t(25, 25, 25), index3_t(25, 25, 126));

		const char diffgauss[] =
			"var tw := 1.27 / 1.5e+9; \n"
			"var tmp := 4 * (t - tw) / tw; \n"
			"sqrt(2 * 2.718281828459045) * tmp * exp(-tmp * tmp);";
		situation.placePort(index3_t(25, 25, 75), Z_PLUS, new FFVoltageSourceComponent(new FFWaveform(diffgauss), 50.0));

		printf("Configuring solver\n");
		auto time_1 = std::chrono::system_clock::now();

		FFSolverCPU *solver = FFSolverCPU::createSolver();
		std::vector<double> freq_list = linspace(75e6, 3e9, 100);
		situation.configureSolver(solver, situation.calcTimestep(), 1000, freq_list);

		auto time_2 = std::chrono::system_clock::now();
		printf("Time elapsed = %dms\n", (int)std::chrono::duration_cast<std::chrono::milliseconds>(time_2 - time_1).count());
		printf("Simulation started\n");

		for (uint32_t cnt = 0; cnt < 1000; cnt++){
			if ((cnt % 100) == 0){
				dvec2 total = solver->calcTotalEM();
				printf("Step%d : E=%e, H=%e\n", cnt, total.x, total.y);
			}
			if ((cnt == 100) || (cnt == 200) || (cnt == 300)){
				char name[256];
				sprintf(name, "tmp/raw%d.txt", (int)cnt);
				FILE *fp = fopen(name, "w");;
				if (fp != NULL){
					index3_t size = solver->getSize();
					index_t y = 25;
					for (index_t z = 0; z <= size.z; z++){
						for (index_t x = 0; x <= size.x; x++){
							real ex = solver->getExDebug(x, y, z);
							real ey = solver->getEyDebug(x, y, z);
							real ez = solver->getEzDebug(x, y, z);
							real eabs = sqrt(ex * ex + ey * ey + ez * ez);
							fprintf(fp, "%d %d %e %e %e %e\n", (int)z, (int)x, ex, ey, ez, eabs);
						}
						fprintf(fp, "\n");
					}
					fclose(fp);
				}
			}
			bool result;
			result = situation.executeSolverStep1();
			if (result == false){
				break;
			}
			situation.executeSolverStep2();
			situation.executeSolverStep3();
			situation.executeSolverStep4();
			situation.executeSolverStep5();
		}

		auto time_3 = std::chrono::system_clock::now();
		printf("Time elapsed = %dms\n", (int)std::chrono::duration_cast<std::chrono::milliseconds>(time_3 - time_2).count());
		printf("Simulation finished");

		auto port_list = situation.getPortList();
		for (size_t i = 0; i < port_list.size(); i++){
			if (port_list[i] == nullptr){
				continue;
			}
			auto circuit = port_list[i]->getCircuit();
			auto &voltage = circuit->getVoltageHistory();
			auto &current = circuit->getCurrentHistory();
			double dt = circuit->dt();

			char fname[256];
			sprintf(fname, "tmp/port%d_td.txt", (int)i);
			FILE *fp = fopen(fname, "w");
			if (fp == NULL) {
				continue;
			}
			for (size_t n = 0; n < voltage.size(); n++){
				fprintf(fp, "%e %e %e\n", dt * n, voltage[n], current[n]);
			}
			fclose(fp);
		}
#else
		// マルチソルバーテスト

		size_t NF = 1000;

		FFSituation situation[2];
		FFSolverCPU *solver[2];

		for (size_t n = 0; n < 2; n++){
			// FFSituationの生成
			FFGrid grid_x = constspace(0.002, 50);
			FFGrid grid_y = constspace(0.002, 50);
			FFGrid grid_z = constspace(0.002, 151);
			BC_t bc = {
				BoundaryCondition::PML,
				BoundaryCondition::PML,
				BoundaryCondition::PML,
				index3_t(5, 5, 5),
				2.0,
				1e-5
			};
			situation[n].setGrids(grid_x, grid_y, grid_z, bc);
			if (n == 0){
				situation[n].setDivision(0, 100);
			}
			else if (n == 1){
				situation[n].setDivision(100, 51);
			}
			situation[n].createVolumeData();

			index3_t global_size = situation[n].getGlobalSize();
			printf("Situation[%d] :\n", (int)n);
			printf("\tGlobalSize  : %dx%dx%d\n", global_size.x, global_size.y, global_size.z);
			printf("\tLocalSize   : %d\n", situation[n].getLocalSize());
			printf("\tLocalOffset : %d\n", situation[n].getLocalOffset());

			situation[n].initializeMaterialList(0);
			situation[n].placePECCuboid(index3_t(25, 25, 25), index3_t(25, 25, 126));

			const char diffgauss[] =
				"var tw := 1.27 / 1.5e+9; \n"
				"var tmp := 4 * (t - tw) / tw; \n"
				"sqrt(2 * 2.718281828459045) * tmp * exp(-tmp * tmp);";
			situation[n].placePort(index3_t(25, 25, 75), Z_PLUS, new FFVoltageSourceComponent(new FFWaveform(diffgauss), 50.0));
		}

		// ソルバーを設定する
		printf("Configuring solver\n");
		auto time_1 = std::chrono::system_clock::now();

		std::vector<double> freq_list = linspace(75e6, 3e9, 100);
		for (size_t n = 0; n < 2; n++){
			solver[n] = FFSolverCPU::createSolver();
			situation[n].configureSolver(solver[n], situation[n].calcTimestep(), NF, freq_list);
		}
		
		// シミュレーションを開始する
		auto time_2 = std::chrono::system_clock::now();
		printf("Time elapsed = %dms\n", (int)std::chrono::duration_cast<std::chrono::milliseconds>(time_2 - time_1).count());
		printf("Simulation started\n");
		
		for (uint32_t cnt = 0; cnt < 1000; cnt++){
			if ((cnt % 100) == 0){
				for (size_t n = 0; n < 2; n++){
					dvec2 total = situation[n].calcTotalEM();
					printf("[%d] Step%d : E=%e, H=%e\n", (int)n, cnt, total.x, total.y);
				}
			}
			if ((cnt == 100) || (cnt == 200) || (cnt == 300)){
				for (size_t n = 0; n < 2; n++){
					char name[256];
					sprintf(name, "tmp/raw%d_%d.txt", (int)n, (int)cnt);
					FILE *fp = fopen(name, "w");;
					if (fp != NULL){
						FFSolverCPU *sol = solver[n];
						index3_t size = sol->getSize();
						index_t y = 25;
						for (index_t z = 0; z <= size.z; z++){
							for (index_t x = 0; x <= size.x; x++){
								real ex = sol->getExDebug(x, y, z);
								real ey = sol->getEyDebug(x, y, z);
								real ez = sol->getEzDebug(x, y, z);
								real eabs = sqrt(ex * ex + ey * ey + ez * ez);
								fprintf(fp, "%d %d %e %e %e %e\n", (int)z, (int)x, ex, ey, ez, eabs);
							}
							fprintf(fp, "\n");
						}
						fclose(fp);
					}
				}
			}
			bool result;
			result = situation[0].executeSolverStep1();
			result &= situation[1].executeSolverStep1();
			if (result == false){
				break;
			}
			situation[0].executeSolverStep2();
			situation[1].executeSolverStep2();
			situation[0].executeSolverStep3(nullptr, &situation[1]);
			situation[1].executeSolverStep3(&situation[0], nullptr);
			situation[0].executeSolverStep4();
			situation[1].executeSolverStep4();
			situation[0].executeSolverStep5(nullptr, &situation[1]);
			situation[1].executeSolverStep5(&situation[0], nullptr);
		}

		// シミュレーションを終了する
		auto time_3 = std::chrono::system_clock::now();
		printf("Time elapsed = %dms\n", (int)std::chrono::duration_cast<std::chrono::milliseconds>(time_3 - time_2).count());
		printf("Simulation finished");

		for (size_t n = 0; n < 2; n++){
			auto port_list = situation[n].getPortList();
			for (size_t i = 0; i < port_list.size(); i++){
				if (port_list[i] == nullptr){
					continue;
				}
				auto circuit = port_list[i]->getCircuit();
				auto &voltage = circuit->getVoltageHistory();
				auto &current = circuit->getCurrentHistory();
				double dt = circuit->dt();

				char fname[256];
				sprintf(fname, "tmp/port%d_td.txt", (int)i);
				FILE *fp = fopen(fname, "w");
				if (fp == NULL) {
					continue;
				}
				for (size_t n = 0; n < voltage.size(); n++){
					fprintf(fp, "%e %e %e\n", dt * n, voltage[n], current[n]);
				}
				fclose(fp);
			}
		}
#endif
	}
	catch (...){
		// 計算を中断する
		MPI_Abort(MPI_COMM_WORLD, 0);
	}

	// MPIを終了する
	MPI_Finalize();

	printf("\nPress any key.\n");
	getchar();
	
	return 0;
}
