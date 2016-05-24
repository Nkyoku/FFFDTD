// シミュレータ本体

#include "FFSituation.h"
#include "FFSolverCPU.h"
#include "Circuit/FFVoltageSourceComponent.h"

#include <stdio.h>
#include <chrono>
#include <mpi.h>
#include <stddef.h>



using namespace FFFDTD;
using namespace glm;



#define MULTI_SOLVER_TEST 2



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



static const int ROOT_RANK = 0;



class SOLVERINFO_t{
private:
	// ソルバー名
	char m_Name[64];

	// ソルバーを持つプロセスのランク
	int m_Rank;

	// ランク内でのソルバー番号
	uint32_t m_Index;

	// 処理速度(Cell/sec)
	uint64_t m_Speed;
	
	// メモリー容量(Byte)
	uint64_t m_Memory;

public:
	// コンストラクタ
	SOLVERINFO_t(void)
		: m_Name(), m_Rank(-1), m_Speed(0), m_Memory(0)
	{
		strcpy(m_Name, "Unknown");
	}

	// コンストラクタ
	SOLVERINFO_t(const char *name, int rank, uint32_t index, uint64_t speed, uint64_t memory)
		: m_Name(), m_Rank(rank), m_Index(index), m_Speed(speed), m_Memory(memory)
	{
		strncpy(m_Name, name, sizeof(m_Name) - 1);
		m_Name[sizeof(m_Name) - 1] = '\0';
	}

	// ソルバー名を取得する
	const char* getName(void) const{
		return m_Name;
	}

	// プロセスのランクを取得する
	int getRank(void) const{
		return m_Rank;
	}

	// ソルバー番号を取得する
	uint32_t getIndex(void) const{
		return m_Index;
	}

	// 処理速度を取得する
	uint64_t getSpeed(void) const{
		return m_Speed;
	}

	// メモリー容量を取得する
	uint64_t getMemory(void) const{
		return m_Memory;
	}

	// この構造体のMPIデータタイプ
	static MPI_Datatype m_MPIDataType;

	// MPIデータタイプを登録する
	static void registerType(void){
		int blocklength[5] = {sizeof(m_Name), 1, 1, 1, 1};
		MPI_Aint displacement[5] = {offsetof(SOLVERINFO_t, m_Name), offsetof(SOLVERINFO_t, m_Rank), offsetof(SOLVERINFO_t, m_Index), offsetof(SOLVERINFO_t, m_Speed), offsetof(SOLVERINFO_t, m_Memory)};
		MPI_Datatype type[5] = {MPI_CHAR, MPI_INT, MPI_UINT32_T, MPI_UINT64_T, MPI_UINT64_T};
		MPI_Type_create_struct(5, blocklength, displacement, type, &m_MPIDataType);
		MPI_Type_commit(&m_MPIDataType);
	}

	// データタイプを取得する
	static MPI_Datatype getDataType(void){
		return m_MPIDataType;
	}
};

// この構造体のMPIデータタイプ
MPI_Datatype SOLVERINFO_t::m_MPIDataType;



