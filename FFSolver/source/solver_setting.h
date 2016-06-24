#pragma once

#include <string>
#include "FFSolver.h"



namespace SolverSetting{
	// ホスト名を取得する
	void getHostname(char *buf, size_t length);

	// ソルバー設定ファイルを読み込みソルバーを作成する
	void createSolvers(const char *path, const char *hostname, std::vector<FFFDTD::FFSolver*> *solver_list, std::vector<uint64_t> *speed_list);



}
