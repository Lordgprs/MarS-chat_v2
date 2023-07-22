#include <iostream>
#include <string>

using namespace std;

int main()
{	
	cout << "Welcome to the chat!" << endl;
	cout << "To output help, type /help" << endl;

	// хранит текст введеный пользователем
	string input_text;

	while (true)
	{
		getline(cin, input_text);

		try {
			// отработка алгоритма программы
		}
		catch (const exception e) {
			// отработка исключений
		}

	}



	return 0;
}