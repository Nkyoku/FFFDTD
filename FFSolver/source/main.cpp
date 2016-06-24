// シミュレータ本体

#include <stdio.h>
#include <chrono>
#include <array>
#include <mpi.h>
#include <stddef.h>

#include "FFSituation.h"
#include "FFSolverCPU.h"
#include "Circuit/FFVoltageSourceComponent.h"
#include "Basic/FFException.h"

#include "cmdline.h"
#include "solver_setting.h"
#include "parser.h"

 

using namespace FFFDTD;
using namespace glm;



#include "main.h"



// ホスト名の最大長
static const size_t HOSTNAME_LENGTH = 256;

// ルートランク
static const int ROOT_RANK = 0;



// 自プロセスのランク
static int g_mpi_my_rank;

// プロセス数
static int g_mpi_total_process;



// ユーザーのキー入力を待つ
static void waitForPressingAnyKey(void){
	if (g_mpi_my_rank == ROOT_RANK){
		printf("\nPress any key.\n");
		fflush(stdout);
		getchar();
	}
}

// 全プロセスでソルバーを作成し共有する
static void createSolversAndGather(const char *solver_setting_filepath, std::vector<FFSolver*> &solver_list, std::vector<SOLVERINFO_t> &whole_solverinfo_list, std::vector<std::string> &hostname_list){
	// FFSolverを作成する
	char hostname[HOSTNAME_LENGTH];
	SolverSetting::getHostname(hostname, sizeof(hostname));
	std::vector<uint64_t> speed_list;
	SolverSetting::createSolvers(solver_setting_filepath, hostname, &solver_list, &speed_list);
	uint32_t num_of_solvers = (uint32_t)solver_list.size();
	if ((g_mpi_my_rank == ROOT_RANK) && (num_of_solvers == 0)){
		// ルートランクは処理の都合上、必ず1つはソルバーを持たなくてはならないため、処理能力0のCPUソルバーを作成する
		solver_list.push_back(FFSolverCPU::createSolver(1));
		speed_list.push_back(0);
	}

	// ホスト名を集める
	std::vector<std::array<char, HOSTNAME_LENGTH>> hostname_list_c;
	if (g_mpi_my_rank == ROOT_RANK){
		hostname_list_c.resize(g_mpi_total_process);
	}
	MPI_Gather(hostname, (int)HOSTNAME_LENGTH, MPI_CHAR, hostname_list_c.data(), (int)HOSTNAME_LENGTH, MPI_CHAR, ROOT_RANK, MPI_COMM_WORLD);
	if (g_mpi_my_rank == ROOT_RANK){
		hostname_list.resize(hostname_list_c.size());
		for (int i = 0; i < g_mpi_total_process; i++){
			hostname_list[i] = hostname_list_c[i].data();
		}
	}

	// 全てのソルバー情報を全プロセスで共有する
	SOLVERINFO_t::registerType();
	std::vector<SOLVERINFO_t> solverinfo_list;
	for (size_t i = 0; i < solver_list.size(); i++){
		FFSolver *solver = solver_list[i];
		solverinfo_list.push_back(SOLVERINFO_t(solver->getName().c_str(), g_mpi_my_rank, (uint32_t)i, speed_list[i], solver->getMemoryCapacity()));
	}
	
	// 全体のソルバー数を取得する
	int num_of_solvers_i = (int)num_of_solvers;
	std::vector<int> solver_count(g_mpi_total_process);
	MPI_Allgather(&num_of_solvers_i, 1, MPI_UINT32_T, solver_count.data(), 1, MPI_UINT32_T, MPI_COMM_WORLD);

	// 全てのソルバー情報を集める
	size_t total_solvers = 0;
	std::vector<int> disp_list(g_mpi_total_process);
	for (int p = 0; p < g_mpi_total_process; p++){
		disp_list[p] = (int)total_solvers;
		total_solvers += solver_count[p];
	}
	whole_solverinfo_list.resize(total_solvers);
	MPI_Allgatherv(solverinfo_list.data(), (int)solverinfo_list.size(), SOLVERINFO_t::getDataType(), whole_solverinfo_list.data(), solver_count.data(), disp_list.data(), SOLVERINFO_t::getDataType(), MPI_COMM_WORLD);
}

