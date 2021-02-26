#include <iostream>
#include <uwebsockets/App.h>
using namespace std;

const string MESSAGE_TO = "MESSAGE_TO::";
const string SET_NAME = "SET_NAME::";

bool isSetName(string message) {
	return message.find(SET_NAME) == 0;
}
string parseName(string message) {
	return message.substr(SET_NAME.size());
}

//getting user id
string parseUserId(string message) {
	string rest = message.substr(MESSAGE_TO.size());
	int pos = rest.find("::");
	return rest.substr(0, pos);
}

/*getting user message*/
string parseUserText(string message) {
	string rest = message.substr(MESSAGE_TO.size());
	int pos = rest.find("::");
	return rest.substr(pos + 2);
}

bool isMessageTo(string message) {
	return message.find(MESSAGE_TO) == 0;
}

string messageFrom(string user_id, string senderName, string message) {
	return "MESSAGE_FROM::" + user_id + "::[" + senderName + "] " + message;
}

int main() {
	//User information
	struct PerSocketData {
		string name;
		unsigned int user_id;
	};

	unsigned int last_user_id = 10;
	unsigned int count_user = 0;

	/*Server configuration*/
	uWS::App(). //create a Server without encryptio
		ws<PerSocketData>("/*", {
		/* Settings */
		.idleTimeout = 1200, //sec
			.open = [&last_user_id, &count_user](auto* ws) { // "Лямбда функция"
				PerSocketData* userData = (PerSocketData*)ws->getUserData();
				userData->name = "UNNAMED";
				userData->user_id = last_user_id++;
				count_user++;
				cout << "New user connected, id = " << userData->user_id << endl;
				cout << "Total user connected: " << count_user << endl;

				ws->subscribe("user#" + to_string(userData->user_id)); //личный канал пользователя
				ws->subscribe("broadcast"); //общий канал
			},
			.message = [&last_user_id](auto* ws, string_view message, uWS::OpCode opCode) {
				string strMessage = string(message);
				PerSocketData* userData = (PerSocketData*)ws->getUserData();
				string authorId = to_string(userData->user_id);

				if (isMessageTo(strMessage)) {
					string receiverId = parseUserId(strMessage);
					if (stoi(receiverId) <= last_user_id && stoi(receiverId) > 10) {
						string text = parseUserText(strMessage);
						string outgoingMessage = messageFrom(authorId, userData->name, text);

						ws->publish("user#" + receiverId, outgoingMessage, uWS::OpCode::TEXT); //отправка получателю
						ws->send("Message sent", uWS::OpCode::TEXT);

						cout << "Author #" << authorId << " wrote message to " << receiverId << endl;
					}
					else ws->send("Error, there is no user with ID = "+ receiverId, uWS::OpCode::TEXT);
				}
				if (isSetName(strMessage)) {
					string newName = parseName(strMessage);
					if (newName.size() <= 255 && newName.find("::") == -1) {
						userData->name = newName;
						cout << "User  #" << authorId << " set their name" << endl;
					}
				}
			},
			.close = [](auto* /*ws*/, int /*code*/, string_view /*message*/) {

			}
			})
		/*Server start*/
				.listen(9001, [](auto* listen_socket) {
				if (listen_socket) {
					cout << "Listening on port " << 9001 << endl;
				}
					}).run();  //Launch
}