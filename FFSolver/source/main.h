// 線形に等間隔なdouble型ベクトルを作成する
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

// 数値に接頭辞をつけた文字列を出力する(SI接頭辞)
static void putPrefix(uint64_t value, char *buffer){
	char prefix;
	double value_d;
	if (1000000000000000000ULL <= value){
		value_d = value * 1e-18;
		prefix = 'E';
	}
	else if (1000000000000000ULL <= value){
		value_d = value * 1e-15;
		prefix = 'P';
	}
	else if (1000000000000ULL <= value){
		value_d = value * 1e-12;
		prefix = 'T';
	}
	else if (1000000000ULL <= value){
		value_d = value * 1e-9;
		prefix = 'G';
	}
	else if (1000000ULL <= value){
		value_d = value * 1e-6;
		prefix = 'M';
	}
	else if (1000ULL <= value){
		value_d = value * 1e-3;
		prefix = 'K';
	}
	else{
		sprintf(buffer, "%llu", value);
		return;
	}
	sprintf(buffer, "%.3f%c", value_d, prefix);
}

// 数値に接頭辞をつけた文字列を出力する(2進接頭辞)
static void putPrefix2(uint64_t value, char *buffer){
	char prefix;
	double value_d;
	if ((1ULL << 60) <= value){
		value_d = value * (1.0 / (1ULL << 60));
		prefix = 'E';
	}
	else if ((1ULL << 50) <= value){
		value_d = value * (1.0 / (1ULL << 50));
		prefix = 'P';
	}
	else if ((1ULL << 40) <= value){
		value_d = value * (1.0 / (1ULL << 40));
		prefix = 'T';
	}
	else if ((1ULL << 30) <= value){
		value_d = value * (1.0 / (1ULL << 30));
		prefix = 'G';
	}
	else if ((1ULL << 20) <= value){
		value_d = value * (1.0 / (1ULL << 20));
		prefix = 'M';
	}
	else if ((1ULL << 10) <= value){
		value_d = value * (1.0 / (1ULL << 10));
		prefix = 'K';
	}
	else{
		sprintf(buffer, "%llu", value);
	}
	sprintf(buffer, "%.3f%ci", value_d, prefix);
}


// ソルバーの名前や性能に関する情報を格納するクラス
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

	// 処理速度[cell/s]を取得する
	uint64_t getSpeed(void) const{
		return m_Speed;
	}

	// メモリー容量[byte]を取得する
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