// 入力ファイルを読み込む
static void loadInputFile(const char *input_filepath, std::vector<uint8_t> &input_data, mpack_tree_t &mp_tree){
	// ファイルを読み込んで共有する
	if (g_mpi_my_rank == ROOT_RANK){
		// 入力ファイルを開く
		FILE *fp;
		fp = fopen(input_filepath, "rb");
		if (fp == NULL){
			throw FFException("Failed to open the input file");
		}

		// ファイルサイズをチェックする
		fseek(fp, 0, SEEK_END);
		size_t size = ftell(fp);
		if (INT_MAX < size){
			throw FFException("The input file is too large (maximum 2GB is allowed)");
		}

		// 内容をすべて読み込む
		input_data.resize(size);
		fseek(fp, 0, SEEK_SET);
		fread(input_data.data(), input_data.size(), 1, fp);
		fclose(fp);

		// ファイルサイズを共有する
		int size_int = (int)size;
		MPI_Bcast(&size_int, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
	}
	else{
		// ファイルサイズを受信する
		int size_int;
		MPI_Bcast(&size_int, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
		input_data.resize(size_int);
	}
	// ファイルの内容を共有する
	MPI_Bcast(input_data.data(), (int)input_data.size(), MPI_UINT8_T, ROOT_RANK, MPI_COMM_WORLD);

	// 入力データをパースする
	mpack_tree_init(&mp_tree, (const char*)input_data.data(), input_data.size());
	if (mpack_tree_error(&mp_tree) == mpack_error_io){
		throw FFException("Failed to open the input file");
	}
	else if (mpack_tree_error(&mp_tree) != mpack_ok){
		throw FFException("The input file is corrupted");
	}
}

// 計算能力で処理を割り振る
static void assignDivision(const index3_t &size, const std::vector<SOLVERINFO_t> &whole_solverinfo_list, std::vector<index_t> &whole_division_list, std::vector<FFSituation> &situation_list){
	whole_division_list.resize(whole_solverinfo_list.size());

	// 処理速度の合計を求める
	uint64_t total_cps = 0;
	for (auto &it : whole_solverinfo_list){
		total_cps += it.getSpeed();
	}
	index_t num_of_slices = size.z;

	// 処理速度の比率に従って処理スライス数を割り当てる
	index_t cell_per_slice = size.x * size.y;
	index_t num_of_division = 0;
	for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
		uint64_t cps = whole_solverinfo_list[i].getSpeed();
		whole_division_list[i] = (index_t)((num_of_slices * cps + total_cps / 2) / total_cps);
		num_of_division += whole_division_list[i];
	}
	while (num_of_division != num_of_slices){
		for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
			if (num_of_slices < num_of_division){
				if (0 < whole_division_list[i]){
					whole_division_list[i]--;
					num_of_division--;
				}
			}
			else if ((num_of_division < num_of_slices) && (0 < whole_solverinfo_list[i].getSpeed())){
				whole_division_list[i]++;
				num_of_division++;
			}
			else if (num_of_division == num_of_slices){
				break;
			}
		}
	}

	// メモリー容量に従って処理スライス数を調整する
	//
	// To Do
	//

	// シミュレーション空間の割り振りを決定する
	index_t division_offset = 0;
	for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
		auto &solverinfo = whole_solverinfo_list[i];
		if (solverinfo.getRank() == g_mpi_my_rank){
			uint32_t index = solverinfo.getIndex();
			situation_list[index].setDivision(division_offset, whole_division_list[i]);
			situation_list[index].createVolumeData();
		}
		division_offset += whole_division_list[i];
	}
}

