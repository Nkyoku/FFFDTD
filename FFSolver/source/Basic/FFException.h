#pragma once

#include <stdexcept>
#include <string>



namespace FFFDTD{
	// 例外クラス
	class FFException : public std::exception{
		/*** メンバー変数 ***/
	private:
		// 例外メッセージ
		std::string m_Message;
		

		
		/*** メンバー ***/
	public:
		// コンストラクタ
		FFException(void) : m_Message(){}

		// コンストラクタ
		FFException(const char *format, ...);

		// コンストラクタ
		FFException(int line, const char *file);

		// コンストラクタ
		FFException(int line, const char *file, const char *format, ...);

		// 例外メッセージを返す
		const char* what() const override{
			return m_Message.c_str();
		}

		// 例外メッセージを表示する
		void print() const{
			puts(what());
		}
	};
}