// メイン
int main(int argc, char *argv[]){
	// MPIを初期化する
	MPI_Init(&argc, &argv);

	// 自プロセスのランクを取得する
	int my_rank;
	int total_process;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &total_process);
	if (my_rank == ROOT_RANK){
		//printf("Process Rank    : %d\n", my_rank);
		printf("Total Processes : %d\n", total_process);
	}

	try{
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
#elif MULTI_SOLVER_TEST == 1
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
#elif MULTI_SOLVER_TEST == 2
		// マルチプロセステスト
		
		std::vector<FFSolver*> solver_list;
		std::vector<FFSituation> situation_list;

		// ソルバーを作成する
		solver_list.push_back(FFSolverCPU::createSolver());
		solver_list.push_back(FFSolverCPU::createSolver());
		/*for (size_t i = 0; i < solver_list.size(); i++){
			printf("Solver[%d] : %s\n", (int)i, solver_list[0]->getSolverName().c_str());
		}*/
		
		situation_list.resize(solver_list.size());
		uint32_t num_of_situations = (uint32_t)situation_list.size();

		// 全てのソルバー情報を全プロセスで共有する
		SOLVERINFO_t::registerType();
		std::vector<SOLVERINFO_t> solverinfo_list;
		std::vector<SOLVERINFO_t> whole_solverinfo_list;
		for(size_t i = 0; i < solver_list.size(); i++){
			FFSolver *solver = solver_list[i];
			solverinfo_list.push_back(SOLVERINFO_t(solver->getSolverName().c_str(), my_rank, (uint32_t)i, 123 + 4 * my_rank, 456 + my_rank));
		}
		{
			// 全体のソルバー数を取得する
			uint32_t num_of_solvers = (uint32_t)solver_list.size();
			std::vector<int> solver_count(total_process);
			MPI_Allgather(&num_of_solvers, 1, MPI_UINT32_T, solver_count.data(), 1, MPI_UINT32_T, MPI_COMM_WORLD);

			// 全てのソルバー情報を集める
			size_t total_solvers = 0;
			std::vector<int> disp_list(total_process);
			for (int p = 0; p < total_process; p++){
				disp_list[p] = (int)total_solvers;
				total_solvers += solver_count[p];
			}
			whole_solverinfo_list.resize(total_solvers);
			MPI_Allgatherv(solverinfo_list.data(), (int)solverinfo_list.size(), SOLVERINFO_t::getDataType(), whole_solverinfo_list.data(), solver_count.data(), disp_list.data(), SOLVERINFO_t::getDataType(), MPI_COMM_WORLD);
		}
		if (my_rank == ROOT_RANK){
			// 全てのソルバー情報を出力する
			printf("Solvers :\n");
			for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
				auto &it = whole_solverinfo_list[i];
				printf("  [%d] Rank[%d] Solver[%d] : %llu c/s, %llu bytes, '%s'\n", (int)i, it.getRank(), it.getIndex(), it.getSpeed(), it.getMemory(), it.getName());
			}
		}
		fflush(stdout);

		// シミュレーション環境の大きさを入力する
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
		if (my_rank == ROOT_RANK){
			// シミュレーション空間サイズを出力する
			printf("Situation :\n");
			printf("  GlobalSize  : %dx%dx%d\n", grid_x.count(), grid_y.count(), grid_z.count());
		}
		fflush(stdout);

		// 各ソルバーへの問題の分割の仕方を決定する
		std::vector<index_t> division(whole_solverinfo_list.size());
		{
			// 処理速度の合計を求める
			uint64_t total_cps = 0;
			for (auto &it : whole_solverinfo_list){
				total_cps += it.getSpeed();
			}
			index_t num_of_slices = grid_z.count();

			// 処理速度の比率に従って処理スライス数を割り当てる
			index_t cell_per_slice = grid_x.count() * grid_y.count();
			index_t num_of_division = 0;
			for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
				uint64_t cps = whole_solverinfo_list[i].getSpeed();
				division[i] = (index_t)((num_of_slices * cps + total_cps / 2) / total_cps);
				num_of_division += division[i];
			}
			while (num_of_division != num_of_slices){
				for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
					if (num_of_slices < num_of_division){
						if (0 < division[i]){
							division[i]--;
							num_of_division--;
						}
					}
					else if (num_of_division < num_of_slices){
						division[i]++;
						num_of_division++;
					}
					if (num_of_division == num_of_slices){
						break;
					}
				}
			}

			// メモリー容量に従って処理スライス数を調整する
			//
			// To Do
			//
		}
		if (my_rank == ROOT_RANK){
			// 処理スライスの割り当てを出力する
			printf("Divisions :\n");
			index_t start = 0;
			for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
				if (0 < division[i]){
					auto &it = whole_solverinfo_list[i];
					printf("  Division[%d-%d] -> Rank[%d] Solver[%d]\n", start, start + division[i] - 1, it.getRank(), it.getIndex());
					start += division[i];
				}
			}
		}

		// シミュレーション空間を作成する
		index_t division_offset = 0;
		for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
			auto &solverinfo = whole_solverinfo_list[i];
			if (solverinfo.getRank() == my_rank){
				uint32_t index = solverinfo.getIndex();
				FFSituation &situation = situation_list[index];
				situation.setGrids(grid_x, grid_y, grid_z, bc);
				situation.setDivision(division_offset, division[i]);
				situation.createVolumeData();
				situation.initializeMaterialList(0);
			}
			division_offset += division[i];
		}
		
		// シミュレーション空間にオブジェクトを配置する
		for (uint32_t i = 0; i < num_of_situations; i++){
			FFSituation &situation = situation_list[i];
			situation.placePECCuboid(index3_t(25, 25, 25), index3_t(25, 25, 126));
			static const char diffgauss[] =
				"var tw := 1.27 / 1.5e+9; \n"
				"var tmp := 4 * (t - tw) / tw; \n"
				"sqrt(2 * 2.718281828459045) * tmp * exp(-tmp * tmp);";
			situation.placePort(index3_t(25, 25, 75), Z_PLUS, new FFVoltageSourceComponent(new FFWaveform(diffgauss), 50.0));
		}
		
		// ソルバーを構成する
		if (my_rank == ROOT_RANK){
			printf("Configuring solver\n");
		}
		fflush(stdout);
		size_t NT = 1000;
		double timestep = 0.0;
		std::vector<double> freq_list = linspace(75e6, 3e9, 100);
		for (uint32_t i = 0; i < num_of_situations; i++){
			FFSituation &situation = situation_list[i];
			if (i == 0){
				timestep = situation.calcTimestep();
			}
			situation.configureSolver(solver_list[i], timestep, NT, freq_list);
		}

		// ソルバーに接続している別のソルバーを取得
		std::vector<FFSituation*> bottom_situation(num_of_situations, nullptr), top_situation(num_of_situations, nullptr);
		std::vector<int> bottom_rank(num_of_situations, -1), top_rank(num_of_situations, -1);
		for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
			auto &solverinfo = whole_solverinfo_list[i];
			if (solverinfo.getRank() == my_rank){
				uint32_t index = solverinfo.getIndex();
				if (0 < i){
					// 下に別のソルバーが接続される
					auto &bottom_solverinfo = whole_solverinfo_list[i - 1];
					if (bottom_solverinfo.getRank() == my_rank){
						bottom_situation[index] = &situation_list[bottom_solverinfo.getIndex()];
						printf("  [%d] Rank[%d] Bottom section is FFSituation[%d]\n", (int)i, my_rank, bottom_solverinfo.getIndex());
					}
					else{
						bottom_rank[index] = bottom_solverinfo.getRank();
						printf("  [%d] Rank[%d] Bottom section is Rank[%d]\n", (int)i, my_rank, bottom_solverinfo.getRank());
					}
				}
				if (i < (whole_solverinfo_list.size() - 1)){
					// 上に別のソルバーが接続される
					auto &top_solverinfo = whole_solverinfo_list[i + 1];
					if (top_solverinfo.getRank() == my_rank){
						top_situation[index] = &situation_list[top_solverinfo.getIndex()];
						printf("  [%d] Rank[%d] Top section is FFSituation[%d]\n", (int)i, my_rank, top_solverinfo.getIndex());
					}
					else{
						top_rank[index] = top_solverinfo.getRank();
						printf("  [%d] Rank[%d] Top section is Rank[%d]\n", (int)i, my_rank, top_solverinfo.getRank());
					}
				}
			}
		}
		
		// シミュレーションを行う
		MPI_Barrier(MPI_COMM_WORLD);
		if (my_rank == ROOT_RANK){
			printf("Simulation started\n");
		}
		fflush(stdout);
		for (size_t it = 0; it < NT; it++){
			if ((it % 100) == 0){
				for (uint32_t i = 0; i < num_of_situations; i++){
					dvec2 total = situation_list[i].calcTotalEM();
					printf("  Rank[%d] Index[%d] Step%d : E=%e, H=%e\n", my_rank, i, (int)it, total.x, total.y);
				}
				fflush(stdout);
			}
			bool result = true;
			for (uint32_t i = 0; i < num_of_situations; i++){
				result &= situation_list[i].executeSolverStep1();
			}
			if (result == false){
				// 計算終了
				break;
			}
			for (uint32_t i = 0; i < num_of_situations; i++){
				situation_list[i].executeSolverStep2();
			}
			for (uint32_t i = 0; i < num_of_situations; i++){
				situation_list[i].executeSolverStep3(bottom_situation[i], top_situation[i], bottom_rank[i], top_rank[i]);
			}
			for (uint32_t i = 0; i < num_of_situations; i++){
				situation_list[i].executeSolverStep4();
			}
			for (uint32_t i = 0; i < num_of_situations; i++){
				situation_list[i].executeSolverStep5(bottom_situation[i], top_situation[i], bottom_rank[i], top_rank[i]);
			}
		}

		// シミュレーションを終了する
		MPI_Barrier(MPI_COMM_WORLD);
		if (my_rank == ROOT_RANK){
			printf("Simulation finished\n");
		}
		fflush(stdout);
		for (uint32_t i = 0; i < num_of_situations; i++){
			FFSituation &situation = situation_list[i];
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
		}
#endif
	}
	catch (...){
		// 計算を中断する
		MPI_Abort(MPI_COMM_WORLD, 0);
	}

	// MPIを終了する
	MPI_Finalize();

	if (my_rank == ROOT_RANK){
		printf("\nPress any key.\n");
		fflush(stdout);
		getchar();
	}
	
	return 0;
}
