#include "cmdline.h"
#include <vector>






// コマンドライン引数をパースする
bool Cmdline::parse(int argc, char *argv[]){
	enum STATE_e{
		ST_OPTION,
		ST_SOLVERPATH,
		ST_INPUTPATH,
		ST_OUTPUTPATH,
	};

	bool show_help = (argc == 0);
	STATE_e state = ST_OPTION;
	while (0 < argc--){
		const char *p = *argv++;
		switch (state){
		case ST_OPTION:
			if (p[0] == '-'){
				switch (p[1]){
				case 'h':
					show_help = true;
					break;
				case 's':
					state = ST_SOLVERPATH;
					break;
				case 'i':
					state = ST_INPUTPATH;
					break;
				case 'o':
					state = ST_OUTPUTPATH;
					break;
				case 't':
					m_TestMode = true;
					break;
				default:
					printf("Unknown option '%s'\n", p);
					break;
				}
			}
			break;

		case ST_SOLVERPATH:
			m_SolverSettingPath = p;
			state = ST_OPTION;
			break;

		case ST_INPUTPATH:
			m_InputPath = p;
			state = ST_OPTION;
			break;

		case ST_OUTPUTPATH:
			m_OutputPath = p;
			state = ST_OPTION;
			break;

		default:
			state = ST_OPTION;
			break;
		}
	}

	if (show_help == true){
		// ヘルプメッセージを表示して終了
		puts("  -h  Display this help message");
		puts("  -i  Path to input file (necessary)");
		puts("  -o  Path to output file (necessary)");
		puts("  -t  Test solver's settings flag");
		puts("  -s  Path to solver setting file");
		return false;
	}
	if (m_InputPath.empty()){
		puts("Specify one input file (-i [file path])");
		return false;
	}
	if (m_OutputPath.empty()){
		puts("Specify one output file (-o [file path])");
		return false;
	}

	return true;
}
