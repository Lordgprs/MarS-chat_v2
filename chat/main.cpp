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

int main()
{
	cout << "Welcome to the chat!" << endl;
	cout << "To output help, type /help" << endl;

	// stores user input
	string input_text;

	while (true)
	{
		cout << "> ";
		getline(cin, input_text);

		if (input_text == "/help") {
			displayHelp();
		}
		else if (input_text == "/exit") {
			// closing the program
			break;
		}


		try {
			// working out the program algorithm
		}
		catch (const exception e) {
			// exception handling
		}

	}



	return 0;
}