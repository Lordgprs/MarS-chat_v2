#include "chat.h"

using namespace std;

int main()
{
	chat chat_obj;
	chater* loggedUser = nullptr;

	cout << "Welcome to the chat," << endl;
	cout << "to output help, type /help" << endl;

	

	// stores user input
	string input_text;

	while (true)
	{
		try {
			// working out the program algorithm

			cout << (loggedUser ? loggedUser->getLogin() : "") << "> ";
			getline(cin, input_text);

			if (input_text == "/help") {
				// output help
				chat_obj.displayHelp();
			}

			else if (input_text == "/signup") {
				// registration
				if (loggedUser == nullptr) {
					string name, login, password;

					cout << "Enter login: ";
					std::getline(std::cin, login);

					cout << "Enter password: ";
					std::getline(std::cin, password);

					cout << "Enter name: ";
					std::getline(std::cin, name);

					chat_obj.signup(login, password, name);
					cout << "User registered successfully.\n" << endl;
				}
				else {
					cout << "to register, enter /logout.\n" << endl;
				}
			}

			else if (input_text == "/signin") {
				// authorization
				if (loggedUser != nullptr) {
					cout << "to log in, enter /logout.\n" << endl;
				}
				else {
					string login, password;
					cout << "Enter login: ";
					getline(std::cin, login);

					cout << "Enter password: ";
					getline(std::cin, password);

					loggedUser = &chat_obj.signin(login, password);
				}
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