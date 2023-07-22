#include <iostream>
#include <string>

using namespace std;

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

		if (input_text == "/exit") {
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