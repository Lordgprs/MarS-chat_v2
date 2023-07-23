#include "chat_mgr.h"

using namespace std;

int main()
{
	chat_mgr chat_obj;
	chat_user* loggedUser = nullptr;

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

					chat_obj.signUp(login, password, name);
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

					loggedUser = &chat_obj.signIn(login, password);
				}
			}

			else if (input_text == "/logout") {
				if (loggedUser == nullptr) {
					std::cout << "You are not logged in.\n" << std::endl;
				}
				else {
					chat_obj.signOut(*loggedUser);
					loggedUser = nullptr;
					std::cout << "";
				}
			}

			else if (input_text == "/remove") {
				if (loggedUser == nullptr) {
					std::cout << "You are not logged in.\n" << std::endl;
				}
				else {
					chat_obj.removeUser(*loggedUser);
					loggedUser = nullptr;
					std::cout << "User removed successfully.\n" << std::endl;
				}
			}

			else if (input_text == "/exit") {
				// closing the program
				break;
			}

			else if (loggedUser != nullptr) {
				// if there is an authorized user, we send a message
				chat_obj.sendMessage(*loggedUser, input_text);
			}

			else
			{
				std::cout << "the command is not recognized, " << std::endl;
				std::cout << "to output help, type /help\n" << std::endl;
			}
		}

		catch (const exception e) {
			// exception handling

			cout << "Error: " << e.what() << "\n" << endl;
		}
	}

	return 0;
}