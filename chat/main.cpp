#include "chat.h"

using namespace std;

int main()
{
	chat chat_obj;

	cout << "Welcome to the chat!" << endl;
	cout << "To output help, type /help" << endl;

	

	// stores user input
	string input_text;

	while (true)
	{
		try {
			// working out the program algorithm


			cout << "> ";
			getline(cin, input_text);

			if (input_text == "/help") {
				// output help
				chat_obj.displayHelp();
			}
			else if (input_text == "/signup") {
				// registration
				string name, login, password;

				cout << "Enter login: ";
				std::getline(std::cin, login);

				cout << "Enter password: ";
				std::getline(std::cin, password);

				cout << "Enter name: ";
				std::getline(std::cin, name);

				chat_obj.signup(login, password, name);
			}
			else if (input_text == "/exit") {
				// closing the program
				break;
			}
		}

		catch (const exception e) {
			// exception handling

			cout << "Error: " << e.what() << "\n" << endl;
		}
	}




	return 0;
}