// ソルバーの接続情報を取得する
static void getSolverConnection(std::vector<FFSituation> &situation_list, const std::vector<SOLVERINFO_t> &whole_solverinfo_list, std::vector<index_t> &whole_division_list, std::vector<FFSituation*> &bottom_situation, std::vector<FFSituation*> &top_situation, std::vector<int> &bottom_rank, std::vector<int> &top_rank){
	size_t num_of_situations = situation_list.size();
	bottom_situation.resize(num_of_situations, nullptr);
	top_situation.resize(num_of_situations, nullptr);
	bottom_rank.resize(num_of_situations, -1);
	top_rank.resize(num_of_situations, -1);
	
	for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
		auto &solverinfo = whole_solverinfo_list[i];
		if ((solverinfo.getRank() == g_mpi_my_rank) && (0 < whole_division_list[i])){
			uint32_t index = solverinfo.getIndex();
			
			// 下に別のソルバーが接続されているか調べる
			for (size_t j = i; 0 < j--;){
				if (0 < whole_division_list[j]){
					auto &bottom_solverinfo = whole_solverinfo_list[j];
					if (bottom_solverinfo.getRank() == g_mpi_my_rank){
						bottom_situation[index] = &situation_list[bottom_solverinfo.getIndex()];
						//printf("  [%d] Rank[%d] Bottom section is FFSituation[%d]\n", (int)i, g_mpi_my_rank, bottom_solverinfo.getIndex());
					}
					else{
						bottom_rank[index] = bottom_solverinfo.getRank();
						//printf("  [%d] Rank[%d] Bottom section is Rank[%d]\n", (int)i, g_mpi_my_rank, bottom_solverinfo.getRank());
					}
					break;
				}
			}

			// 上に別のソルバーが接続されているか調べる
			for (size_t j = i + 1; j < whole_solverinfo_list.size(); j++){
				if (0 < whole_division_list[j]){
					auto &top_solverinfo = whole_solverinfo_list[j];
					if (top_solverinfo.getRank() == g_mpi_my_rank){
						top_situation[index] = &situation_list[top_solverinfo.getIndex()];
						//printf("  [%d] Rank[%d] Top section is FFSituation[%d]\n", (int)i, g_mpi_my_rank, top_solverinfo.getIndex());
					}
					else{
						top_rank[index] = top_solverinfo.getRank();
						//printf("  [%d] Rank[%d] Top section is Rank[%d]\n", (int)i, g_mpi_my_rank, top_solverinfo.getRank());
					}
					break;
				}
			}
		}
	}
	//fflush(stdout);
}




