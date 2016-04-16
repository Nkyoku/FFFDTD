#include "FFVoxel2D.h"
#include "../FFConst.h"
#include <memory>



namespace MUFDTD{
	// 2次元ボクセルデータのヘッダー文字列
	const char FFVoxel2D::HEADER_STRING[] = "FF2D";
	


	// コンストラクタ
	FFVoxel2D::FFVoxel2D()
		: m_Size(0, 0)
	{
		
	}

	// コピーコンストラクタ
	FFVoxel2D::FFVoxel2D(const FFVoxel2D &obj)
		: m_Size(obj.m_Size), m_Data(obj.m_Size.y, false)
	{
		for (index_t i = 0; i < m_Size.y; i++){
			m_Data[i] = new std::vector<bool>(*obj.m_Data[i]);
		}
	}

	// コンストラクタ
	// 与えた入力ストリームは削除される
	FFVoxel2D::FFVoxel2D(FFIStream *stream){
		loadFromIStream(stream);
	}

	// デストラクタ
	FFVoxel2D::~FFVoxel2D(){
		for (index_t i = 0; i < m_Size.y; i++){
			delete m_Data[i];
		}
	}

	// 入力ストリームからボクセルデータを読み込む
	// 与えた入力ストリームは削除される
	void FFVoxel2D::loadFromIStream(FFIStream *stream_){
		std::unique_ptr<FFIStream> stream(stream_);

		// ヘッダーをパースする
		if (stream->check(HEADER_STRING, strlen(HEADER_STRING)) == false) throw;
		index2_t size;
		size.x = stream->get4byte();
		size.y = stream->get4byte();
		if ((size.x < 0) || (MAX_SIZE < size.x)) throw;
		if ((size.y < 0) || (MAX_SIZE < size.y)) throw;

		// メモリーを確保する
		setSize(size);

		// デコードする
		// 0の個数 → 1の個数 → 0の個数 → ・・・ という順番に1バイトのデータが続く
		uint64_t remaining = (uint64_t)m_Size.x * m_Size.y;
		bool value = false;
		index_t x = 0, y = 0;
		while (0 < remaining){
			uint8_t count = stream->get1byte();
			if (remaining < count) throw;
			remaining -= count;
			while (0 < count--){
				setPointInternal(x, y, value);
				x++;
				if (x == size.x){
					x = 0;
					y++;
				}
			}
			value = !value;
		}
	}

	// ボクセルデータの大きさを設定する
	void FFVoxel2D::setSize(const index2_t &size){
		// サイズをチェックする
		if ((size.x <= 0) || (MAX_SIZE < size.x)) throw;
		if ((size.y <= 0) || (MAX_SIZE < size.y)) throw;

		if (size.y < m_Size.y){
			// Y方向に縮小する場合はラインを削除する
			for (index_t i = size.y; i < m_Size.y; i++){
				delete m_Data[i];
			}
			m_Data.resize(size.y);
		}
		else if (m_Size.y < size.y){
			// Y方向に拡張する場合はラインを追加する
			m_Data.resize(size.y);
			for (index_t i = m_Size.y; i < size.y; i++){
				m_Data[i] = new std::vector<bool>(size.x, false);
			}
		}

		// 既存のラインをX方向に拡張・縮小する
		for (index_t i = 0; i < m_Size.y; i++){
			(m_Data[i])->resize(size.x, false);
		}
		
		m_Size = size;
	}

	// ボクセルデータにオフセットを追加する
	void FFVoxel2D::addOffset(const index2_t &offset){
		// サイズをチェックする
		index2_t size = offset + m_Size;
		if ((size.x <= 0) || (MAX_SIZE < size.x) || (size.x < m_Size.x)) throw;
		if ((size.y <= 0) || (MAX_SIZE < size.y) || (size.y < m_Size.y)) throw;

		// X方向にオフセットを追加する
		for (index_t i = 0; i < m_Size.y; i++){
			std::vector<bool> tmp(*m_Data[i]);
			std::vector<bool> &line = *m_Data[i];
			line.resize(size.x);
			for (index_t i = 0; i < offset.x; i++){
				line[i] = false;
			}
			for (index_t i = 0; i < m_Size.x; i++){
				line[offset.x + i] = tmp[i];
			}
		}

		// Y方向にオフセットを追加する
		m_Data.resize(size.y);
		memmove(&m_Data[offset.x], &m_Data[0], m_Size.y);
		for (index_t i = 0; i < offset.y; i++){
			 m_Data[i] = new std::vector<bool>(size.x, false);
		}

		m_Size = size;
	}






}

