#include <iostream>
#include <string>

using namespace std;

void displayHelp() {
	std::cout << "Available commands:" << std::endl;
	std::cout << "   /help - chat help, displays a list of commands to manage the chat" << std::endl;
	std::cout << "   /signup - registration, user enters data for registration" << std::endl;
	std::cout << "   /signin - authorization, only a registered user can authorize" << std::endl;
	std::cout << "   /logout - user logout" << std::endl;
	std::cout << "   /remove - delete registered user" << std::endl;
	std::cout << "   /exit - close the program" << std::endl;
	std::cout << "   @username is an addressable message," << std::endl;
	std::cout << "     not an addressable message for all users.\n" << std::endl;
}

void signup(const string& login, const string& password, const string& name)
{
	if (password.empty() || name.empty()) {
		throw std::invalid_argument("Password and name cannot be empty.");
	}
}

int main()
{
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
				displayHelp();
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

				signup(login, password, name);
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