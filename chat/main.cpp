#include <iostream>
#include <string>

using namespace std;

int main()
{	
	cout << "Welcome to the chat!" << endl;
	cout << "To output help, type /help" << endl;

	// ������ ����� �������� �������������
	string input_text;

	while (true)
	{
		getline(cin, input_text);

		try {
			// ��������� ��������� ���������
		}
		catch (const exception e) {
			// ��������� ����������
		}

	}



	return 0;
}