// メイン
int main(int argc, char *argv[]){
	// 自プロセスのソルバーへのポインタのリスト
	std::vector<FFSolver*> solver_list;

	// MPIを初期化する
	int mpi_multithread_level;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_multithread_level);

	// 自プロセスのランクを取得する
	MPI_Comm_rank(MPI_COMM_WORLD, &g_mpi_my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &g_mpi_total_process);
	
	if (g_mpi_my_rank == ROOT_RANK){
		// MPIのマルチスレッド対応レベルを表示する
		const char *mt_string;
		switch (mpi_multithread_level){
		case MPI_THREAD_SINGLE:
			mt_string = "MPI_THREAD_SINGLE";
			break;
		case MPI_THREAD_FUNNELED:
			mt_string = "MPI_THREAD_FUNNELED";
			break;
		case MPI_THREAD_SERIALIZED:
			mt_string = "MPI_THREAD_SERIALIZED";
			break;
		case MPI_THREAD_MULTIPLE:
			mt_string = "MPI_THREAD_MULTIPLE";
			break;
		default:
			mt_string = "Unknown";
			break;
		}
		printf("MPI Multithread : %s\n", mt_string);
		printf("Total Processes : %d\n", g_mpi_total_process);
	}

	try{
		Cmdline cmdline;
		if (g_mpi_my_rank == ROOT_RANK){
			// コマンドラインオプションをパースする
			if (cmdline.parse(argc - 1, argv + 1) == false){
				throw;
			}
		}

		// 全プロセスでソルバーを作成し、ソルバー情報を共有する
		std::vector<SOLVERINFO_t> whole_solverinfo_list;	// 全体のソルバー情報のリスト
		std::vector<std::string> hostname_list;				// ホスト名のリスト
		createSolversAndGather(cmdline.solverSettingPath(), solver_list, whole_solverinfo_list, hostname_list);
		if (g_mpi_my_rank == ROOT_RANK){
			// 全てのソルバー情報を出力する
			puts("Solvers :");
			for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
				auto &info = whole_solverinfo_list[i];
				if ((0 < info.getSpeed()) && (0 < info.getMemory())){
					int rank = info.getRank();
					const char *hostname = hostname_list[rank].data();
					char cps[64], cap[64];
					putPrefix(info.getSpeed(), cps);
					putPrefix2(info.getMemory(), cap);
					printf("  [%d] %d:%s's solver%d : %scell/s, %sB, '%s'\n", (int)i, rank, hostname, info.getIndex(), cps, cap, info.getName());
				}
			}
			fflush(stdout);
		}

		// テストモードのフラグを全プロセスで共有する
		bool testmode = cmdline.isTestMode();
		MPI_Bcast(&testmode, 1, MPI_C_BOOL, ROOT_RANK, MPI_COMM_WORLD);
		if (testmode == true){
			// テストモードの場合はここで終了する
			goto finalize;
		}

		// FFSituationを作成する
		int num_of_solvers = (int)solver_list.size();
		std::vector<FFSituation> situation_list(num_of_solvers);

		// 入力ファイルを読み込む
		std::vector<uint8_t> mp_data;
		mpack_tree_t mp_tree;
		loadInputFile(cmdline.inputPath(), mp_data, mp_tree);
		mpack_node_t mp_root_node = mpack_tree_root(&mp_tree);

		// Spaceノードをパースする
		mpack_node_t mp_space_node = mpack_node_map_cstr(mp_root_node, "Space");
		index3_t space_size = Parser::parseGridAndBC(mp_space_node, situation_list);
		double num_of_voxels = (double)space_size.x * (double)space_size.y * (double)space_size.z;
		if (g_mpi_my_rank == ROOT_RANK){
			// シミュレーション空間サイズを出力する
			puts("Situation :");
			printf("  Space size = %u x %u x %u\n", space_size.x, space_size.y, space_size.z);
			printf("  Cell count = %llu\n", (uint64_t)space_size.x * space_size.y * space_size.z);
			fflush(stdout);
		}

		// 最適なタイムステップを計算する
		double optimum_timestep = 0.0;
		if (g_mpi_my_rank == ROOT_RANK){
			optimum_timestep = situation_list[0].calcTimestep();
			printf("  Optimum timestep = %e s\n", optimum_timestep);
		}
		MPI_Bcast(&optimum_timestep, 1, MPI_DOUBLE, ROOT_RANK, MPI_COMM_WORLD);
		
		// 計算能力で処理を割り振る
		std::vector<index_t> whole_division_list;
		assignDivision(space_size, whole_solverinfo_list, whole_division_list, situation_list);
		if (g_mpi_my_rank == ROOT_RANK){
			// 処理スライスの割り当てを出力する
			puts("Divisions :");
			index_t start = 0;
			for (size_t i = 0; i < whole_solverinfo_list.size(); i++){
				if (0 < whole_division_list[i]){
					auto &info = whole_solverinfo_list[i];
					int rank = info.getRank();
					const char *hostname = hostname_list[rank].data();
					printf("  Division[%d-%d] -> %d:%s solver%d\n", start, start + whole_division_list[i] - 1, rank, hostname, info.getIndex());
					start += whole_division_list[i];
				}
			}
			fflush(stdout);
		}

		// ソルバーの接続情報を取得する
		std::vector<FFSituation*> bottom_situation, top_situation;
		std::vector<int> bottom_rank, top_rank;
		getSolverConnection(situation_list, whole_solverinfo_list, whole_division_list, bottom_situation, top_situation, bottom_rank, top_rank);

		// Materialノードをパースする
		mpack_node_t mp_material_node = mpack_node_map_cstr(mp_root_node, "Material");
		Parser::parseMaterials(mp_material_node, situation_list);
		if (g_mpi_my_rank == ROOT_RANK){
			// 材質情報を出力する
			puts("Materials :");
			size_t count = situation_list[0].getNumberOfMaterials();
			for (size_t i = 1; i < count; i++){
				const FFMaterial *mat = situation_list[0].getMaterialByID((matid_t)i);
				printf("  [%d] Eps=%f, Sigma=%f, Mu=%f\n", (int)i, mat->eps_r(), mat->sigma(), mat->mu_r());
			}
			fflush(stdout);
		}

		// Objectノードをパースする
		mpack_node_t mp_object_node = mpack_node_map_cstr(mp_root_node, "Object");
		Parser::parseObjects(mp_object_node, situation_list);

		// Portノードをパースする
		mpack_node_t mp_port_node = mpack_node_map_cstr(mp_root_node, "Port");
		Parser::parsePorts(mp_port_node, situation_list);

		// Solverノードをパースし、ソルバーを構成する
		if (g_mpi_my_rank == ROOT_RANK){
			puts("Configuring solvers...");
			fflush(stdout);
		}
		mpack_node_t mp_solver_node = mpack_node_map_cstr(mp_root_node, "Solver");
		size_t max_iteration = Parser::parseSolvers(mp_solver_node, situation_list, solver_list, optimum_timestep);
		solver_list.clear();

		// 入力データを破棄する
		mpack_tree_destroy(&mp_tree);
		mp_data.clear();

		// シミュレーションを行う
		MPI_Barrier(MPI_COMM_WORLD);
		if (g_mpi_my_rank == ROOT_RANK){
			puts("Simulation started");
			fflush(stdout);
		}
		for (size_t it = 0; it < max_iteration; it++){
			if ((it % 100) == 0){
				double total_e = 0.0, total_h = 0.0;
#pragma omp parallel for reduction(+ : total_e, total_h)
				for (int i = 0; i < num_of_solvers; i++){
					dvec2 total = situation_list[i].calcTotalEM();
					total_e += total.x;
					total_h += total.y;
				}
				double buf[2] = {total_e, total_h};
				if (g_mpi_my_rank == ROOT_RANK){
					double recv_buf[2];
					MPI_Reduce(buf, recv_buf, 2, MPI_DOUBLE, MPI_SUM, ROOT_RANK, MPI_COMM_WORLD);
					printf("  Step%d : E=%e, H=%e\n", (int)it, recv_buf[0], recv_buf[1]);
					fflush(stdout);
				}
				else{
					MPI_Reduce(buf, nullptr, 2, MPI_DOUBLE, MPI_SUM, ROOT_RANK, MPI_COMM_WORLD);
				}
			}
			bool result = true;
#pragma omp parallel
			{
#pragma omp for reduction(&& : result)
				for (int i = 0; i < num_of_solvers; i++){
					result &= situation_list[i].executeSolverStep1();
				}
				if (result == true){
#pragma omp for
					for (int i = 0; i < num_of_solvers; i++){
						situation_list[i].executeSolverStep2();
					}
#pragma omp for
					for (int i = 0; i < num_of_solvers; i++){
						situation_list[i].executeSolverStep3(bottom_situation[i], top_situation[i], bottom_rank[i], top_rank[i]);
					}
#pragma omp for
					for (int i = 0; i < num_of_solvers; i++){
						situation_list[i].executeSolverStep4();
					}
#pragma omp for
					for (int i = 0; i < num_of_solvers; i++){
						situation_list[i].executeSolverStep5(bottom_situation[i], top_situation[i], bottom_rank[i], top_rank[i]);
					}
				}
			}
			if (result == false){
				break;
			}
		}

		// シミュレーションを終了する
		MPI_Barrier(MPI_COMM_WORLD);
		if (g_mpi_my_rank == ROOT_RANK){
			puts("Simulation finished");
			fflush(stdout);
		}
		
		// シミュレーション結果を出力する
		for (int i = 0; i < num_of_solvers; i++){
			FFSituation &situation = situation_list[i];
			std::vector<const FFPort*> port_list = situation.getPortList();
			for (size_t i = 0; i < port_list.size(); i++){
				if (port_list[i] == nullptr){
					continue;
				}
				const FFCircuit *circuit = port_list[i]->getCircuit();
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

		if (g_mpi_my_rank == ROOT_RANK){
			puts("Finished");
			fflush(stdout);
		}
	}
	catch (FFException &exception){
		exception.print();
		waitForPressingAnyKey();

		// 計算を中断する
		MPI_Abort(MPI_COMM_WORLD, 0);
	}
	catch (...){
		puts("Simulation aborted with unknown reason");
		waitForPressingAnyKey();

		// 計算を中断する
		MPI_Abort(MPI_COMM_WORLD, 0);
	}

finalize:
	// MPIを終了する
	MPI_Finalize();

	// ソルバーを解放する
	for (auto *solver : solver_list){
		delete solver;
	}

	waitForPressingAnyKey();
	
	return 0;
}
