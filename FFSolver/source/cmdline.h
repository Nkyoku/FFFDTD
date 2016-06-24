#pragma once

#include <string>
#include <stdint.h>

class Cmdline{
	/*** メンバー変数 ***/
private:
	// ソルバー設定ファイルへのパス
	std::string m_SolverSettingPath = "solvers.ini";

	// 入力ファイルへのパス
	std::string m_InputPath;

	// 出力ファイルへのパス
	std::string m_OutputPath;

	// テストモード
	bool m_TestMode = false;



	/*** メソッド ***/
public:
	// コマンドライン引数をパースする
	bool parse(int argc, char *argv[]);

	// ソルバー設定ファイルへのパスを取得する
	const char* solverSettingPath(void) const{
		return m_SolverSettingPath.c_str();
	}

	// 入力ファイルへのパスを取得する
	const char* inputPath(void) const{
		return m_InputPath.c_str();
	}

	// 出力ファイルへのパスを取得する
	const char* outputPath(void) const{
		return m_OutputPath.c_str();
	}

	// テストモードか取得する
	bool isTestMode(void) const{
		return m_TestMode;
	}
};
